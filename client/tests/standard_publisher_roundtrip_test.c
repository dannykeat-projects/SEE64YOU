#include "buffer.h"
#include "document.h"
#include "storage/local.h"
#include "platform/dos.h"
#include "platform/uci.h"
#include "publisher/standard_transport.h"
#include "publisher/standard_publish.h"
#include "publisher/standard_publish_names.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Reproduction companion for the P1 upload report: open real 8TML files with
   the Publisher loader, then put them through the real transport and
   document wire writer.  The mock server validates the pinned API 1 draft
   rules (body = declared size + 64 lowercase hex signature digits,
   Content-Length = size + 64).  Expected signatures are computed
   independently with Python (see todo.md evidence trail), so drift across
   64-byte codec chunk boundaries would also be caught. */

static const char key[]="000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
static const char challenge0[]="00112233445566778899aabbccddeeff";
static const char challenge1[]="112233445566778899aabbccddeeff00";
static const char* filename;
static const unsigned char* file_bytes;
static unsigned int file_size, file_offset;
static char request[8192], response[2048];
static unsigned request_length, response_offset, challenge_count;

static const char doc_repro[]="---\n8tml: 1\ntitle: Publisher Repro\n---\n# Publisher repro report\n\nThis paragraph explains what happened on the C64\nUltimate when the upload was attempted. It contains\npunctuation: full stop. comma, colon: semi;\nand a quote \042plus\042 a bracket [see] and ampersand &.\n\nSecond paragraph keeps the document long\nenough to cross several codec chunk boundaries during\nthe live upload.\n\n-- fin\n";
static const char doc_edge[]="---\n8tml: 1\ntitle: Edge\n---\n# Edge case\nRepro padding line for the chunk-boundary check.\nxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
static const char doc_large[]="---\n8tml: 1\ntitle: Large\n---\nParagraph filler 0 keeps this document near the maximum publishable size.\nParagraph filler 1 keeps this document near the maximum publishable size.\nParagraph filler 2 keeps this document near the maximum publishable size.\nParagraph filler 3 keeps this document near the maximum publishable size.\nParagraph filler 4 keeps this document near the maximum publishable size.\nParagraph filler 5 keeps this document near the maximum publishable size.\nParagraph filler 6 keeps this document near the maximum publishable size.\nParagraph filler 7 keeps this document near the maximum publishable size.\nParagraph filler 8 keeps this document near the maximum publishable size.\nParagraph filler 9 keeps this document near the maximum publishable size.\nParagraph filler 10 keeps this document near the maximum publishable size.\nParagraph filler 11 keeps this document near the maximum publishable size.\nParagraph filler 12 keeps this document near the maximum publishable size.\nParagraph filler 13 keeps this document near the maximum publishable size.\nParagraph filler 14 keeps this document near the maximum publishable size.\nParagraph filler 15 keeps this document near the maximum publishable size.\nParagraph filler 16 keeps this document near the maximum publishable size.\nParagraph filler 17 keeps this document near the maximum publishable size.\nParagraph filler 18 keeps this document near the maximum publishable size.\nParagraph filler 19 keeps this document near the maximum publishable size.\n";

static const struct Fixture { const char* name; const char* bytes; unsigned size; const char* sig; } fixtures[]={
 {"repro.8tml",doc_repro,383u,"e1db43cd14199dd007b9667808e17c96cdfe1b4cb72b3c8b064e863a851e45be"},
 {"edge.8tml",doc_edge,129u,"5e533e1a3f82c0b50ecb87fd991107235c098dfc4c9887c6a1a0eaf826a3b3ad"},
 {"large.8tml",doc_large,1519u,"aef266cfa4df547e3c2122fbfa95a58d254389fd3bf57905efc5973826f7bb01"} };
#define FIXTURE_COUNT (sizeof(fixtures)/sizeof(fixtures[0]))

static S64UciResult execute(const S64UciCommandPart* parts,uint8_t count,S64UciReply* reply)
{
 unsigned int length,chunk; const char* status="00,OK";
 (void)count;
 if(parts[0].data[1]==2u) {
  if(strcmp((const char*)parts[1].data,filename)) status="88,FILE NOT FOUND"; else file_offset=0u;
 } else if(parts[0].data[1]==4u) {
  length=(unsigned)parts[0].data[2]|((unsigned)parts[0].data[3]<<8u);
  while(length) { /* One UCI command, many data packets like the real read. */
   chunk=90u; if(chunk>length) chunk=length;
   if(chunk>file_size-file_offset) chunk=file_size-file_offset;
   if(!chunk) break;
   if(!reply->data_sink(file_bytes+file_offset,(uint16_t)chunk,reply->data_context)) return S64_UCI_CONSUMER_ABORTED;
   file_offset+=chunk; length-=chunk;
  }
 }
 strcpy(reply->status,status); reply->status_length=(uint16_t)strlen(status);
 return S64_UCI_OK;
}


static unsigned hex_ok(const char* bytes,unsigned count)
{ unsigned i; for(i=0u;i<count;++i) if(!((bytes[i]>='0'&&bytes[i]<='9')||(bytes[i]>='a'&&bytes[i]<='f'))) return 0u; return 1u; }
static const char* uri_name(const char* r,char* out)
{ const char* p=strstr(r,"&name="); unsigned i=0u; if(!p) return 0; p+=6u;
 while(p[i]&&p[i]!='&'&&i<64u) { out[i]=p[i]; ++i; } out[i]=0; return out; }
static unsigned uri_size(const char* r)
{ const char* p=strstr(r,"&size="); unsigned value=0u; if(!p) return 0xFFFFFFFFu; p+=6u;
 while(*p>='0'&&*p<='9') value=value*10u+(unsigned)(*p++-'0');
 return value; }
static unsigned content_length(const char* r)
{ const char* p=strstr(r,"Content-Length: "); unsigned value=0u; if(!p) return 0xFFFFFFFFu; p+=16u;
 while(*p>='0'&&*p<='9') value=value*10u+(unsigned)(*p++-'0');
 return value; }
static char* body_at(const char* r)
{ const char* p=strstr(r,"\r\n\r\n"),*last=0;
 while(p) { last=p; p=strstr(p+2u,"\r\n\r\n"); }
 return (char*)last; }
static void reply(const char* body)
{ snprintf(response,sizeof(response),"HTTP/1.0 200 OK\r\nContent-Length: %lu\r\n\r\n%s",(unsigned long)strlen(body),body); }
static void reply_body_error(const char* what)
{ char body[160]; snprintf(body,sizeof(body),"status: error\nerror: invalid-body\nmessage: The body must be the content followed by %s\n",what); reply(body); }

S64NetResult s64_net_open_tcp(S64NetTransport* t,const char* host,uint16_t port,uint8_t* handle)
{ (void)t; (void)host; (void)port; request_length=0u; response_offset=0u; request[0]=0; response[0]=0; *handle=1u; return S64_NET_OK; }
S64NetResult s64_net_close(S64NetTransport* t,uint8_t handle)
{ (void)t; (void)handle; return S64_NET_OK; }
S64NetResult s64_net_write(S64NetTransport* t,uint8_t handle,const uint8_t* bytes,uint16_t count)
{ (void)t; (void)handle;
 if(request_length+count>=sizeof(request)) return S64_NET_TRANSPORT_ERROR;
 memcpy(request+request_length,bytes,count); request_length+=count; request[request_length]=0;
 return S64_NET_OK; }
S64NetResult s64_net_write_text(S64NetTransport* t,uint8_t h,const char* text,uint16_t count)
{ return s64_net_write(t,h,(const uint8_t*)text,count); }
static void serve(void)
{ static char name[65]; const char* body; unsigned i,known=FIXTURE_COUNT,size,cl;
 if(strstr(request,"?op=status")) { reply("status: ok\napi: 1\nauth: hmac https\nmax-size: 61440\n"); return; }
 if(strstr(request,"?op=challenge&")) {
  assert(strstr(request,"Content-Length: 0\r\n"));
  assert(challenge_count<2u); /* list and put need one fresh challenge each */
  snprintf(response,sizeof(response),"HTTP/1.0 200 OK\r\n\r\nstatus: ok\nchallenge: %s\nexpires: 120\n",challenge_count?challenge1:challenge0);
  ++challenge_count; return;
 }
 if(strstr(request,"?op=list")) {
  body=body_at(request);
  if(!body) { reply_body_error("the 64 hex digits for an empty list body"); return; }
  body+=4u;
  if(strlen(body)!=64u||!hex_ok(body,64u)) { reply_body_error("the 64 hex digits for an empty list body"); return; }
  reply("status: ok\nfile: 12 other.8tml\n"); return;
 }
 assert(strstr(request,"?op=put&"));
 body=body_at(request); assert(body); body+=4u;
 size=uri_size(request); cl=content_length(request);
 if(uri_name(request,name)) for(i=0u;i<FIXTURE_COUNT;++i) if(!strcmp(name,fixtures[i].name)) { known=i; break; }
 if(known>=FIXTURE_COUNT) reply_body_error("a known remote name");
 else if(size!=fixtures[known].size||cl!=fixtures[known].size+64u) reply_body_error("the declared source length");
 else if((unsigned)strlen(body)!=cl) reply_body_error("the source bytes plus the 64 hex signature digits");
 else if(!hex_ok(body+fixtures[known].size,64u)) reply_body_error("the 64 lowercase hex signature digits");
 else if(strncmp(body+fixtures[known].size,fixtures[known].sig,64u)) reply_body_error("the signature for the bytes actually sent");
 else { char ok[192]; snprintf(ok,sizeof(ok),"status: ok\nname: %s\nsize: %u\nurl: http://example.test/%s\n",name,size,name); reply(ok); }
}
S64NetResult s64_net_read(S64NetTransport* t,uint8_t handle,uint16_t maximum,S64UciDataSink sink,void* context,uint16_t* received)
{ unsigned amount; (void)t; (void)handle; (void)maximum;
 if(!response[0]) { serve(); *received=0u; return S64_NET_NO_DATA; } /* Process the request when reading starts. */
 amount=(unsigned)strlen(response)-response_offset; if(amount>7u) amount=7u; /* Fragment replies like the real UCI read. */
 if(!amount) return S64_NET_CLOSED;
 *received=(uint16_t)amount;
 if(!sink((const uint8_t*)response+response_offset,(uint16_t)amount,context)) return S64_NET_TRANSPORT_ERROR;
 response_offset+=amount; return S64_NET_OK;
}

/* Same writer the Publisher installs in main: the literal document wire path. */
static int write_source(S64UciDataSink sink,void* context,void* source)
{ return s64_document_write_wire((S64Document*)source,sink,context)==S64_DOCUMENT_OK; }

int main(void)
{
 S64Dos dos; S64Buffer body; S64Document document; S64LocalDocument local;
 S64StandardClient client; S64StandardReply result;
 char check[65]; uint32_t declared; unsigned i;
 memset(&client,0,sizeof(client)); client.network.execute=execute;
 client.url="http://example.test/publish"; client.id="test-1"; client.key=key; client.allow_http=1u;
 s64_buffer_init(&body); s64_document_init(&document,&body);
 s64_dos_init(&dos,execute,S64_DOS_TARGET_PRIMARY);
 s64_local_init(&local,&dos,&document);
 for(i=0u;i<FIXTURE_COUNT;++i) {
  assert((unsigned)strlen(fixtures[i].bytes)==fixtures[i].size);
  filename=(char*)fixtures[i].name; file_bytes=(const unsigned char*)fixtures[i].bytes;
  file_size=fixtures[i].size; file_offset=0u; challenge_count=0u;
  assert(s64_local_open(&local,filename,1u)==S64_LOCAL_OK);
  assert(local.document->kind==S64_DOCUMENT_KIND_8TML);
  declared=(uint32_t)s64_buffer_length(&body);
  assert(declared==fixtures[i].size); /* Declared size equals disk bytes for LF-only files. */
  assert(s64_standard_status(&client,&result)==S64_STANDARD_OK);
  assert(s64_standard_list(&client,filename,&result)==S64_STANDARD_OK);
  assert(s64_standard_put(&client,filename,declared,write_source,&document,&result)==S64_STANDARD_OK);
  assert(result.size==fixtures[i].size); assert(!result.warning); assert(!strcmp(result.name,fixtures[i].name));
  assert(challenge_count==2u);
  { const char* wire=strstr(request,"\r\n\r\n");
    assert(strstr(request,"?op=put&"));
    assert(uri_size(request)==fixtures[i].size);
    assert(uri_name(request,check)&&!strcmp(check,fixtures[i].name));
    assert(content_length(request)==fixtures[i].size+64u);
    assert(wire); wire+=4u;
    assert((unsigned)strlen(wire)==fixtures[i].size+64u);
    assert(!memcmp(wire,fixtures[i].bytes,fixtures[i].size));
    assert(!strcmp(wire+fixtures[i].size,fixtures[i].sig));
  }
 }
 puts("standard publisher put round-trip fixtures passed");
 return 0;
}
