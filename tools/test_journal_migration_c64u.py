# Run only when the C64U is available for disposable, backed-up acceptance.
from pathlib import Path
import argparse,urllib.request,urllib.parse,ftplib,io,time,json,hashlib,re
parser=argparse.ArgumentParser(description="Disposable C64U journal staging/retirement acceptance; exact backups and restoration. Run from repository root with an empty BASIC program.")
parser.add_argument('--host',required=True,help='C64U IP or hostname')
parser.add_argument('--output',required=True,help='NEW receipt/backup directory; must not exist')
args=parser.parse_args()
ROOT=Path('.').resolve();OUT=Path(args.output).resolve();OUT.mkdir(parents=True,exist_ok=False)
HOST=args.host;STATE=OUT/'state.json'
PATHS=['/sd/S64JRN0.DAT','/sd/S64JRN1.DAT','/sd/SEE64YOU/S64JRN0.DAT','/sd/SEE64YOU/S64JRN1.DAT','/sd/SEE64YOU/S64MIG.PRG','/sd/SEE64YOU/S64OLD0.DAT','/sd/SEE64YOU/S64OLD1.DAT']
def api(path,method='GET',**args):
    url='http://'+HOST+'/v1/'+path
    if args:url+='?'+urllib.parse.urlencode(args)
    with urllib.request.urlopen(urllib.request.Request(url,method=method),timeout=10) as r:return r.read()
def mem(a,n):return api('machine:readmem',address=f'{a:04X}',length=n)
def screen(label):
    b=mem(0x400,1000)
    s='\n'.join(''.join(chr((v&127)+64 if (v&127)<32 else v&127) for v in b[i:i+40]) for i in range(0,1000,40))
    (OUT/(label+'.txt')).write_text(s);return s
def keys(b):
    assert len(b)<=10
    end=time.monotonic()+8
    while mem(0xc6,1)!=b'\0':
        if time.monotonic()>end:raise RuntimeError('Keyboard busy')
        time.sleep(.1)
    api('machine:writemem','PUT',address='0277',data=b.hex())
    api('machine:writemem','PUT',address='00C6',data=bytes([len(b)]).hex());time.sleep(.15)
def basic(b):
    for i in range(0,len(b),10):keys(b[i:i+10])
    time.sleep(.3)
def live():
    basic(b'POKE827,PEEK(53297)\r');return mem(0x33b,1)[0]
def fetch(f,p):
    b=io.BytesIO()
    try:f.retrbinary('RETR '+p,b.write)
    except ftplib.error_perm as e:
        if str(e).startswith('550'):
            parent, name = p.rsplit('/', 1)
            names = [entry.rstrip('/').rsplit('/', 1)[-1].upper() for entry in f.nlst(parent)]
            if name.upper() not in names:return None
        raise
    return b.getvalue()
def put(f,p,b):
    f.storbinary('STOR '+p,io.BytesIO(b));assert fetch(f,p)==b,'Readback '+p
def delete(f,p):
    try:f.delete(p)
    except ftplib.error_perm as e:
        if not str(e).startswith('550') or fetch(f,p) is not None:raise
def checksum(b):
    b=bytearray(b);a,c=0x53,0x4a
    for v in b[:22]:a=(a+v)&255;c=(c+a)&255
    b[22:24]=bytes([a,c]);return bytes(b)
def wire(b,state,seq):
    b=bytearray(b);b[5]=state;b[12:16]=seq.to_bytes(4,'little');return checksum(b)
def verify(b):
    assert b is not None and len(b)==24 and b[:5]==b'S64J\1' and checksum(b)==b
    return b
assert not STATE.exists(),'Existing session receipt: use separate session or restore first'
assert 'READY.' in screen('before'),'Need idle BASIC'
assert mem(int.from_bytes(mem(0x2b,2),'little'),2)==b'\0\0','Save/clear existing BASIC program before testing'
for name in ('journal-migration-probe.prg','journal-migration-check.prg','journal-migration-retire.prg'):
    assert (ROOT/'client/build'/name).is_file(),'Compile diagnostics first: '+name
prior=live();irq=mem(0x314,2);drives=api('drives');(OUT/'drives-before.json').write_bytes(drives)
f=ftplib.FTP(HOST,timeout=10);f.login();original={}
for i,p in enumerate(PATHS):
    b=fetch(f,p);original[p]=None if b is None else {'file':f'original-{i}.bin','sha256':hashlib.sha256(b).hexdigest()}
    if b is not None:(OUT/f'original-{i}.bin').write_bytes(b);assert (OUT/f'original-{i}.bin').read_bytes()==b
s={'original':original,'turbo':prior,'restored':False,'tests':[]};STATE.write_text(json.dumps(s,indent=2));active=False

def snapshot():return {p:fetch(f,p) for p in PATHS[:4]}
def clear_journals():
    for p in PATHS[:4]:delete(f,p)
def run(name,label,expected,entry_turbo=None):
    global active
    assert 'READY.' in screen('pre-'+label)
    data=(ROOT/'client/build'/name).read_bytes();put(f,PATHS[4],data)
    entry=re.search(b'\x9e([0-9]+)\x00',data[2:34]);assert entry
    api('runners:load_prg','PUT',file=PATHS[4]);time.sleep(.5)
    if entry_turbo is None:entry_turbo=prior
    basic(('POKE53297,'+str(entry_turbo)+'\r').encode());assert live()==entry_turbo
    api('machine:writemem','PUT',address='C040',data='00000000')
    keys(b'\x93');active=True;basic(b'SYS'+entry.group(1)+b'\r')
    end=time.monotonic()+45
    while time.monotonic()<end:
        result=mem(0xc040,4)
        if result[0] in (0xa5,0x5a):break
        time.sleep(.2)
    else:raise AssertionError('Migration probe did not finish')
    time.sleep(.4);assert result[0]==expected and result[2]==entry_turbo and result[3]==prior,(label,result.hex())
    keys(b'\x93');basic(b'PRINT 6*7\r');assert '42' in screen(label)
    assert live()==prior and mem(0x314,2)==irq
    active=False;s['tests'].append({'name':label,'report':result.hex(),'BASIC_42':True});STATE.write_text(json.dumps(s,indent=2))
    print(label,result.hex(),flush=True);return result
def fixtures(peer=None):
    clear_journals()
    for p in PATHS[5:]:delete(f,p)
    put(f,PATHS[0],clean);put(f,PATHS[1],original_active if peer is None else peer)
    put(f,PATHS[3],folder)

def all_files():return {p:fetch(f,p) for p in PATHS[:4]+PATHS[5:]}

try:
    clear_journals();run('journal-migration-probe.prg','legacy-active-clean-stage',0xa5)
    original_active=verify(fetch(f,PATHS[1]));clean=verify(fetch(f,PATHS[0]));folder=verify(fetch(f,PATHS[3]))
    assert original_active[5]==1 and original_active[12:16]==b'\1\0\0\0'
    assert clean==wire(original_active,2,2) and folder==wire(original_active,2,1)
    (OUT/'seed-active.bin').write_bytes(original_active);(OUT/'root-clean.bin').write_bytes(clean);(OUT/'folder-clean.bin').write_bytes(folder)
    delete(f,PATHS[0]);put(f,PATHS[3],wire(original_active,1,9));before=snapshot()
    r=run('journal-migration-check.prg','concurrent-active-refused',0x5a);assert r[1]==2 and snapshot()==before
    put(f,PATHS[3],folder[:6]);before=snapshot()
    r=run('journal-migration-check.prg','partial-folder-refused',0x5a);assert r[1]==2 and snapshot()==before
    clear_journals();put(f,PATHS[0],clean)
    run('journal-migration-check.prg','clean-stage-retry',0xa5)
    assert fetch(f,PATHS[0])==clean and fetch(f,PATHS[3])==folder
    clear_journals();put(f,PATHS[0],original_active)
    run('journal-migration-check.prg','wrong-slot-active-preserved',0xa5)
    assert fetch(f,PATHS[0])==original_active and fetch(f,PATHS[1])==clean and fetch(f,PATHS[3])==folder
    # Simulate an interrupted session leaving a different, previously proven
    # turbo speed. Recovery must restore the recorded byte before CLEAN.
    clear_journals();put(f,PATHS[0],original_active)
    changed_turbo=(prior & 0xf0) | (0 if (prior & 0x0f)==0x0f else 0x0f)
    run('journal-migration-check.prg','legacy-active-restores-changed-turbo',0xa5,changed_turbo)
    assert fetch(f,PATHS[0])==original_active and fetch(f,PATHS[1])==clean and fetch(f,PATHS[3])==folder
    fixtures()
    run('journal-migration-retire.prg','clean-root-backed-up-and-retired',0xa5)
    assert fetch(f,PATHS[0]) is None and fetch(f,PATHS[1]) is None
    assert fetch(f,PATHS[5])==clean and fetch(f,PATHS[6])==original_active
    assert fetch(f,PATHS[3])==folder
    before=all_files()
    run('journal-migration-retire.prg','retirement-idempotent',0xa5)
    assert all_files()==before
    fixtures();delete(f,PATHS[0]);before=all_files()
    r=run('journal-migration-retire.prg','active-root-retirement-refused',0x5a)
    assert r[1]==2 and all_files()==before
    fixtures();put(f,PATHS[3],wire(folder,1,9));before=all_files()
    r=run('journal-migration-retire.prg','active-folder-retirement-refused',0x5a)
    assert r[1]==2 and all_files()==before
    fixtures();put(f,PATHS[5],b'CONFLICT');before=all_files()
    r=run('journal-migration-retire.prg','different-backup-refused',0x5a)
    assert r[1]==2 and all_files()==before
    fixtures(original_active[:6])
    run('journal-migration-retire.prg','torn-peer-backed-up-exact-length',0xa5)
    assert fetch(f,PATHS[0]) is None and fetch(f,PATHS[1]) is None
    assert fetch(f,PATHS[5])==clean and fetch(f,PATHS[6])==original_active[:6]
    fixtures(original_active+b'X');before=all_files()
    r=run('journal-migration-retire.prg','oversized-peer-preserved',0x5a)
    assert r[1]==2 and all_files()==before
    s['passed']=True
finally:
    if not active:
        for p,item in original.items():
            if item is None:delete(f,p)
            else:put(f,p,(OUT/item['file']).read_bytes())
        for p,item in original.items():
            b=fetch(f,p)
            assert b is None if item is None else hashlib.sha256(b).hexdigest()==item['sha256']
        assert json.loads(api('drives'))==json.loads(drives) and mem(0x314,2)==irq
        basic(('POKE53297,'+str(prior)+'\r').encode());assert live()==prior
        # Entry requires an empty BASIC program; remove our diagnostic stub.
        basic(b'NEW\r')
        assert mem(int.from_bytes(mem(0x2b,2),'little'),2)==b'\0\0'
        s['restored']=True;STATE.write_text(json.dumps(s,indent=2))
        print('Original journals/probe restored byte-exact; BASIC/IRQ/drives/live turbo restored',prior,flush=True)
    f.quit()
assert s.get('passed') and s['restored']
