#include "browser/http.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const char* response;
static unsigned offset, chunk, closes, opens, bytes, fail_send, fail_close, cancel;
static unsigned fail_open, fail_read, idle_read, fail_sink;
static char request[600];
S64NetResult s64_net_open_tcp(S64NetTransport* n, const char* h, uint16_t p, uint8_t* handle)
{ (void)n; assert(!strcmp(h,"example.org")); assert(p == 8080); ++opens; *handle=1; return fail_open ? S64_NET_TARGET_ERROR : S64_NET_OK; }
S64NetResult s64_net_close(S64NetTransport* n, uint8_t h)
{ (void)n; assert(h==1); ++closes; return fail_close ? S64_NET_TARGET_ERROR : S64_NET_OK; }
S64NetResult s64_net_write_text(S64NetTransport* n, uint8_t h, const char* text, uint16_t length)
{ (void)n; (void)h; assert(strlen(request)+length<sizeof(request)); strncat(request,text,length); return fail_send ? S64_NET_TARGET_ERROR : S64_NET_OK; }
S64NetResult s64_net_read(S64NetTransport* n, uint8_t h, uint16_t max, S64UciDataSink sink, void* context, uint16_t* received)
{
    unsigned count=(unsigned)strlen(response)-offset;
    (void)n; (void)h; assert(max==512);
    if(fail_read) return S64_NET_TRANSPORT_ERROR;
    if(idle_read) { *received=0; return S64_NET_NO_DATA; }
    if(count>chunk) count=chunk;
    *received=(uint16_t)count;
    if(!count) return S64_NET_CLOSED;
    if(!sink((const uint8_t*)response+offset,(uint16_t)count,context)) return S64_NET_TRANSPORT_ERROR;
    offset+=count; return S64_NET_OK;
}
static uint8_t body(const uint8_t* data,uint16_t length,void* context)
{ (void)data; (void)context; bytes+=length; return (uint8_t)!fail_sink; }
static uint8_t wait_read(void* context) { (void)context; return (uint8_t)!cancel; }
static S64BrowserHttpResult run(const char* reply,unsigned split)
{
    S64NetTransport network;
    memset(&network,0,sizeof(network)); response=reply; chunk=split;
    offset=closes=opens=bytes=0; request[0]=0;
    return s64_browser_http_get(&network,"example.org",8080,"/index.php",body,0,wait_read,0);
}
int main(void)
{
    unsigned i;
    const char* good="HTTP/1.0 200 OK\r\nContent-Type: application/x-see64you-browser; charset=us-ascii\r\nContent-Length: 4\r\n\r\ntest";
    for(i=1;i<120;++i) {
        assert(run(good,i)==S64_BROWSER_HTTP_OK); assert(bytes==4 && opens==1 && closes==1);
        assert(strstr(request,"Host: example.org:8080\r\n"));
        assert(strstr(request,"Accept: application/x-see64you-browser\r\n"));
    }
    assert(run("HTTP/1.1 200 OK\nContent-Type: application/x-see64you-browser\n\ntest",1)==S64_BROWSER_HTTP_OK);
    assert(run("HTTP/1.0 404 Missing\r\nContent-Type: text/html\r\n\r\nerror",3)==S64_BROWSER_HTTP_SERVER && bytes==0);
    assert(run("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\nerror",2)==S64_BROWSER_HTTP_PROTOCOL && bytes==0);
    assert(run("HTTP/1.0 200 OK\r\nContent-Type: application/x-see64you-browser-extra\r\n\r\n",7)==S64_BROWSER_HTTP_PROTOCOL);
    assert(run("HTTP/1.0 200 OK\r\nContent-Type: application/x-see64you-browser\r\nContent-Length: 5\r\n\r\ntest",9)==S64_BROWSER_HTTP_PROTOCOL);
    assert(run("HTTP/1.0 200 OK\r\nContent-Type: application/x-see64you-browser\r\nContent-Length: 3\r\n\r\ntest",512)==S64_BROWSER_HTTP_PROTOCOL);
    assert(run("HTTP/1.0 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n",1)==S64_BROWSER_HTTP_PROTOCOL);
    assert(run("HTTP/1.0 200 OK\rX: y\r\n\r\n",1)==S64_BROWSER_HTTP_PROTOCOL);
    fail_send=1; assert(run(good,5)==S64_BROWSER_HTTP_NETWORK && closes==1); fail_send=0;
    cancel=1; assert(run(good,5)==S64_BROWSER_HTTP_CANCELLED && closes==1); cancel=0;
    fail_close=1; assert(run(good,5)==S64_BROWSER_HTTP_CLOSE && closes==1); fail_close=0;
    fail_open=1; assert(run(good,5)==S64_BROWSER_HTTP_NETWORK && closes==0); fail_open=0;
    fail_read=1; assert(run(good,5)==S64_BROWSER_HTTP_NETWORK && closes==1); fail_read=0;
    idle_read=1; assert(run(good,5)==S64_BROWSER_HTTP_PROTOCOL && closes==1); idle_read=0;
    fail_sink=1; assert(run(good,5)==S64_BROWSER_HTTP_PROTOCOL && closes==1); fail_sink=0;
    puts("browser HTTP tests passed"); return 0;
}
