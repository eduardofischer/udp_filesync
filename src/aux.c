#include "../include/aux.h"

void delay(int secs){
    // Stroing start time 
    clock_t start_time = time(NULL); 
  
    // looping till required time is not acheived 
    while (time(NULL) < start_time + secs);
}