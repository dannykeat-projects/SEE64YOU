#!/usr/bin/env python3
"""Bounded C64U standard-network fixture gate; no installed files are replaced."""
import argparse, ftplib, hashlib, io, json, re, time, urllib.parse, urllib.request
from pathlib import Path


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--host',required=True)
    p.add_argument('--prg',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=False)
    state={'input':'REST-assisted; not physical keyboard','restored':False,'active':False}
    def save(): (a.output/'state.json').write_text(json.dumps(state,indent=2)+'\n')
    def api(path,method='GET',**q):
        url='http://'+a.host+'/v1/'+path
        if q: url+='?'+urllib.parse.urlencode(q)
        with urllib.request.urlopen(urllib.request.Request(url,method=method),timeout=12) as r:return r.read()
    def mem(address,size):return api('machine:readmem',address=f'{address:04X}',length=size)
    def screen():
        b=mem(0x400,1000)
        return '\n'.join(''.join(chr((v&127)+64 if (v&127)<32 else v&127) for v in b[i:i+40]) for i in range(0,1000,40))
    def keys(b):
        for i in range(0,len(b),8):
            deadline=time.monotonic()+10
            while mem(0xc6,1)!=b'\0':
                if time.monotonic()>deadline:raise RuntimeError('Keyboard buffer busy')
                time.sleep(.05)
            chunk=b[i:i+8]
            api('machine:writemem','PUT',address='0277',data=chunk.hex())
            api('machine:writemem','PUT',address='00C6',data=bytes([len(chunk)]).hex())
            time.sleep(.25)
    def wait_ready():
        deadline=time.monotonic()+30
        while time.monotonic()<deadline:
            if 'READY.' in screen() and mem(0xc6,1)==b'\0':return
            time.sleep(.1)
        raise RuntimeError('Responsive BASIC not reached')
    def live(address):
        keys(f'POKE827,PEEK({address})\r'.encode());wait_ready();return mem(827,1)[0]
    assert 'READY.' in screen() and mem(0xc6,1)==b'\0','Need idle BASIC'
    assert mem(int.from_bytes(mem(0x2b,2),'little'),2)==b'\0\0','Existing BASIC program'
    saved_scratch=mem(827,1)
    state['scratch_827']=saved_scratch.hex();save()
    original={'irq':mem(0x314,2).hex(),'turbo':live(53297),'colours':[live(x) for x in (53280,53281,646)]}
    drives=api('drives');configs=api('configs')
    state['before']=original
    data=a.prg.read_bytes();state['prg_sha256']=hashlib.sha256(data).hexdigest()
    state['source_commit']=__import__('subprocess').check_output(['git','rev-parse','HEAD'],text=True).strip()
    entry=re.search(b'\x9e([0-9]+)\x00',data[2:34]);assert entry,'Missing SYS entry'
    folder='/sd/S64NETTEST20261027';remote=folder+'/NTEST.PRG';created=False;ftp=None
    try:
        ftp=ftplib.FTP(a.host,timeout=20);ftp.login()
        assert not any(x.rstrip('/').rsplit('/',1)[-1].upper()=='S64NETTEST20261027' for x in ftp.nlst('/sd')),'Test folder exists'
        ftp.mkd(folder);created=True
        ftp.storbinary('STOR '+remote,io.BytesIO(data),blocksize=256)
        read=io.BytesIO();ftp.retrbinary('RETR '+remote,read.write)
        assert read.getvalue()==data,'Program readback differs'
        save();api('runners:load_prg','PUT',file=remote);time.sleep(.7)
        keys(f"POKE53297,{original['turbo']}\r".encode());wait_ready()
        # POKE returns to BASIC; clear screen so an old READY cannot certify exit.
        keys(b'\x93SYS'+entry.group(1)+b'\r');state['active']=True;save()
        deadline=time.monotonic()+180
        while time.monotonic()<deadline:
            progress,result,detail=mem(0xc060,3)
            if result in (0xa5,0x5a):
                state.update(progress=progress,result=result,detail=detail);break
            time.sleep(.3)
        else:raise RuntimeError('Network diagnostic timed out; do not reset')
        (a.output/'result-screen.txt').write_text(screen())
        # Both result branches end at the same bounded STOP return prompt.
        keys(b'\x03');wait_ready();state['active']=False
        after={'irq':mem(0x314,2).hex(),'turbo':live(53297),'colours':[live(x) for x in (53280,53281,646)]}
        assert after==original,(original,after)
        assert api('drives')==drives and api('configs')==configs,'Configuration changed'
        keys(b'\x93PRINT 6*7\r');wait_ready();assert re.search(r'(?:^|\n)\s*42\s*(?:\n|$)',screen())
        keys(b'\x93PRINT"OK"\r');wait_ready();assert re.search(r'(?:^|\n)OK\s*(?:\n|$)',screen())
        keys(b'NEW\r');wait_ready()
        assert mem(int.from_bytes(mem(0x2b,2),'little'),2)==b'\0\0','Diagnostic BASIC stub remains'
        api('machine:writemem','PUT',address='033B',data=saved_scratch.hex())
        state['after']=after;state['restored']=True
        assert state['result']==0xa5,state
    except Exception as e:
        state['error']=str(e);raise
    finally:
        if ftp:
            if created and not state['active']:
                ftp.delete(remote);ftp.rmd(folder);state['test_files_removed']=True
            ftp.quit()
        save()
    print('PASS real C64U standard network; exact temporary PRG readback; DOS contexts, IRQ/turbo/colours/config/drives and responsive BASIC verified')

if __name__=='__main__':main()
