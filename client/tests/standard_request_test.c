#include "publisher/standard_request.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const char* c = "00112233445566778899aabbccddeeff";
    char path[256];
    assert(s64_publish_request_path(path,sizeof(path),"/publish",
        S64_PUBLISH_REQUEST_STATUS,0,0,0,0));
    assert(strcmp(path,"/publish?op=status")==0);
    assert(s64_publish_request_path(path,sizeof(path),"/publish",
        S64_PUBLISH_REQUEST_CHALLENGE,"anna-1",0,0,0));
    assert(strcmp(path,"/publish?op=challenge&id=anna-1")==0);
    assert(s64_publish_request_path(path,sizeof(path),"/publish",
        S64_PUBLISH_REQUEST_LIST,"anna-1",0,0,c));
    assert(strcmp(path,"/publish?op=list&id=anna-1&size=0&c=00112233445566778899aabbccddeeff")==0);
    assert(s64_publish_request_path(path,sizeof(path),"/publish",
        S64_PUBLISH_REQUEST_PUT,"anna-1","news/index.8tml",1874,c));
    assert(strcmp(path,"/publish?op=put&id=anna-1&name=news/index.8tml&size=1874&c=00112233445566778899aabbccddeeff")==0);
    assert(!s64_publish_request_path(path,10,"/publish",
        S64_PUBLISH_REQUEST_STATUS,0,0,0,0) && !path[0]);
    assert(!s64_publish_request_path(path,sizeof(path),"/publish?x=1",
        S64_PUBLISH_REQUEST_STATUS,0,0,0,0));
    assert(!s64_publish_request_path(path,sizeof(path),"/publish",
        S64_PUBLISH_REQUEST_LIST,"anna-1",0,1,c));
    assert(!s64_publish_request_path(path,sizeof(path),"/publish",
        S64_PUBLISH_REQUEST_PUT,"anna-1","../x",2,c));
    assert(!s64_publish_request_path(path,sizeof(path),"/publish",
        S64_PUBLISH_REQUEST_PUT,"anna-1","foo.8tml",2,"invalid"));
    puts("standard request path tests passed");
    return 0;
}
