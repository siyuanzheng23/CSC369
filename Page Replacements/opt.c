#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"
#include "sim.h" 


//extern int memsize;

extern int debug;

extern struct frame *coremap;

extern char *tracefile;
int lines;
//int count = getFileLines();
addr_t *list; //[count];
extern int ref_count;

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
/*Get number of virtual addresses in tracfile.*/
int getFileLines(){
 	FILE *file = fopen (tracefile, "r");
    lines = 0;	
	int ch;

   	if ( file != NULL ){	

	   	while(!feof(file)){
		    ch = fgetc(file);
	        if(ch == '\n'){
	    	    lines++;
	    	}
	    }

	     fclose ( file );
   	}else{
        perror ("error"); /* why didn't the file open? */
   	  	return -1;
    }  
  	
  	return lines;
 }

 /*Calculater the distance between next usage of virtual address (virtualAddr)
 	and the current reference count.
 	Return -1 if the virtual address is not found in list.
 	*/
int getDistance(int start, addr_t virtualAddr){
	int distance = -1;
	int g;

	for(g = start + 1; g < lines; g++){
		if((int)list[g] == (int)virtualAddr){
			/*Return distance*/
			return g - start;
		}
	}

	return distance;
}


int opt_evict() {
	int i;
	int evicted_list[memsize];
	
	/*Loop through over each frame, if there's virtual address that won't be used
	later on, evict the frame holding that virtual address.
	Otherwise, keep going, and evict the frame with longest unused period.*/
	for(i = 0; i < memsize; i++){

		int dist= getDistance(ref_count, coremap[i].vddr);
		if(dist == -1){
			return i;

		}else{
			evicted_list[i] = dist;
		}
	}

	int max_frame, max_dist;

	for(i = 0; i < memsize; i++){
		if(i == 0){
			max_frame = i;
			max_dist = evicted_list[i];

		}else{
			if(max_dist < evicted_list[i]){
				max_dist = evicted_list[i];
				max_frame = i;
			}
		}
	}

	return max_frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {

	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
 
void opt_init() {

	/*Get the lines count of tracefile in order to create an array to hold the virtual addresses.*/
	int count = getFileLines();
	/*Holding the list of virtual addresses in tracefile.*/
	list = malloc(sizeof(addr_t) * count);

  	/*File pointer of tracefile.*/
   	FILE *re_openfile = fopen (tracefile, "r");
   	
   	if ( re_openfile != NULL ){	

   		/*Read each line and put the virtual address in list.*/
        char line [MAXLINE]; 
   	    int j = 0;
   	    addr_t virtualaddress = 0;
   	    char type;

        while ( fgets ( line, MAXLINE,  re_openfile) != NULL ){	

      		if(line[0] != '=') {
	      		sscanf(line, "%c %lx", &type, &virtualaddress);      		
	      		list[j] = (addr_t)virtualaddress;
	      		
	      		j ++;
	      	}			
      	}
      	
      	fclose(re_openfile);
    }else{
    	perror ("error"); 
    }
}

