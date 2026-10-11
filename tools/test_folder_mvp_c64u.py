#!/usr/bin/env python3
"""Disposable folder-MVP acceptance. Requires available, empty BASIC.

Backups contain private configuration: keep the new output directory private.
REST keyboard-buffer injection is not physical/USB keyboard acceptance.
No machine reset is performed. Failure restoration requires verified BASIC.
"""
import argparse
import ftplib
import hashlib
import io
import json
from pathlib import Path
import re
import struct
import time
import urllib.parse
import urllib.request


def journal(state, sequence, turbo=0):
    wire = bytearray(b'S64J' + bytes([1, state]))
    wire += struct.pack('<HIII', 0x201, 0xF01D0001, sequence, 0)
    wire += bytes([1, turbo])
    a, b = 0x53, 0x4a
    for value in wire:
        a = (a + value) & 255
        b = (b + a) & 255
    return bytes(wire) + struct.pack('<H', (b << 8) | a)


def decode_journal(wire):
    if wire is None or len(wire) != 24 or wire[:5] != b'S64J\x01':
        raise AssertionError('Invalid journal length/header')
    a, b = 0x53, 0x4a
    for value in wire[:22]:
        a = (a + value) & 255
        b = (b + a) & 255
    assert int.from_bytes(wire[22:], 'little') == (b << 8) | a
    return {'state': wire[5], 'sequence': int.from_bytes(wire[12:16], 'little'),
            'turbo': wire[21]}


def source(title, body):
    return ('SEE64YOU/1\nType: post\nSlug: folder-fixture\nTitle: ' + title
            + '\nStatus: published\nDate: \nOrder: 0\n\n' + body).encode('ascii')


def native(text):
    return bytes(value - 32 if 97 <= value <= 122 else value + 128
                 if 65 <= value <= 90 else value for value in text.encode('ascii'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    package = root / 'client/build/independent-package/SEE64YOU'
    expected = {'SEE64YOU.PRG', 'S64CFG.PRG', 'S64EDIT.PRG', 'S64PUB.PRG',
                'S64WEB.PRG', 'S64FILE.BIN', 'S64MODEL.BIN', 'S64PREP.BIN',
                'S64RUN.BIN', 'S64MODULE.MNF', 'S64OVL.MNF'}
    assert {p.name for p in package.iterdir() if p.is_file()} == expected
    candidate = {p.name: p.read_bytes() for p in package.iterdir() if p.is_file()}
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=False)
    state = {'restored': False, 'active': False, 'phase': 'preflight', 'tests': [],
             'keyboard_scope': 'REST keyboard-buffer injection only'}

    def save():
        (out / 'state.json').write_text(json.dumps(state, indent=2))

    def phase(name, active=None):
        state['phase'] = name
        if active is not None:
            state['active'] = active
        save()

    def api(path, method='GET', **query):
        url = 'http://' + args.host + '/v1/' + path
        if query:
            url += '?' + urllib.parse.urlencode(query)
        with urllib.request.urlopen(urllib.request.Request(url, method=method), timeout=10) as reply:
            return reply.read()

    def mem(address, length):
        return api('machine:readmem', address=f'{address:04X}', length=length)

    def screen(label):
        data = mem(0x400, 1000)
        text = '\n'.join(''.join(chr((v & 127) + 64 if (v & 127) < 32 else v & 127)
                                  for v in data[i:i + 40]) for i in range(0, 1000, 40))
        (out / (label + '.txt')).write_text(text)
        return text

    def keys(data):
        for start in range(0, len(data), 8):
            deadline = time.monotonic() + 10
            while mem(0xc6, 1) != b'\0':
                if time.monotonic() > deadline:
                    raise RuntimeError('Keyboard busy; no extra input injected')
                time.sleep(.05)
            chunk = data[start:start + 8]
            api('machine:writemem', 'PUT', address='0277', data=chunk.hex())
            api('machine:writemem', 'PUT', address='00C6', data=bytes([len(chunk)]).hex())
            time.sleep(.2)

    def wait(expected, label, timeout=80):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            text = screen(label)
            if expected.upper() in text.upper() and mem(0xc6, 1) == b'\0':
                return text
            time.sleep(.25)
        raise AssertionError((label, expected, text))

    def basic(label):
        wait('READY.', label)
        keys(b'\x93PRINT 6*7\r')
        text = wait('READY.', label + '-number')
        assert re.search(r'(?:^|\n)\s*42\s*(?:\n|$)', text), text
        keys(b'\x93PRINT"OK"\r')
        text = wait('READY.', label + '-string')
        assert re.search(r'(?:^|\n)OK\s*(?:\n|$)', text), text
        phase(label, False)

    def turbo(value=None):
        assert not state['active'], 'Live register sampling requires verified BASIC'
        if value is not None:
            keys(('POKE53297,' + str(value) + '\r').encode('ascii'))
            wait('READY.', 'turbo-set')
        keys(b'POKE827,PEEK(53297)\r')
        wait('READY.', 'turbo-read')
        return mem(827, 1)[0]

    assert 'READY.' in screen('before'), 'Need idle BASIC'
    assert mem(0xc6, 1) == b'\0', 'Keyboard buffer must be empty'
    assert mem(int.from_bytes(mem(0x2b, 2), 'little'), 2) == b'\0\0', 'Save/clear BASIC first'
    basic('baseline-basic')
    prior = turbo()
    irq = mem(0x314, 2)
    drives = api('drives')
    state.update(turbo=prior, irq=irq.hex())
    (out / 'drives-before.json').write_bytes(drives)
    ftp = ftplib.FTP(args.host, timeout=15)
    ftp.login()
    folder = '/sd/SEE64YOU/'
    paths = [folder + name for name in sorted(expected)]
    for directory in ('/sd/', folder):
        paths += [directory + name for name in ('S64JRN0.DAT', 'S64JRN1.DAT',
                                                 'SEE64YOU.REC', 'SEE64YOU.REC.tmp',
                                                 'S6F00.S64', 'S6F00.S64.tmp')]
    paths += [folder + name for name in ('S64OLD0.DAT', 'S64OLD1.DAT', 'SEE64YOU.CFG')]

    def fetch(path):
        target = io.BytesIO()
        try:
            ftp.retrbinary('RETR ' + path, target.write)
        except ftplib.error_perm as error:
            if str(error).startswith('550'):
                directory, name = path.rsplit('/', 1)
                names = {n.rstrip('/').rsplit('/', 1)[-1].upper() for n in ftp.nlst(directory)}
                if name.upper() not in names:
                    return None
            raise
        return target.getvalue()

    def put(path, data):
        ftp.storbinary('STOR ' + path, io.BytesIO(data))
        assert fetch(path) == data, ('Upload mismatch', path)

    def delete(path):
        if fetch(path) is not None:
            ftp.delete(path)
        assert fetch(path) is None

    originals = {path: fetch(path) for path in paths}
    state['originals'] = {}
    for index, (path, data) in enumerate(originals.items()):
        filename = 'original-' + str(index) + '.bin'
        if data is not None:
            (out / filename).write_bytes(data)
        state['originals'][path] = {'backup': filename if data is not None else None,
                                   'sha256': hashlib.sha256(data).hexdigest() if data is not None else None}
    save()
    assert originals[folder + 'SEE64YOU.CFG'] is not None, 'Canonical config required; do not trigger config migration'

    def check_config():
        assert fetch(folder + 'SEE64YOU.CFG') == originals[folder + 'SEE64YOU.CFG'], 'Config changed'

    def clean_journal(label):
        records = [decode_journal(data) for data in
                   (fetch(folder + 'S64JRN0.DAT'), fetch(folder + 'S64JRN1.DAT')) if data is not None]
        assert records and max(records, key=lambda r: r['sequence'])['state'] == 2, label
        assert turbo() == 0, 'Captured turbo was not restored'
        assert mem(0x314, 2) == irq and api('drives') == drives
        check_config()
        state['tests'].append(label)
        save()

    def launch(name, label):
        assert not state['active']
        entry = re.search(b'\x9e([0-9]+)\x00', candidate[name][2:34])
        assert entry, 'PRG has no BASIC SYS entry'
        phase(label, True)  # Before load: failure must not restore files under a running program.
        state['program'] = name
        save()
        api('runners:load_prg', 'PUT', file=folder + name)
        time.sleep(.5)
        keys(b'SYS' + entry.group(1) + b'\r')

    try:
        phase('stage-candidate')
        for name, data in candidate.items():
            put(folder + name, data)
        for path in paths:
            if path.endswith(('S64JRN0.DAT', 'S64JRN1.DAT', 'S64OLD0.DAT', 'S64OLD1.DAT',
                              'SEE64YOU.REC', 'SEE64YOU.REC.tmp', 'S6F00.S64', 'S6F00.S64.tmp')):
                delete(path)
        seed = journal(1, 2)
        put('/sd/S64JRN0.DAT', seed)
        launch('SEE64YOU.PRG', 'launcher-root-block')
        wait('JOURNAL BLOCKED - RUN S64CFG.PRG', 'root-blocked')
        assert fetch('/sd/S64JRN0.DAT') == seed
        keys(b'X')
        basic('blocked-exit')
        state['tests'].append('launcher-blocks-legacy-active')
        assert turbo(15) == 15
        launch('S64CFG.PRG', 'settings-migration')
        wait('M MOVE SESSION FILES', 'settings-start')
        keys(b'M')
        wait('CONTINUE? Y/N', 'migration-confirm')
        phase('settings-migration-confirmed', True)
        keys(b'Y')
        wait('SESSION FILES MOVED AND VERIFIED', 'migration-success')
        assert fetch('/sd/S64JRN0.DAT') is None and fetch('/sd/S64JRN1.DAT') is None
        # ACTIVE original is superseded in root before backup/retirement.
        old0 = fetch(folder + 'S64OLD0.DAT')
        old1 = fetch(folder + 'S64OLD1.DAT')
        assert old0 == seed
        assert decode_journal(old1)['state'] == 2
        assert decode_journal(old1)['sequence'] == 3
        keys(b'\r')
        wait('M MOVE SESSION FILES', 'migration-return')
        keys(b'\x03')
        basic('settings-exit')
        clean_journal('settings-migration-restores-and-retires')
        fixture = source('Folder fixture', 'Folder hardware source.')
        root_fixture = source('Root fixture', 'Legacy root source differs.')
        put(folder + 'SEE64YOU.REC', fixture)
        put('/sd/S6F00.S64', root_fixture)
        launch('SEE64YOU.PRG', 'editor-recovery')
        wait('Recover autosave? Y/N', 'editor-recovery-prompt')
        keys(b'Y')
        wait('Recovered autosave', 'editor-recovered')
        keys(b'!')
        phase('editor-dirty-exit', True)
        keys(b'\x03')
        wait('S SAVE  D DISCARD', 'editor-save-choice')
        keys(b'S')
        wait('Save as:', 'editor-save-as')
        keys(native('S6F00.S64') + b'\r')
        basic('editor-exit')
        saved = fetch(folder + 'S6F00.S64')
        assert saved is not None
        head, body = saved.split(b'\n\n', 1)
        assert body == b'!Folder hardware source.', body
        for line in (b'Type: post', b'Slug: folder-fixture', b'Title: Folder fixture', b'Status: published'):
            assert line in head.splitlines(), line
        assert fetch('/sd/S6F00.S64') == root_fixture
        assert fetch(folder + 'S6F00.S64.tmp') is None
        clean_journal('editor-folder-save-root-collision-preserved')
        launch('S64PUB.PRG', 'publisher-folder-open')
        wait('L LOAD .S64', 'publisher-start')
        keys(b'L')
        wait('OPEN .S64:', 'publisher-folder-prompt')
        keys(native('S6F00.S64') + b'\r')
        text = wait('Local .S64 loaded', 'publisher-folder-loaded')
        assert 'FOLDER FIXTURE' in text.upper(), text
        keys(b'\x03')
        basic('publisher-exit')
        assert fetch(folder + 'S6F00.S64') == saved and fetch('/sd/S6F00.S64') == root_fixture
        check_config()
        state['tests'].append('fresh-publisher-folder-load')
        save()
        launch('S64PUB.PRG', 'publisher-explicit-root-open')
        wait('L LOAD .S64', 'publisher-root-start')
        keys(b'L')
        wait('OPEN .S64:', 'publisher-root-prompt')
        keys(native('/sd/S6F00.S64') + b'\r')
        text = wait('Local .S64 loaded', 'publisher-root-loaded')
        assert 'ROOT FIXTURE' in text.upper(), text
        keys(b'\x03')
        basic('publisher-root-exit')
        assert fetch(folder + 'S6F00.S64') == saved and fetch('/sd/S6F00.S64') == root_fixture
        clean_journal('publisher-explicit-root-access-no-write')
    except BaseException as error:
        state['error'] = repr(error)
        save()
        raise
    finally:
        if state['active']:
            try:
                keys(b'\x03')
                time.sleep(1)
                text = screen('failure-clean-stop')
                if 'READY.' in text and mem(0xc6, 1) == b'\0':
                    basic('failure-basic')
            except Exception as error:
                state['clean_exit_error'] = repr(error)
                save()
        if not state['active']:
            try:
                phase('restoring-originals', False)
                for path, data in originals.items():
                    if fetch(path) == data:
                        continue
                    elif data is None:
                        delete(path)
                    else:
                        put(path, data)
                assert turbo(prior) == prior
                basic('restored-basic')
                assert mem(0x314, 2) == irq and api('drives') == drives
                assert all(fetch(path) == data for path, data in originals.items())
                keys(b'NEW\r')
                wait('READY.', 'restored-empty-basic')
                assert mem(int.from_bytes(mem(0x2b, 2), 'little'), 2) == b'\0\0'
                state['restored'] = True
                phase('finished', False)
            except Exception as error:
                state['restore_error'] = repr(error)
                save()
        else:
            state['restoration_blocked'] = 'Program active; no reset and no original-file overwrite. Backups retained.'
            save()
        ftp.quit()
    assert state['restored'], state
    print(json.dumps(state, indent=2))


if __name__ == '__main__':
    main()
