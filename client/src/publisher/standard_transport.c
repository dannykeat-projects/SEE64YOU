#include "publisher/standard_transport.h"
#include "publisher/standard_request.h"
#include "publisher/standard_reply.h"
#include "publisher/standard_publish.h"
#include "publisher/direct_auth.h"
#include "charset.h"
#include <string.h>

/* Private scratch for a single synchronous operation, never source memory. */
static char host[96], endpoint[193], uri[384], header[256];
static char challenge[33], signature[65], target[65];
static uint8_t raw_key[32];
static S64PublishSigner signer;
static S64PublishReplyReader reader;
static S64StandardClient* active;
static S64StandardReply* output;
static S64PublishRequestKind operation;
static uint8_t socket_handle, fields, headers_done, have_status, have_length;
static uint8_t failed, body_sent;
static uint16_t port, header_length, header_total, http_status;
static uint32_t remaining;

static int decimal(const char* text, uint32_t* value)
{
 uint32_t result=0u;
 if (!*text) return 0;
 while (*text) {
  if (*text<'0' || *text>'9' || result>429496729u
   || (result==429496729u && *text>'5')) return 0;
  result=result*10u+(uint8_t)(*text++-'0');
 }
 *value=result; return 1;
}
static void number(uint32_t value)
{
 char digits[10]; uint8_t count=0u,i=0u;
 do { digits[count++]=(char)('0'+value%10u); value/=10u; } while(value);
 while(count) header[i++]=digits[--count];
 header[i]=0;
}
static int copy(char* dest,uint16_t capacity,const char* source)
{
 if(strlen(source)>=capacity) return 0;
 strcpy(dest,source); return 1;
}
static void lowercase(char* text)
{
 while(*text) { if(*text>='A' && *text<='Z') *text+='a'-'A'; ++text; }
}
static int equal_ci(const char* a,const char* b,uint8_t count)
{
 char x,y;
 while(count--) {
  x=*a++; y=*b++; if(!x || !y) return 0;
  if(x>='A' && x<='Z') x+='a'-'A';
  if(y>='A' && y<='Z') y+='a'-'A';
  if(x!=y) return 0;
 }
 return 1;
}
static int url_valid(const char* url, uint8_t public_url)
{
 const char* cursor;
 uint16_t length=0u, host_length=0u;
 uint32_t value=0u;
 char ch;
 if (!url) return 0;
 while (url[length]) if (++length>=193u) return 0;
 if (equal_ci(url,"http://",7u)) cursor=url+7;
 else if (equal_ci(url,"https://",8u)) cursor=url+8;
 else return 0;
 while (*cursor && *cursor!='/' && *cursor!=':') {
  ch=*cursor++;
  if (!((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
   || (ch>='0' && ch<='9') || ch=='.' || ch=='-')) return 0;
  ++host_length;
 }
 if (!host_length) return 0;
 if (*cursor==':') {
  ++cursor;
  if (*cursor<'0' || *cursor>'9') return 0;
  while (*cursor>='0' && *cursor<='9') {
   value=value*10u+(uint8_t)(*cursor++-'0');
   if (value>65535u) return 0;
  }
  if (!value) return 0;
 }
 if (*cursor!='/') return 0;
 while (*cursor) {
  ch=*cursor++;
  if (!((ch>='a' && ch<='z') || (ch>='A' && ch<='Z')
   || (ch>='0' && ch<='9')
   || strchr("/@-._~!$&'()*+,;=:%[]^`{|}",ch)
   || (public_url && (ch=='?' || ch=='#')))) return 0;
 }
 return 1;
}

static S64StandardResult prepare(S64StandardClient* client,S64StandardReply* reply)
{
 const char *authority,*path,*colon;
 uint16_t length; uint32_t parsed_port;
 if(!client || !reply || !client->url || !client->network.execute) return S64_STANDARD_INVALID;
 memset(reply,0,sizeof(*reply)); active=client; output=reply;
 if(equal_ci(client->url,"https://",8u)) return S64_STANDARD_TLS;
 if(!client->allow_http) return S64_STANDARD_CONSENT;
 if(!equal_ci(client->url,"http://",7u) || !url_valid(client->url,0u)) return S64_STANDARD_INVALID;
 authority=client->url+7; path=strchr(authority,'/');
 if(!path || path==authority || strpbrk(authority,"@?# \r\n\t")) return S64_STANDARD_INVALID;
 length=(uint16_t)(path-authority);
 if(length>=sizeof(host) || !copy(endpoint,sizeof(endpoint),path)) return S64_STANDARD_INVALID;
 memcpy(host,authority,length); host[length]=0; port=80u;
 colon=strchr(host,':');
 if(colon) {
  if(colon==host || !decimal(colon+1,&parsed_port) || !parsed_port || parsed_port>65535u) return S64_STANDARD_INVALID;
  port=(uint16_t)parsed_port;
 }
 return S64_STANDARD_OK;
}
static int unique(uint8_t flag)
{
 if(fields&flag) return 0;
 fields|=flag; return 1;
}
static int reply_line(const char* key,const char* value,void* context)
{
 const char* space; uint32_t size;
 (void)context;
 if(!strcmp(key,"message") || !strcmp(key,"error")) {
  strncpy(output->message,value,sizeof(output->message)-1u); return 1;
 }
 if(!strcmp(key,"warning")) {
  output->warning=1u; strncpy(output->message,value,sizeof(output->message)-1u); return 1;
 }
 if(reader.is_error) return 1;
 if(operation==S64_PUBLISH_REQUEST_STATUS) {
  if(!strcmp(key,"api")) return unique(1u) && !strcmp(value,"1");
  if(!strcmp(key,"auth")) {
   if(!unique(2u)) return 0;
   while(*value) {
    space=strchr(value,' ');
    if((!space && !strcmp(value,"hmac")) || (space && space-value==4 && !strncmp(value,"hmac",4u))) return 1;
    if(!space) break;
    value=space+1;
   }
   return 0;
  }
  if(!strcmp(key,"max-size")) return unique(4u) && decimal(value,&output->max_size) && output->max_size!=0u;
 } else if(operation==S64_PUBLISH_REQUEST_CHALLENGE) {
  if(!strcmp(key,"challenge")) return unique(1u) && s64_publish_challenge_valid(value) && copy(challenge,sizeof(challenge),value);
 } else if(operation==S64_PUBLISH_REQUEST_LIST) {
  if(!strcmp(key,"file")) {
   space=strchr(value,' ');
   if(!space || space-value>10) return 0;
   memcpy(signature,value,(uint16_t)(space-value)); signature[space-value]=0;
   if(!decimal(signature,&size) || !s64_publish_name_valid(space+1)) return 0;
   if(!copy(signature,sizeof(signature),space+1)) return 0;
   lowercase(signature); if(!strcmp(signature,target)) output->exists=1u;
  }
 } else {
  if(!strcmp(key,"name")) return unique(1u) && s64_publish_name_valid(value) && !strcmp(value,target) && copy(output->name,sizeof(output->name),value);
  if(!strcmp(key,"size")) return unique(2u) && decimal(value,&output->size);
  if(!strcmp(key,"url")) return unique(4u) && url_valid(value,1u) && copy(output->url,sizeof(output->url),value);
 }
 return 1;
}
static uint8_t receive(const uint8_t* bytes,uint16_t length,void* context)
{
 uint16_t i=0u,amount; uint8_t native; const char* value;
 (void)context;
 while(i<length && !headers_done) {
  if(++header_total>4096u) goto invalid;
  if(bytes[i]==13u) { ++i; continue; }
  if(bytes[i]!=10u) {
   if(header_length>=sizeof(header)-1u || bytes[i]<32u || bytes[i]>126u || !s64_charset_ascii_to_native(bytes[i++],&native)) goto invalid;
   header[header_length++]=(char)native; continue;
  }
  ++i; header[header_length]=0;
  if(!have_status) {
   if(header_length<12u || (!equal_ci(header,"HTTP/1.0 ",9u) && !equal_ci(header,"HTTP/1.1 ",9u))) goto invalid;
   if(header[9]<'1' || header[9]>'5' || header[10]<'0' || header[10]>'9' || header[11]<'0' || header[11]>'9') goto invalid;
   http_status=(uint16_t)((header[9]-'0')*100u+(header[10]-'0')*10u+header[11]-'0'); have_status=1u;
  } else if(!header_length) headers_done=1u;
  else if(equal_ci(header,"Content-Length:",15u)) {
   value=header+15; while(*value==' ') ++value;
   if(have_length || !decimal(value,&remaining) || remaining>65535u) goto invalid;
   have_length=1u;
  } else if(equal_ci(header,"Transfer-Encoding:",18u)) goto invalid;
  header_length=0u;
 }
 amount=length-i;
 if(amount) {
  if(have_length && amount>remaining) goto invalid;
  if(have_length) remaining-=amount;
  if(!s64_publish_reply_feed(&reader,bytes+i,amount,reply_line,0)) goto invalid;
 }
 return 1u;
invalid: failed=1u; return 0u;
}
static int send_text(const char* text)
{
 return s64_net_write_text(&active->network,socket_handle,text,(uint16_t)strlen(text))==S64_NET_OK;
}
static uint8_t send_source(const uint8_t* data,uint16_t length,void* context)
{
 (void)context;
 return s64_publish_sign_update(&signer,data,length) && s64_net_write(&active->network,socket_handle,data,length)==S64_NET_OK;
}
static S64StandardResult exchange(S64PublishRequestKind kind,uint32_t size,S64StandardWriter writer,void* context)
{
 S64NetResult net; S64StandardResult result=S64_STANDARD_NETWORK;
 uint16_t idle=0u,received,polls=0u; char* colon;
 operation=kind;
 if(!s64_publish_request_path(uri,sizeof(uri),endpoint,kind,active->id,target,size,challenge)) return S64_STANDARD_INVALID;
 if(kind==S64_PUBLISH_REQUEST_LIST || kind==S64_PUBLISH_REQUEST_PUT) {
  if(!active->key || !s64_direct_key_parse(active->key,raw_key) || !s64_publish_sign_begin(&signer,raw_key,kind==S64_PUBLISH_REQUEST_LIST?"list":"put",active->id,challenge,kind==S64_PUBLISH_REQUEST_LIST?"":target,size)) {
   memset(raw_key,0,sizeof(raw_key)); return S64_STANDARD_INVALID;
  }
  memset(raw_key,0,sizeof(raw_key));
 }
 colon=strchr(host,':'); if(colon) *colon=0;
 net=s64_net_open_tcp(&active->network,host,port,&socket_handle);
 if(colon) *colon=':';
 if(net!=S64_NET_OK) { s64_publish_sign_cancel(&signer); return result; }
 headers_done=have_status=have_length=failed=body_sent=fields=0u;
 header_length=header_total=http_status=0u; remaining=0u;
 s64_publish_reply_init(&reader);
 if(!send_text(kind==S64_PUBLISH_REQUEST_STATUS?"GET ":"POST ") || !send_text(uri) || !send_text(" HTTP/1.0\r\nHost: ") || !send_text(host) || !send_text("\r\nConnection: close\r\n")) goto done;
 if(kind!=S64_PUBLISH_REQUEST_STATUS) {
  number(kind==S64_PUBLISH_REQUEST_CHALLENGE?0u:size+64u);
  if(!send_text("Content-Length: ") || !send_text(header) || !send_text("\r\nContent-Type: application/octet-stream\r\n")) goto done;
 }
 if(!send_text("\r\n")) goto done;
 if(kind==S64_PUBLISH_REQUEST_PUT || kind==S64_PUBLISH_REQUEST_LIST) {
  if((writer && !writer(send_source,0,context)) || !s64_publish_sign_finish(&signer,signature)) goto done;
  /* A complete signature may reach the server even if acknowledgement fails. */
  body_sent=1u; if(!send_text(signature)) goto done;
 }
 while(idle<512u && polls++<4096u) {
  received=0u;
  net=s64_net_read(&active->network,socket_handle,S64_NET_READ_MAX,receive,0,&received);
  if(failed) { result=S64_STANDARD_PROTOCOL; goto done; }
  if(net!=S64_NET_OK && net!=S64_NET_NO_DATA && net!=S64_NET_CLOSED) goto done;
  if(received) idle=0u; else ++idle;
  if((headers_done && have_length && !remaining) || net==S64_NET_CLOSED) break;
 }
 if(idle>=512u || polls>=4096u || !headers_done || (have_length && remaining) || !s64_publish_reply_finish(&reader)) { result=S64_STANDARD_PROTOCOL; goto done; }
 if(reader.is_error || http_status<200u || http_status>=300u) { result=S64_STANDARD_SERVER; goto done; }
 if((kind==S64_PUBLISH_REQUEST_STATUS && fields!=7u) || (kind==S64_PUBLISH_REQUEST_CHALLENGE && fields!=1u) || (kind==S64_PUBLISH_REQUEST_PUT && fields!=7u)) { result=S64_STANDARD_PROTOCOL; goto done; }
 result=S64_STANDARD_OK;
done:
 net=s64_net_close(&active->network,socket_handle);
 if (net!=S64_NET_OK && result==S64_STANDARD_OK) {
  if (kind==S64_PUBLISH_REQUEST_PUT) {
   if (!output->warning) strcpy(output->message,"Published, socket close failed");
   output->warning=1u;
  } else result=S64_STANDARD_NETWORK;
 }
 memset(signature,0,sizeof(signature)); s64_publish_sign_cancel(&signer);
 if(kind==S64_PUBLISH_REQUEST_PUT && body_sent && result!=S64_STANDARD_OK && result!=S64_STANDARD_SERVER) return S64_STANDARD_UNCERTAIN;
 return result;
}
S64StandardResult s64_standard_status(S64StandardClient* client,S64StandardReply* reply)
{
 S64StandardResult result=prepare(client,reply);
 return result==S64_STANDARD_OK?exchange(S64_PUBLISH_REQUEST_STATUS,0u,0,0):result;
}
S64StandardResult s64_standard_list(S64StandardClient* client,const char* name,S64StandardReply* reply)
{
 S64StandardResult result=prepare(client,reply);
 if(result!=S64_STANDARD_OK) return result;
 if(!s64_publish_name_valid(name) || !copy(target,sizeof(target),name)) return S64_STANDARD_INVALID;
 lowercase(target); result=exchange(S64_PUBLISH_REQUEST_CHALLENGE,0u,0,0);
 if(result==S64_STANDARD_OK) result=exchange(S64_PUBLISH_REQUEST_LIST,0u,0,0);
 memset(challenge,0,sizeof(challenge)); return result;
}
S64StandardResult s64_standard_put(S64StandardClient* client,const char* name,uint32_t size,S64StandardWriter writer,void* context,S64StandardReply* reply)
{
 S64StandardResult result=s64_standard_status(client,reply);
 if(result!=S64_STANDARD_OK) return result;
 if(size>reply->max_size || size>61440u) return S64_STANDARD_TOO_LARGE;
 if(!writer || !s64_publish_name_valid(name) || !copy(target,sizeof(target),name)) return S64_STANDARD_INVALID;
 lowercase(target); result=exchange(S64_PUBLISH_REQUEST_CHALLENGE,0u,0,0);
 if(result==S64_STANDARD_OK) result=exchange(S64_PUBLISH_REQUEST_PUT,size,writer,context);
 if(result==S64_STANDARD_OK && reply->size!=size) reply->warning=1u;
 memset(challenge,0,sizeof(challenge)); return result;
}
