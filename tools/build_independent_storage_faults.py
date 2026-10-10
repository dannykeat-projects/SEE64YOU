#!/usr/bin/env python3
"""Build the bounded real-SD storage fault diagnostic; not a product PRG."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
client = root / 'client'
sources = ['diagnostics/independent_storage_faults.c', 'src/buffer_reu.c',
           'src/document.c', 'src/charset.c', 'src/platform/dos.c',
           'src/platform/uci.c', 'src/platform/reu.c', 'src/platform/reu_dma.s',
           'src/storage/local.c', 'src/publisher/settings_exchange.c',
           'src/publisher/settings_validate.c', 'src/publisher/settings_store.c',
           'src/platform/basic_workspace_reset.s']
out = client / 'build/independent-storage-faults.prg'
out.parent.mkdir(exist_ok=True)
subprocess.run(['cl65', '-t', 'c64', '-Os', '-I', str(client / 'src'),
                '-DS64_NATIVE_PETSCII=1', '-DS64_REU_BUFFER=1', '-o', str(out),
                *[str(client / source) for source in sources]], check=True)
print(out)
