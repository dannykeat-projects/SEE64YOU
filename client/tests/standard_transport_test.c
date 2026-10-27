#include "publisher/standard_transport.h"
#include "publisher/standard_publish.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static char request[4096], response[8192], challenges[2][33];
static unsigned request_length, response_offset, opens, closes, writes, put_count;
static int scenario, challenge_count;
static const char key[]="000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f";
static const uint8_t source[]="---\n8tml: 1\n---\n# Hello\n";
static S64UciResult execute(const S64UciCommandPart* parts,uint8_t count,S64UciReply* reply)
{ (void)parts; (void)count; (void)reply; return S64_UCI_OK; }
S64NetResult s64_net_open_tcp(S64NetTransport* t,const char* host,uint16_t port,uint8_t* handle)
{
 (void)t; assert(!strcmp(host,"example.test")); assert(port==8080u);
 ++opens; request_length=response_offset=0u; request[0]=response[0]=0; *handle=1u; return S64_NET_OK;
}
S64NetResult s64_net_close(S64NetTransport* t,uint8_t handle)
{ (void)t; assert(handle==1u); ++closes; return scenario==12 || (scenario==13 && strstr(request,"?op=put&")) ? S64_NET_TARGET_ERROR : S64_NET_OK; }
S64NetResult s64_net_write(S64NetTransport* t,uint8_t handle,const uint8_t* bytes,uint16_t count)
{
 (void)t; assert(handle==1u); ++writes;
 if (strstr(request,"?op=put&") && strstr(request,"\r\n\r\n")) {
  if (scenario==16 && count==7u) return S64_NET_TRANSPORT_ERROR;
  if (scenario==17 && count==64u) return S64_NET_TRANSPORT_ERROR;
 }
 assert(request_length+count<sizeof(request)); memcpy(request+request_length,bytes,count);
 request_length+=count; request[request_length]=0; return S64_NET_OK;
}
S64NetResult s64_net_write_text(S64NetTransport* t,uint8_t h,const char* text,uint16_t count)
{ return s64_net_write(t,h,(const uint8_t*)text,count); }
static void reply(const char* body,unsigned status)
{
 snprintf(response,sizeof(response),"HTTP/1.0 %u Test\r\nContent-Length: %lu\r\n\r\n%s",status,(unsigned long)strlen(body),body);
}
static void signed_request(const char* op,const char* name,unsigned size)
{
 S64PublishSigner s; uint8_t bytes[32]; char signature[65]; const char* body; unsigned i;
 for(i=0u;i<32u;++i) bytes[i]=(uint8_t)i;
 body=strstr(request,"\r\n\r\n"); assert(body); body+=4;
 assert(s64_publish_sign_begin(&s,bytes,op,"test-1",challenges[challenge_count-1],name,size));
 assert(s64_publish_sign_update(&s,(const uint8_t*)body,(uint16_t)size));
 assert(s64_publish_sign_finish(&s,signature)); assert(!strcmp(body+size,signature));
 assert(strlen(body)==size+64u);
 if (challenge_count == 1) assert(!strcmp(signature, size ? "f61702e908e31462519ef7a1a89b5fa9b723524deb5e7d20806dbc5a257e62db" : "91b810d221b4176e501565e1fb910f1cba860000a74e6fc5d6db218d6166d512"));
}
S64NetResult s64_net_read(S64NetTransport* t,uint8_t handle,uint16_t maximum,S64UciDataSink sink,void* context,uint16_t* received)
{
 unsigned amount, i;
 (void)t; assert(handle==1u); (void)maximum;
 if(!response[0]) {
  assert(strstr(request,"Host: example.test:8080\r\n"));
  if(strstr(request,"?op=status ")) {
   assert(!strncmp(request,"GET ",4u));
   if(scenario==1) reply("status: ok\napi: 1\nauth: https\nmax-size: 61440\n",200u);
   else if(scenario==2) reply("status: ok\napi: 1\nauth: hmac https\nmax-size: 1\n",200u);
   else if(scenario==3) reply("status: ok\napi: 1\napi: 1\nauth: hmac\nmax-size: 61440\n",200u);
   else if(scenario==8 || scenario==10) snprintf(response,sizeof(response),"HTTP/1.0 200 OK\r\n\r\nstatus: ok\napi: 1\nauth: hmac\nmax-size: 61440%s",scenario==10?"":"\n");
   else if(scenario==9) snprintf(response,sizeof(response),"HTTP/1.0 200 OK\r\nContent-Length: 100\r\n\r\nstatus: ok\napi: 1\nauth: hmac\nmax-size: 61440\n");
   else reply("status: ok\napi: 1\nauth: hmac https\nmax-size: 61440\n",200u);
  } else if(strstr(request,"?op=challenge&")) {
   assert(strstr(request,"Content-Length: 0\r\n"));
   assert(challenge_count<2);
   strcpy(challenges[challenge_count],challenge_count?"112233445566778899aabbccddeeff00":"00112233445566778899aabbccddeeff");
   snprintf(response,sizeof(response),"HTTP/1.0 200 OK\r\n\r\nstatus: ok\nchallenge: %s\n",challenges[challenge_count++]);
  } else if(strstr(request,"?op=list&")) {
   signed_request("list","",0u);
   if (scenario==14) {
    strcpy(response,"HTTP/1.0 200 OK\r\n\r\nstatus: ok\n");
    for (i=0u;i<200u;++i) strcat(response,"file: 10 other.8tml\n");
    strcat(response,"file: 24 index.8tml\n");
   } else reply("status: ok\nfile: 12 other.8tml\nfile: 24 index.8tml\n",200u);
  } else {
   assert(strstr(request,"?op=put&")); ++put_count;
   assert(strstr(request,"&name=index.8tml&size=24&c="));
   assert(strstr(request,"Content-Length: 88\r\n"));
   signed_request("put","index.8tml",24u);
   if(scenario==4) return S64_NET_TRANSPORT_ERROR;
   if(scenario==5) reply("status: error\nerror: quota\nmessage: Storage full\n",403u);
   else if(scenario==6) reply("status: ok\nname: index.8tml\nsize: 25\nurl: http://example.test/index.8tml\nwarning: Converted\n",200u);
   else if(scenario==18) reply("status: ok\nname: index.8tml\nsize: 24\nurl: http://example.test/@keato/index.8tml\n",200u);
   else if(scenario==15) reply("status: ok\nname: index.8tml\nsize: 24\nurl: http://user@example.test/index.8tml\n",200u);
   else if(scenario==7) reply("status: ok\nname: index.8tml\nsize: 24\n",200u);
   else reply("status: ok\nname: index.8tml\nsize: 24\nurl: http://example.test/index.8tml\n",200u);
  }
 }
 if(scenario==11) { *received=0u; return S64_NET_NO_DATA; }
 amount=(unsigned)strlen(response)-response_offset;
 if(amount>7u) amount=7u; /* Fragment headers and body across arbitrary bytes. */
 *received=(uint16_t)amount;
 if(!amount) return S64_NET_CLOSED;
 if (!sink((const uint8_t*)response+response_offset,(uint16_t)amount,context)) return S64_NET_TRANSPORT_ERROR;
 response_offset+=amount; return S64_NET_OK;
}
static int writer(S64UciDataSink sink,void* sink_context,void* context)
{
 unsigned* count=(unsigned*)context; ++*count;
 return sink(source,7u,sink_context) && sink(source+7u,sizeof(source)-1u-7u,sink_context);
}
static void reset(int value)
{ scenario=value; opens=closes=writes=put_count=0u; challenge_count=0; }
int main(void)
{
 S64StandardClient client; S64StandardReply result; unsigned count;
 memset(&client,0,sizeof(client)); client.network.execute=execute;
 client.url="http://example.test:8080/publish"; client.id="test-1"; client.key=key; client.allow_http=1u;
 reset(0); assert(s64_standard_status(&client,&result)==S64_STANDARD_OK); assert(result.max_size==61440u); assert(opens==closes);
 reset(1); assert(s64_standard_status(&client,&result)==S64_STANDARD_PROTOCOL); assert(opens==closes);
 reset(3); assert(s64_standard_status(&client,&result)==S64_STANDARD_PROTOCOL); assert(opens==closes);
 reset(0); assert(s64_standard_list(&client,"INDEX.8tml",&result)==S64_STANDARD_OK); assert(result.exists); assert(challenge_count==1); assert(opens==closes);
 reset(2); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_TOO_LARGE); assert(!count && !put_count); assert(opens==closes);
 reset(0); count=0u; assert(s64_standard_put(&client,"INDEX.8tml",24u,writer,&count,&result)==S64_STANDARD_OK); assert(count==1u && put_count==1u); assert(!strcmp(result.name,"index.8tml")); assert(!result.warning); assert(opens==closes);
 reset(0); count=0u; assert(s64_standard_list(&client,"index.8tml",&result)==S64_STANDARD_OK); assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_OK); assert(challenge_count==2 && count==1u && put_count==1u); assert(opens==closes);
 reset(4); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_UNCERTAIN); assert(put_count==1u && count==1u); assert(opens==closes);
 reset(5); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_SERVER); assert(!strcmp(result.message,"Storage full")); assert(opens==closes);
 reset(6); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_OK); assert(result.warning && result.size==25u && !strcmp(result.message,"Converted"));
 reset(7); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_UNCERTAIN); assert(opens==closes);
 reset(8); assert(s64_standard_status(&client,&result)==S64_STANDARD_OK); assert(opens==closes);
 reset(9); assert(s64_standard_status(&client,&result)==S64_STANDARD_PROTOCOL); assert(opens==closes);
 reset(10); assert(s64_standard_status(&client,&result)==S64_STANDARD_PROTOCOL); assert(opens==closes);
 reset(11); assert(s64_standard_status(&client,&result)==S64_STANDARD_PROTOCOL); assert(opens==closes);
 reset(12); assert(s64_standard_status(&client,&result)==S64_STANDARD_NETWORK); assert(opens==closes);
 reset(13); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_OK); assert(result.warning && !strcmp(result.message,"Published, socket close failed")); assert(opens==closes);
 reset(14); assert(s64_standard_list(&client,"index.8tml",&result)==S64_STANDARD_OK); assert(result.exists); assert(opens==closes);
 reset(15); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_UNCERTAIN); assert(opens==closes);
 reset(18); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_OK); assert(!strcmp(result.url,"http://example.test/@keato/index.8tml")); assert(opens==closes);
 reset(16); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_NETWORK); assert(count==1u && !put_count); assert(opens==closes);
 reset(17); count=0u; assert(s64_standard_put(&client,"index.8tml",24u,writer,&count,&result)==S64_STANDARD_UNCERTAIN); assert(count==1u && !put_count); assert(opens==closes);
 reset(0); client.url="http://user@example.test:8080/publish"; assert(s64_standard_status(&client,&result)==S64_STANDARD_INVALID); assert(!opens);
 client.url="http://example.test:65536/publish"; assert(s64_standard_status(&client,&result)==S64_STANDARD_INVALID); assert(!opens);
 client.url="http://example.test:8080/pub\\lish"; assert(s64_standard_status(&client,&result)==S64_STANDARD_INVALID); assert(!opens);
 client.url="http://example.test:8080/publish?op=put"; assert(s64_standard_status(&client,&result)==S64_STANDARD_INVALID); assert(!opens);
 client.url="http://example.test:8080/publish";
 reset(0); client.allow_http=0u; assert(s64_standard_status(&client,&result)==S64_STANDARD_CONSENT); assert(!opens);
 client.url="https://example.test/publish"; assert(s64_standard_status(&client,&result)==S64_STANDARD_TLS); assert(!opens);
 puts("standard transport tests passed"); return 0;
}
