/* Deterministic byte-reference stress; run tools/test_editor_stress.py. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "editor.h"
#include "platform/reu.h"
static unsigned char ram[2][65536], model[61440], before[61440], actual[61440], clip[61440];
static unsigned int size, clip_size, step;
static uint32_t rng, seed;
static const char *action;
uint8_t s64_reu_probe(void) { return 1; }
uint16_t s64_reu_probe_banks(void) { return 2; }
uint8_t s64_reu_store(uint8_t b,uint16_t a,const void *p,uint16_t n) {
    if(b>=2 || !p || (unsigned long)a+n>65536) return 0;
    memcpy(ram[b]+a,p,n); return 1;
}
uint8_t s64_reu_load(uint8_t b,uint16_t a,void *p,uint16_t n) {
    if(b>=2 || !p || (unsigned long)a+n>65536) return 0;
    memcpy(p,ram[b]+a,n); return 1;
}
static void check(int ok,const char *what) {
    if(!ok) { fprintf(stderr,"FAIL seed=%lu step=%u action=%s: %s\n",(unsigned long)seed,step,action,what); exit(1); }
}
static unsigned int next(unsigned int limit) {
    rng ^= rng<<13; rng ^= rng>>17; rng ^= rng<<5;
    return limit ? rng%limit : 0;
}
static void equal(S64Editor *e,const unsigned char *text,unsigned int n) {
    unsigned int i, line=1, column=1, cursor=s64_buffer_cursor(&e->buffer);
    check(cursor<=n,"cursor within document");
    check(s64_buffer_length(&e->buffer)==n,"length");
    check(s64_buffer_read(&e->buffer,0,actual,n)==n,"read count");
    check(!memcmp(actual,text,n),"byte equality");
    for(i=0;i<cursor;i++) { if(text[i]=='\n') { line++; column=1; } else column++; }
    check(e->line==line && e->column==column,"reference line and column");
}
static void select_at(S64Editor *e,unsigned int start,unsigned int end) {
    check(s64_buffer_move_cursor(&e->buffer,start),"move start");
    s64_editor_clear_selection(e);
    s64_editor_recalculate_position(e);
    s64_editor_toggle_mark(e);
    while(s64_buffer_cursor(&e->buffer)<end) s64_editor_right(e);
}
static void load(S64Editor *e,unsigned int n,unsigned int pattern) {
    unsigned int i;
    s64_editor_init(e); size=n;
    for(i=0;i<n;i++) {
        model[i]=(pattern==1 || (pattern==2 && i%41==40)) ? '\n' : (unsigned char)('a'+i%26);
        check(s64_buffer_insert(&e->buffer,model[i]),"fixture insert");
    }
    s64_editor_document_loaded(e);
}
static void replace(unsigned int a,unsigned int b,const unsigned char *p,unsigned int n) {
    memmove(model+a+n,model+b,size-b); memcpy(model+a,p,n); size=size-(b-a)+n;
}
static void boundary(void) {
    static const unsigned int lengths[]={0,1,39,40,41,255,256,4095,4096,4097,61439,61440};
    S64Editor e; S64Clipboard c; unsigned int i,p,n;
    for(p=0;p<3;p++) for(i=0;i<sizeof(lengths)/sizeof(lengths[0]);i++) {
        action="boundary full copy/cut/paste"; step=i; n=lengths[i]; load(&e,n,p); s64_clipboard_init(&c);
        s64_editor_select_all(&e);
        check(!!s64_editor_copy(&e,&c)==!!n,"copy result");
        check(c.length==n,"clipboard size");
        check(s64_clipboard_read(&c,0,clip,n)==n,"clipboard read");
        check(!memcmp(clip,model,n),"clipboard bytes"); equal(&e,model,n);
        if(n) {
            check(s64_editor_cut(&e,&c),"cut"); equal(&e,model,0);
            check(s64_editor_paste(&e,&c),"paste"); equal(&e,model,n);
        }
        if(n==61440) {
            action="full capacity refusal"; check(!s64_editor_insert(&e,'!'),"insert refused");
            check(!s64_editor_paste(&e,&c),"paste refused"); equal(&e,model,n);
            select_at(&e,0,1); check(!s64_editor_paste(&e,&c),"oversized selection paste refused"); equal(&e,model,n);
            s64_editor_select_all(&e); check(s64_editor_paste(&e,&c),"full selection replacement"); equal(&e,model,n);
        }
    }
}
static void random_run(uint32_t initial,unsigned int n,unsigned int pattern) {
    S64Editor e; S64Clipboard c; unsigned int a,b,op,k,old_size,inserted; int result,expected,changed;
    unsigned char value;
    seed=initial; rng=initial; action="random fixture"; load(&e,n,pattern); s64_clipboard_init(&c); clip_size=0;
    for(step=0;step<1200;step++) {
        a=next(size+1); b=a+next((size-a<80 ? size-a : 80)+1); op=next(6);
        select_at(&e,a,b); memcpy(before,model,size); old_size=size; changed=0;
        if(op==0) {
            action="insert/replace"; value=next(7)==0?'\n':(unsigned char)('A'+next(26));
            expected=size-(b-a)<61440; result=s64_editor_insert(&e,value);
            if(expected) { replace(a,b,&value,1); changed=1; }
        } else if(op==1 || op==2) {
            action=op==1?"delete":"backspace";
            if(a==b) { if(op==1 && b<size) b++; else if(op==2 && a) a--; }
            expected=a<b; result=op==1?s64_editor_delete(&e):s64_editor_backspace(&e);
            if(expected) { replace(a,b,model,0); changed=1; }
        } else if(op==3 || op==4) {
            action=op==3?"copy":"cut"; expected=a<b;
            result=op==3?s64_editor_copy(&e,&c):s64_editor_cut(&e,&c);
            if(expected) { clip_size=b-a; memcpy(clip,model+a,clip_size); if(op==4) { replace(a,b,model,0); changed=1; } }
        } else {
            action="paste"; expected=clip_size && size-(b-a)+clip_size<=61440;
            result=s64_editor_paste(&e,&c);
            if(expected) { replace(a,b,clip,clip_size); changed=1; }
        }
        check(!!result==!!expected,"operation result"); equal(&e,model,size);
        check(c.length==clip_size,"clipboard retained size");
        check(s64_clipboard_read(&c,0,actual,clip_size)==clip_size,"clipboard read"); check(!memcmp(actual,clip,clip_size),"clipboard retained bytes");
        if(changed) {
            check(s64_editor_undo(&e),"undo available"); equal(&e,before,old_size);
            check(s64_editor_redo(&e),"redo available"); equal(&e,model,size);
        }
        /* Movement is independently non-mutating, including wrapped long lines. */
        inserted=next(8);
        for(k=0;k<inserted;k++) { s64_editor_up(&e); s64_editor_down(&e); }
        s64_editor_page_up(&e); s64_editor_page_down(&e); s64_editor_home(&e); s64_editor_end(&e);
        equal(&e,model,size);
    }
}
static void history_sequence(void) {
    S64Editor e; unsigned int i; unsigned char value;
    action="16 transaction history"; load(&e,0,0);
    for(i=0;i<16;i++) {
        step=i; s64_editor_clear_selection(&e); value=(unsigned char)('a'+i);
        check(s64_editor_insert(&e,value),"transaction insert"); model[size++]=value;
    }
    for(i=16;i>0;i--) { check(s64_editor_undo(&e),"multilevel undo"); equal(&e,model,i-1); }
    check(!s64_editor_undo(&e),"oldest undo boundary");
    for(i=1;i<=16;i++) { check(s64_editor_redo(&e),"multilevel redo"); equal(&e,model,i); }
    check(!s64_editor_redo(&e),"newest redo boundary");
    s64_editor_mark_saved(&e); check(!e.dirty,"saved state clean");
    check(s64_editor_undo(&e),"undo saved state"); check(e.dirty,"undo saved state dirty");
    check(s64_editor_redo(&e),"redo saved state"); check(!e.dirty,"redo saved state clean");
    check(s64_editor_undo(&e),"branch undo"); s64_editor_clear_selection(&e);
    check(s64_editor_insert(&e,'!'),"branch insert"); model[15]='!';
    check(!s64_editor_redo(&e),"editing invalidates redo"); equal(&e,model,16);
}
int main(void) {
    unsigned int i;
    action="start"; boundary(); history_sequence();
    for(i=0;i<9;i++) random_run(0x64c0ffeeu+i*7919u,i%3==0?0:i%3==1?4097:61440,i%3);
    puts("Editor stress passed: 36 size/pattern fixtures; 10800 seeded edits with byte-reference, undo/redo and navigation checks.");
    return 0;
}
