#!/usr/bin/env python3
"""Measure source-reader REU stores and verify exact source; excludes DOS/UI latency."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
HARNESS = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "document.h"
#include "platform/reu.h"
static uint8_t reu[65536], source[S64_BUFFER_CAPACITY], raw[S64_BUFFER_CAPACITY * 2];
static unsigned long calls, stored, fail_at;
static unsigned int offset, size;
static S64Buffer body;
static S64Document document;
static S64DocumentWireReader reader;
uint8_t s64_reu_store(uint8_t bank, uint16_t address, const void* data, uint16_t length) {
 assert(bank == S64_REU_DOCUMENT_BANK && (unsigned long)address + length <= sizeof(reu));
 ++calls; if (fail_at && calls == fail_at) return 0;
 stored += length; memcpy(reu + address, data, length); return 1;
}
uint8_t s64_reu_load(uint8_t bank, uint16_t address, void* data, uint16_t length) {
 assert(bank == S64_REU_DOCUMENT_BANK && (unsigned long)address + length <= sizeof(reu));
 memcpy(data, reu + address, length); return 1;
}
static uint8_t collect(const uint8_t* data, uint16_t length, void* unused) {
 (void)unused; assert(offset + length <= size);
 assert(!memcmp(source + offset, data, length)); offset += length; return 1;
}
int main(void) {
 static const unsigned int sizes[] = {1024,16384,S64_BUFFER_CAPACITY};
 static const char header[] = "---\n8tml: 1\nX-future: preserved\n---\n";
 unsigned int index, pattern, i, n, position, chunk;
 unsigned long read_calls, read_stored;
 for (index=0; index<3; ++index) for (pattern=0; pattern<3; ++pattern) {
  size=sizes[index]; memcpy(source,header,sizeof(header)-1);
  for (i=sizeof(header)-1; i<size; ++i) source[i]=(pattern!=1 && i%40==39)?10:'x';
  n=0; for (i=0;i<size;++i) { if (pattern==2 && source[i]==10) raw[n++]=13; raw[n++]=source[i]; }
  s64_buffer_init(&body); s64_document_init(&document,&body);
  s64_document_wire_reader_init(&reader,&document); calls=stored=0;
  for (position=0;position<n;position+=chunk) {
   chunk=n-position>512?512:n-position;
   assert(s64_document_wire_reader_write(raw+position,chunk,&reader));
  }
  read_calls=calls; read_stored=stored;
  assert(read_calls <= (size+63u)/64u + (n+511u)/512u);
  assert(s64_document_wire_reader_finish(&reader)==S64_DOCUMENT_OK);
  assert(s64_buffer_length(&body)==size);
  offset=0; assert(s64_document_write_wire(&document,collect,0)==S64_DOCUMENT_OK && offset==size);
  printf("%u,%u,%lu,%lu\n",size,pattern,read_calls,read_stored);
 }
 /* A failed REU write is an error, with no partially accepted source on finish. */
 s64_document_wire_reader_init(&reader,&document); calls=stored=0; fail_at=2;
 assert(!s64_document_wire_reader_write(raw,512,&reader));
 assert(s64_document_wire_reader_finish(&reader)==S64_DOCUMENT_SINK_ERROR);
 assert(!s64_buffer_length(&body)); fail_at=0;
 /* Final partial-chunk failure retains only the earlier complete logical chunk. */
 s64_document_wire_reader_init(&reader,&document); calls=stored=0; fail_at=2;
 assert(!s64_document_wire_reader_write(source,65,&reader));
 assert(s64_buffer_length(&body)==64);
 assert(s64_document_wire_reader_finish(&reader)==S64_DOCUMENT_SINK_ERROR);
 assert(!s64_buffer_length(&body)); fail_at=0;
 /* Exact capacity succeeds; one more byte fails and finish clears partial input. */
 s64_document_wire_reader_init(&reader,&document);
 assert(s64_document_wire_reader_write(source,S64_BUFFER_CAPACITY,&reader));
 assert(!s64_document_wire_reader_write(source,1,&reader));
 assert(s64_document_wire_reader_finish(&reader)==S64_DOCUMENT_SINK_ERROR);
 assert(!s64_buffer_length(&body));
 /* Append is bounded to an EOF gap and never advances on a failed store. */
 assert(s64_buffer_append(&body,source,64));
 assert(s64_buffer_move_cursor(&body,32));
 assert(!s64_buffer_append(&body,source,1));
 assert(s64_buffer_length(&body)==64 && s64_buffer_cursor(&body)==32);
 assert(s64_buffer_move_cursor(&body,64)); calls=0; fail_at=1;
 assert(!s64_buffer_append(&body,source,1));
 assert(s64_buffer_length(&body)==64 && s64_buffer_cursor(&body)==64); fail_at=0;
 assert(s64_buffer_append(&body,0,0));
 assert(!s64_buffer_append(&body,0,1));
 assert(!s64_buffer_append(&body,source,S64_BUFFER_CAPACITY));
 return 0;
}
'''

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--output',type=Path,required=True)
 args=parser.parse_args()
 sources=['client/src/buffer_reu.c','client/src/document.c','client/src/charset.c']
 with tempfile.TemporaryDirectory(prefix='s64-source-load-') as temp:
  c=Path(temp)/'probe.c'; binary=Path(temp)/'probe'; c.write_text(HARNESS)
  subprocess.run(['cc','-std=c99','-Wall','-Wextra','-Werror','-DS64_REU_BUFFER=1',
   '-DS64_INDEPENDENT_EDITOR=1','-Iclient/src',str(c),*sources,'-o',str(binary)],cwd=ROOT,check=True)
  result=subprocess.run([str(binary)],cwd=ROOT,text=True,capture_output=True,check=True)
 rows=[]
 for line in result.stdout.splitlines():
  size,pattern,calls,stored=map(int,line.split(','))
  rows.append(dict(bytes=size,pattern=['LF short lines','LF long line','CRLF short lines'][pattern],store_calls=calls,stored_bytes=stored))
 report={'scope':'Production source reader host REU counters; excludes DOS, overlays, UI and C64 latency',
  'source_sha256':{p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in sources},'rows':rows,
  'correctness':'Exact normalized roundtrip at 1/16/60 KiB; injected REU write failure clears partial source'}
 args.output.parent.mkdir(parents=True,exist_ok=True); args.output.write_text(json.dumps(report,indent=2)+'\n')
 print(result.stdout,end=''); print('Passed exact source and injected failure checks')
if __name__=='__main__': main()
