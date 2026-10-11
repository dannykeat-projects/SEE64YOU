#!/usr/bin/env python3
"""Backed-up public-browser acceptance on an available C64U with empty BASIC.

Reads a disposable published site; never writes its documents or SD config.
The output directory must be new. Original browser PRG is restored byte-exact.
"""
import argparse
import ftplib
import hashlib
import io
import json
from pathlib import Path
import re
import time
import urllib.parse
import urllib.request

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--site-url', required=True, help='Disposable fixture base URL, ending /')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    state = {'restored': False, 'tests': []}
    def save(): (out/'state.json').write_text(json.dumps(state, indent=2))
    def api(path, method='GET', **query):
        url = 'http://' + args.host + '/v1/' + path
        if query: url += '?' + urllib.parse.urlencode(query)
        with urllib.request.urlopen(urllib.request.Request(url, method=method), timeout=10) as reply:
            return reply.read()
    def mem(address, length): return api('machine:readmem', address=f'{address:04X}', length=length)
    def screen(label):
        data = mem(0x400, 1000)
        text = '\n'.join(''.join(chr((v&127)+64 if (v&127)<32 else v&127)
            for v in data[i:i+40]) for i in range(0, 1000, 40))
        (out/(label+'.txt')).write_text(text)
        return text
    def keys(data):
        for start in range(0, len(data), 8):
            deadline = time.monotonic()+10
            while mem(0xc6,1) != b'\0':
                if time.monotonic()>deadline: raise RuntimeError('Keyboard busy')
                time.sleep(.05)
            chunk = data[start:start+8]
            api('machine:writemem','PUT',address='0277',data=chunk.hex())
            api('machine:writemem','PUT',address='00C6',data=bytes([len(chunk)]).hex())
            time.sleep(.2)
    def wait(expected, label):
        deadline = time.monotonic()+40
        while time.monotonic()<deadline:
            text = screen(label)
            if expected.upper() in text.upper(): return text
            time.sleep(.2)
        raise AssertionError((expected, text))
    def live():
        keys(b'POKE827,PEEK(53297)\r')
        return mem(827,1)[0]
    assert 'READY.' in screen('before'), 'Need idle BASIC'
    assert mem(int.from_bytes(mem(0x2b,2),'little'),2)==b'\0\0', 'Save/clear BASIC program first'
    prior = live(); irq = mem(0x314,2); drives = api('drives')
    state['turbo'] = prior; state['irq'] = irq.hex();
    (out/'drives-before.json').write_bytes(drives); save()
    ftp = ftplib.FTP(args.host, timeout=10); ftp.login()
    path = '/sd/SEE64YOU/S64WEB.PRG'
    def fetch():
        target = io.BytesIO()
        try: ftp.retrbinary('RETR '+path, target.write)
        except ftplib.error_perm as error:
            if str(error).startswith('550'):
                names = [name.rstrip('/').rsplit('/',1)[-1].upper() for name in ftp.nlst('/sd/SEE64YOU')]
                if 'S64WEB.PRG' not in names: return None
            raise
        return target.getvalue()
    original = fetch()
    if original is not None: (out/'original-browser.prg').write_bytes(original)
    state['original_sha256'] = None if original is None else hashlib.sha256(original).hexdigest(); save()
    active = False
    def enter_url(url, label):
        keys(b'O'); wait('OPEN PUBLIC URL',label+'-prompt')
        # Firmware keyboard injection takes PETSCII, not network ASCII.
        native = bytes(c-32 if 97 <= c <= 122 else c+128 if 65 <= c <= 90 else c
            for c in url.encode('ascii'))
        keys(native+b'\r')
    def opened(url, expected, label):
        enter_url(url, label)
        text = wait(expected,label)
        state['tests'].append(label); save(); return text
    try:
        data = (root/'client/build/independent/S64WEB.PRG').read_bytes()
        entry = re.search(b'\x9e([0-9]+)\x00', data[2:34]); assert entry
        ftp.storbinary('STOR '+path,io.BytesIO(data)); assert fetch()==data
        api('runners:load_prg','PUT',file=path); time.sleep(.5)
        active = True; keys(b'SYS'+entry.group(1)+b'\r')
        wait('SEE64YOU PUBLIC BROWSER','startup'); time.sleep(2)
        base = args.site_url
        opened(base+'index.php','Home body fixture','home')
        keys(b'\x11\x11\r'); wait('Page body fixture','page-link')
        first = screen('page-before-paging')
        state['tests'].append('page-link'); save()
        keys(b' '); second = screen('next-page'); assert second != first
        keys(b'-'); wait('Page body fixture','previous-page')
        keys(b'\x11\x11\x11\r'); wait('Post fixture','posts-link')
        state['tests'].append('posts-link'); save()
        # One key per screen redraw avoids racing conio's buffered input.
        for _ in range(7): keys(b'\x11')
        keys(b'\r'); wait('Post body fixture','post-link')
        state['tests'].append('post-link'); save()
        enter_url(base+'slow.php','slow')
        wait('Loading...','loading-feedback')
        keys(b'\x03'); wait('Cancelled; socket closed','cancel-loading')
        state['tests'].append('loading-feedback/cancel'); save()
        opened(base+'page.php?slug=missing','Page missing or server error','missing-page')
        keys(b'\r'); wait('Page missing or server error','no-stale-link')
        opened('https://example.org/','HTTPS unavailable','https-refusal')
        opened(base+'index.php','Home body fixture','reopen-after-error')
        keys(b'\x03'); wait('READY.','exit'); active = False
        keys(b'PRINT 6*7\r'); wait('42','basic-number')
        keys(b'\x93PRINT"OK"\r'); wait('\nOK ','basic-string')
        assert live()==prior and mem(0x314,2)==irq and api('drives')==drives
        state['tests'].append('BASIC/IRQ/drives/turbo'); save()
    finally:
        if active:
            # Cancel an outstanding prompt/request, then request a clean exit.
            keys(b'\x03'); time.sleep(1)
            if 'READY.' not in screen('first-exit'):
                keys(b'\x03'); time.sleep(1)
            active = 'READY.' not in screen('failure-exit')
        if not active:
            if original is None:
                ftp.delete(path); assert fetch() is None
            else:
                ftp.storbinary('STOR '+path,io.BytesIO(original)); assert fetch()==original
            keys(b'NEW\r'); wait('READY.','restored')
            state['restored'] = True; save()
        else:
            state['restoration_blocked'] = 'Browser still active; original file retained in receipt'; save()
        ftp.quit()
    print(json.dumps(state, indent=2))

if __name__ == '__main__': main()
