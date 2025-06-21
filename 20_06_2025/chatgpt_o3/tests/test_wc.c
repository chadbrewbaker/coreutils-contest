###############################################################################
# tests/test_wc.c
#include "wc.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static void run_case(const char *str,uint64_t l,uint64_t w,uint64_t b){
    wc_counts_t c={0};
    wc_count_buffer((const uint8_t*)str,strlen(str),&c);
    assert(c.lines==l);
    assert(c.words==w);
    assert(c.bytes==b);
}

int main(void){
    run_case("",0,0,0);
    run_case("hello\n",1,1,6);
    run_case("hello world\n",1,2,12);
    run_case("  leading and trailing  \n",1,3,27);
    run_case("\n\n\n",3,0,3);
    run_case("one two\nthree\tfour\n",2,4,19);
    puts("All unit tests passed!");

    // Integration test: compare with system wc for this source file
    const char *path="src/wc.c";
    char cmd[256];
    snprintf(cmd,sizeof(cmd),"./wc %s | awk '{print $1,$2,$3}' > /tmp/self_wc",path);
    system(cmd);
    snprintf(cmd,sizeof(cmd),"/usr/bin/wc %s | awk '{print $1,$2,$3}' > /tmp/sys_wc",path);
    int diff=system("diff -q /tmp/self_wc /tmp/sys_wc");
    assert(diff==0);
    puts("Integration test passed (output matches BSD wc).\n");
    return 0;
}
