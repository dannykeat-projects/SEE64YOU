/* Disposable standard-network diagnostic. Never opens or writes local files. */
#include <conio.h>
#include <stdint.h>
#include <string.h>
#include "platform/uci.h"
#include "platform/dos_path.h"
#include "publisher/standard_transport.h"
#ifndef S64_STANDARD_TEST_URL
#define S64_STANDARD_TEST_URL "http://192.168.1.111:8767/publish"
#endif
#define PROGRESS (*(volatile uint8_t*)0xc060)
#define RESULT (*(volatile uint8_t*)0xc061)
#define DETAIL (*(volatile uint8_t*)0xc062)

static S64StandardClient client;
static S64StandardReply reply;
static S64Dos dos;
static char original_paths[2][441], current_path[441];
/* Public disposable fixture credential: never use for an actual site. */
static const char key[] = "000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
static const uint8_t source[2][24] = {
 {45,45,45,10,56,116,109,108,58,32,49,10,45,45,45,10,35,32,72,101,108,108,111,10},
 {45,45,45,10,56,116,109,108,58,32,49,10,45,45,45,10,35,32,65,103,97,105,110,10}
};
static uint8_t version;
static int write_source(S64UciDataSink sink, void* sink_context, void* context)
{
 (void)context;
 return sink(source[version],24u,sink_context);
}
static uint8_t exercise(void)
{
 S64StandardResult result;
 PROGRESS=1u;
 result=s64_standard_status(&client,&reply);
 if(result!=S64_STANDARD_OK) return (uint8_t)result;
 if(reply.max_size<24u) return 0x81u;
 PROGRESS=2u;
 result=s64_standard_list(&client,"codex-test.8tml",&reply);
 if(result!=S64_STANDARD_OK) return (uint8_t)result;
 if(reply.exists) return 0x82u;
 PROGRESS=3u;
 version=0u;
 result=s64_standard_put(&client,"codex-test.8tml",24u,write_source,0,&reply);
 if(result!=S64_STANDARD_OK) return (uint8_t)result;
 if(reply.size!=24u || !reply.url[0] || reply.warning) return 0x83u;
 PROGRESS=4u;
 result=s64_standard_list(&client,"codex-test.8tml",&reply);
 if(result!=S64_STANDARD_OK) return (uint8_t)result;
 if(!reply.exists) return 0x84u;
 PROGRESS=5u;
 version=1u;
 result=s64_standard_put(&client,"codex-test.8tml",24u,write_source,0,&reply);
 if(result!=S64_STANDARD_OK) return (uint8_t)result;
 if(reply.size!=24u || !reply.url[0] || reply.warning) return 0x85u;
 PROGRESS=6u;
 result=s64_standard_put(&client,"fixture-denied.8tml",24u,write_source,0,&reply);
 if(result!=S64_STANDARD_SERVER) return 0x86u;
 return 0u;
}
int main(void)
{
 uint8_t i,detail=0u;
 unsigned char background,border,text;
 background=bgcolor(COLOR_BLUE); border=bordercolor(COLOR_BLUE);
 text=textcolor(COLOR_LIGHTBLUE);
 PROGRESS=RESULT=DETAIL=0u;
 clrscr(); cputs("STANDARD NETWORK FIXTURE\r\nNO LOCAL FILE WRITES\r\n");
 s64_net_init(&client.network,s64_uci_execute);
 client.url=S64_STANDARD_TEST_URL; client.id="test-1";
 client.key=key; client.allow_http=1u;
 PROGRESS=0x10u;
 for(i=0u;i<2u;++i) {
  s64_dos_init(&dos,s64_uci_execute,(uint8_t)(i+1u));
  if(s64_dos_get_path(&dos,original_paths[i],sizeof(original_paths[i]))!=S64_DOS_OK) {
   detail=(uint8_t)(0x90u+i); break;
  }
 }
 if(!detail) detail=exercise();
 /* GET_PATH is read-only. Verify both target contexts even after failures. */
 for(i=0u;i<2u;++i) {
  if(!original_paths[i][0]) continue;
  s64_dos_init(&dos,s64_uci_execute,(uint8_t)(i+1u));
  if(s64_dos_get_path(&dos,current_path,sizeof(current_path))!=S64_DOS_OK
   || strcmp(original_paths[i],current_path)) {
   PROGRESS=7u; detail=(uint8_t)(0xa0u+i);
  }
 }
 DETAIL=detail; RESULT=detail?0x5au:0xa5u;
 if(!detail) { PROGRESS=7u; cputs("STANDARD NETWORK PASSED\r\n"); }
 else cprintf("STANDARD NETWORK FAILED\r\nSTEP %u DETAIL %u\r\n",PROGRESS,DETAIL);
 cputs("RUN/STOP RETURNS TO BASIC");
 while(cgetc()!=CH_STOP) {}
 bgcolor(background); bordercolor(border); textcolor(text);
 cursor(1); return 0;
}
