###############################################################################
# benches/bench_wc.c
#include "wc.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <time.h>

static double now(void){
    struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts);
    return ts.tv_sec + ts.tv_nsec*1e-9;
}

int main(int argc,char **argv){
    if(argc<2){fprintf(stderr,"Usage: %s FILE\n",argv[0]);return 1;}
    int fd=open(argv[1],O_RDONLY); if(fd<0){perror("open");return 1;}
    struct stat st; fstat(fd,&st); size_t len=st.st_size;
    uint8_t *data=mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    if(data==MAP_FAILED){perror("mmap");return 1;}
    double t0=now();
    wc_counts_t c={0}; wc_count_buffer(data,len,&c);
    double t1=now();
    printf("%zu bytes => %llu lines %llu words %llu bytes in %.3f ms (%.2f GiB/s)\n",len,(unsigned long long)c.lines,(unsigned long long)c.words,(unsigned long long)c.bytes,(t1-t0)*1000.0, len/((t1-t0)*1024.0*1024.0*1024.0));
    munmap(data,len); close(fd);
    return 0;
}
