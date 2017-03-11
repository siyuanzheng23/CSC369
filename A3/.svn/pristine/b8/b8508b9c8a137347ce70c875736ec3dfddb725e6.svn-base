#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

//int head = -1;

//'clock hand' position;
int current;



/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int clock_evict() {
	//treating the physical frames as a circular buffer to retrieve corresponding pte_t information, 
	//by indexing coremap
	//The cuurent frame's   page_table_entry's ref bit is high
	while(  ((coremap[current].pte) -> frame ) &  PG_REF     ){
		//set ref bit to low
		coremap[current].pte -> frame &=  ~PG_REF;
		//move the clock hand to point to the next frame
		current = (current + 1 )%memsize;
	}
	//while loop broken => victim found 
	coremap[current].pte -> frame |= PG_REF;
	//return the index that the 'clock hand' refers to 
	return current ;
}


/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	//setting ref bit high is already implemented at pagetable.c's relevant function
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	//initialze the 'clock hand'
	current = 0;
}
