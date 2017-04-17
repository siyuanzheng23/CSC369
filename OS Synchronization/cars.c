#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"

extern struct intersection isection;

/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
 
 /* Problem and undone parts:
 * memory leak checking
 * synchronization problem , cars are continuously arriving(e.g.)
 * The file that contains our cdf and other information
 */
void parse_schedule(char *file_name) {
    int id;
    struct car *cur_car;
    struct lane *cur_lane;
    enum direction in_dir, out_dir;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

        /* construct car */
        cur_car = malloc(sizeof(struct car));
        cur_car->id = id;
        cur_car->in_dir = in_dir;
        cur_car->out_dir = out_dir;

        /* append new car to head of corresponding list */
        cur_lane = &isection.lanes[in_dir];
        cur_car->next = cur_lane->in_cars;
        cur_lane->in_cars = cur_car;
        cur_lane->inc++;
    }

    fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {
    /* 1) Initialize 4 locks for each quadrant.
       2) Initialize each lane. (A total of 4.)*/

    int i;
    for(i = 0; i < 4; i ++){
        /*Init lock for each quad*/
        pthread_mutex_init(&(isection.quad[i]),NULL);
        /*Init lock for each lane */
        pthread_mutex_init(&(isection.lanes[i].lock),NULL);
        /*Initialize two conditional vars for each lane.*/
        pthread_cond_init(&(isection.lanes[i].producer_cv),NULL);
        pthread_cond_init(&(isection.lanes[i].consumer_cv),NULL);
        /*Initialize the first element of linked lists of in_cars and out_cars to be NULL.*/
        isection.lanes[i].in_cars = NULL;
        isection.lanes[i].out_cars = NULL;

        /*Init attributes to zero for each lane.*/
        isection.lanes[i].inc = 0; /*Incoming cars*/      
        isection.lanes[i].passed = 0; /* Number of cars that have passed the lane.*/
        /*make index of the head and tail of the circular buffer to be 0 */ 
        isection.lanes[i].head = 0; /*Head position of buffer. 0 when start.*/
        isection.lanes[i].tail = 0; /*Tail position of buffer. 0 when start.*/
        /*Allocate momory to circular buffer.*/
        isection.lanes[i].buffer = (struct car **)malloc(LANE_LENGTH * sizeof(struct car *));
        isection.lanes[i].capacity = LANE_LENGTH; /*Capcaity is LANE_LENGTH*/
        isection.lanes[i].in_buf = 0; /*Number of cars in the circular buffer is 0 at the very beginning.*/
    }
}

/** Note: for convention and semantic notation, let head denotes the pointer of first car(left) that should cross the lane
 *   (which joined the lane previously), let tail denotes the first car(right) that just arrived at the lane(cross at the end)
 *   i.e.,    Car_cross_firstly, car_cross_secondly, ... , car_cross_last
 *            head                                         tail
*/

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {

    struct lane *l = arg;
    
    /*Thread continue to runs until there's no more incoming cars.*/
    while(1){
        /*Funtion returns when no more cars is ready to go in lane.*/
        if(l->in_cars == NULL){  
            return NULL;
        }

        /*Lock the lane for synchronization.*/
        pthread_mutex_lock(&(l->lock));

        /*Wait for signal when buffer has available space for car to arrive.*/
        while(l->in_buf >= l->capacity){  
            pthread_cond_wait(&(l->producer_cv), &(l->lock));
        }

        /*Car arrives at the tail of buffer.*/
        l->buffer[l->tail] = l->in_cars;
        /*One more car comes in. Increase car count.*/
        l->in_buf += 1;
        /*One less car waiting to arrive. Decrease inc.*/
        l->inc -= 1; 
        
        //printf("Car with ID %d from %d to %d got into at lane %d, now lane has car# %d\n", l->in_cars->id, l->in_cars->in_dir, l->in_cars->out_dir, l->in_cars->in_dir, l->in_buf);

        /*Make in_cars point to next car, as it's the next car ready for arrival.*/
        l->in_cars = (l->in_cars)->next;
        /*Tail position shift.*/
        l->tail = (l->tail+1)%(l->capacity);
        
        /*signal the 'cross' thread that a new car is added.*/
        pthread_cond_signal(&(l->consumer_cv));
        /*Release lane's lock.*/
        pthread_mutex_unlock(&(l->lock));
    }

    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {

    struct lane *l = arg;

    /*Thread keeps on running until there's no more car to cross or arrive.*/
    while(1){
        
        /*Function can return once no more car can cross, and no more car can arrive to this lane.*/
        if(l->in_buf == 0 && l->in_cars == NULL){
            free(l->buffer);
            return NULL;
        }

        /*Lock this lane for synchronization.*/
        pthread_mutex_lock(&(l->lock));
        /*Wait for arrival signal if lane is empty.*/
        if(l->in_buf == 0 ){
            pthread_cond_wait(&(l->consumer_cv),&(l->lock));
        }

        /*Car at head postion of buffer is now good to cross.*/
        struct car * current = l->buffer[l->head];
        /*Call compute_path to get a list of needed lock for acquiring.*/
        int *computed_path = compute_path(current->in_dir,current->out_dir);
        
        /*Acquire the quadrant's locks in priority order. 1->2->3->4*/
        int i ;
        for(i = 0; computed_path[i] != 0; i++){
            pthread_mutex_lock(&(isection.quad[i]));
        }

        /*Update out_cars of outgoing lane. Latest crossed car will be the head of linked list.
        We don't need a lock here because the quadrant required to lock for entering lane with out_dir
        is locked. Resources is not shared, */
        current->next = (isection.lanes[current->out_dir]).out_cars;
        (isection.lanes[current->out_dir]).out_cars = current;

        /*Update head position of buffer.*/
        l->head = (l->head+1)%(l->capacity);
        /*Less 1 to number of cars in lane*/
        l->in_buf -= 1;
        
        /*Release quadrant's lock in order.*/
        for(i = 0; computed_path[i] != 0; i++){
            pthread_mutex_unlock(&(isection.quad[i]));         
        }
        
        /*Free memory of computed path.*/
        free(computed_path);        
        
        /*Signal arrival thread if buffer has spaces.*/       
        pthread_cond_signal(&(l->producer_cv));
               
        /*Release Lane's lock*/
        pthread_mutex_unlock(&(l->lock));      
    }
    
    return NULL;
}

/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */

/*need to free the memory after compute_path has been called and used*/
int *compute_path(enum direction in_dir, enum direction out_dir) {
    int *cross_path = NULL;

    if(in_dir == EAST){
        if(out_dir == EAST){ //1,4
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 1;
            cross_path[1] = 4;
        }else if(out_dir == SOUTH){ //1,2,3
            cross_path = malloc(sizeof(int) * 3);
            cross_path[0] = 1;
            cross_path[1] = 2;
            cross_path[2] = 3;
        }else if(out_dir == WEST){ //1,2
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 1;
            cross_path[1] = 2;
        }else if(out_dir == NORTH){ //1
            cross_path = malloc(sizeof(int));
            cross_path[0] = 1;
        }else{
        		printf("Invalid argument for out_dir\n");
        		return NULL;
        }
    }else if(in_dir == SOUTH){
        if(out_dir == EAST){ //4
            cross_path = malloc(sizeof(int));
            cross_path[0] = 4;
        }else if(out_dir == SOUTH){ //4,3
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 3;
            cross_path[1] = 4;
        }else if(out_dir == WEST){ //4,1,3
            cross_path = malloc(sizeof(int) * 3);
            cross_path[0] = 1;
            cross_path[1] = 3;
            cross_path[2] = 4;
        }else if(out_dir == NORTH){ //4,1
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 1;
            cross_path[1] = 4;
        }else{
        		printf("Invalid argument for out_dir\n");
        		return NULL;
        }
    }else if(in_dir == WEST){
        if(out_dir == EAST){ //3,4
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 3;
            cross_path[1] = 4;
        }else if(out_dir == SOUTH){ //3
            cross_path = malloc(sizeof(int));
            cross_path[0] = 3;
        }else if(out_dir == WEST){ //3,2
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 2;
            cross_path[1] = 3;
        }else if(out_dir == NORTH){ //3.4.1
            cross_path = malloc(sizeof(int) * 3);
            cross_path[0] = 1;
            cross_path[1] = 3;
            cross_path[2] = 4;
        }else{
        		printf("Invalid argument for out_dir\n");
        		return NULL;
        }   
    }else if(in_dir == NORTH){
        if(out_dir == EAST){ //2,3,4
            cross_path = malloc(sizeof(int) * 3);
            cross_path[0] = 2;
            cross_path[1] = 3;
            cross_path[2] = 4;
        }else if(out_dir == SOUTH){ //2,3
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 2;
            cross_path[1] = 3;
        }else if(out_dir == WEST){ //2
            cross_path = malloc(sizeof(int));
            cross_path[0] = 2;
        }else if(out_dir == NORTH){ //2,1
            cross_path = malloc(sizeof(int) * 2);
            cross_path[0] = 1;
            cross_path[1] = 2;
        }else{
        		printf("Invalid argument for out_dir\n");
        		return NULL;
        }
    }else{
    		printf("Invalid argument for in_dir\n");
    		return NULL;
    }
    return cross_path;
}
