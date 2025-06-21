###############################################################################
# src/wc.c
#include "wc.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <arm_neon.h>

static inline uint8_t is_ascii_space(uint8_t c){
    return (c==' '||c=='\n'||c=='\t'||c=='\r'||c=='\f'||c=='\v');
}

void wc_count_buffer(const uint8_t *data,size_t len,wc_counts_t *out){
    uint64_t lines=0,words=0,bytes=len;
    uint8_t in_word=0;
    const uint8_t *ptr=data,*end=data+len;

    // Vectorised loop (16‑byte chunks)
    const size_t step=16;
    while(ptr+step<=end){
        uint8x16_t v=vld1q_u8(ptr);

        // Count newlines (vectorised)
        uint8x16_t nl_mask=vceqq_u8(v,vdupq_n_u8('\n'));
        uint8x16_t nl_bytes=vandq_u8(nl_mask,vdupq_n_u8(1)); // 1 per match
        lines += vaddvq_u8(nl_bytes);

        // Whitespace mask (space, tab, newline, carriage return, formfeed, vtab)
        uint8x16_t ws = vorrq_u8( vorrq_u8( vceqq_u8(v, vdupq_n_u8(' ')),
                                            vceqq_u8(v, vdupq_n_u8('\t'))),
                                   vorrq_u8( vceqq_u8(v, vdupq_n_u8('\n')),
                                             vorrq_u8( vceqq_u8(v, vdupq_n_u8('\r')),
                                                       vorrq_u8(vceqq_u8(v, vdupq_n_u8('\f')),
                                                                vceqq_u8(v, vdupq_n_u8('\v'))))));
        uint8_t ws_arr[16];
        vst1q_u8(ws_arr, ws);
        for(int i=0;i<16;i++){
            uint8_t is_ws = ws_arr[i]==0xFF; // mask bytes are 0xFF for true
            if(!is_ws && !in_word){
                words++;
                in_word=1;
            }else if(is_ws){
                in_word=0;
            }
        }
        ptr+=step;
    }

    // Tail loop
    while(ptr<end){
        uint8_t c=*ptr++;
        if(c=='\n') lines++;
        uint8_t is_ws=is_ascii_space(c);
        if(!is_ws && !in_word){
            words++;
            in_word=1;
        }else if(is_ws){
            in_word=0;
        }
    }

    out->lines+=lines;
    out->words+=words;
    out->bytes+=bytes;
}

static void wc_file(const char *path,wc_counts_t *totals,int print_name,int sel_l,int sel_w,int sel_c){
    int fd=open(path,O_RDONLY);
    if(fd<0){perror(path);return;}
    struct stat st; if(fstat(fd,&st)){perror("fstat");close(fd);return;}
    size_t len=st.st_size;
    uint8_t *data=mmap(NULL,len,PROT_READ,MAP_PRIVATE,fd,0);
    if(data==MAP_FAILED){perror("mmap");close(fd);return;}

    wc_counts_t c={0};
    wc_count_buffer(data,len,&c);

    if(sel_l) printf("%7llu",(unsigned long long)c.lines);
    if(sel_w) printf("%7llu",(unsigned long long)c.words);
    if(sel_c) printf("%7llu",(unsigned long long)c.bytes);
    if(print_name) printf(" %s",path);
    putchar('\n');

    totals->lines+=c.lines;
    totals->words+=c.words;
    totals->bytes+=c.bytes;

    munmap(data,len);
    close(fd);
}

static void wc_stream(FILE *fp,const char *name,wc_counts_t *totals,int sel_l,int sel_w,int sel_c){
    size_t cap=64*1024; uint8_t *buf=malloc(cap);
    if(!buf){perror("malloc");exit(1);}    
    wc_counts_t c={0};
    size_t n;
    uint8_t in_word=0;
    while((n=fread(buf,1,cap,fp))){
        wc_count_buffer(buf,n,&c);
    }
    if(sel_l) printf("%7llu",(unsigned long long)c.lines);
    if(sel_w) printf("%7llu",(unsigned long long)c.words);
    if(sel_c) printf("%7llu",(unsigned long long)c.bytes);
    printf(" %s\n",name);
    totals->lines+=c.lines;
    totals->words+=c.words;
    totals->bytes+=c.bytes;
    free(buf);
}

int main(int argc,char **argv){
    int opt; int sel_l=1,sel_w=1,sel_c=1;
    while((opt=getopt(argc,argv,"clw"))!=-1){
        if(opt=='c'){sel_l=sel_w=0;sel_c=1;}
        else if(opt=='l'){sel_w=sel_c=0;sel_l=1;}
        else if(opt=='w'){sel_l=sel_c=0;sel_w=1;}
        else {fprintf(stderr,"Usage: %s [-clw] [file ...]\n",argv[0]);return 1;}
    }
    int files=argc-optind;
    wc_counts_t totals={0};
    if(files==0){
        wc_stream(stdin,"-",&totals,sel_l,sel_w,sel_c);
    }else{
        for(int i=optind;i<argc;i++)
            wc_file(argv[i],&totals, files>1,sel_l,sel_w,sel_c);
        if(files>1){
            if(sel_l) printf("%7llu",(unsigned long long)totals.lines);
            if(sel_w) printf("%7llu",(unsigned long long)totals.words);
            if(sel_c) printf("%7llu",(unsigned long long)totals.bytes);
            printf(" total\n");
        }
    }
    return 0;
}
