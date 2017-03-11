#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

//Counter to track the last used time for a frame.
int time;

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	int i;

	/*Track the minimum last-used time for a frame*/
	int min;
	/*Track the frame number which has the minimum last-used time*/
	int min_frame;

	for(i = 0; i < memsize; i++) {
		if(i == 0){
			min = coremap[i].lastused;
			min_frame = i;
		}else{
			if(coremap[i].lastused < min){
				min = coremap[i].lastused;
				min_frame = i;
			}
		}

	
	}
	return min_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {

	/*Update last used time for the frame when it's refenrenced.*/
	time ++;

	coremap[p->frame >> PAGE_SHIFT].lastused = time;

	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	/*Init the counter*/
	time = 0;
}
