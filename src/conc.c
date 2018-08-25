// Try having three threads:
//
// Handler:- deals with UI events from the system. Is blocked by the system
// during long user gestures (window move, window resize, trackpad scroll)
// except for generating the events themselves. Must be the program's main
// thread.
//
// Timer:- generates events for any number of timers. Excludes frame ticks,
// which have to be generated on the run thread.
//
// Runner:- runs the main bulk of the program. Generates a frame tick event
// after showing each frame, when animation is requested.

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

// Main function for the events thread.
void *handle(void *arg) {
    while (1) {
        usleep(1700000);
        printf("E\n");
    }
}

// Main function for the timer thread.
void *doTimer(void *arg) {
    usleep(2);
    while (1) {
        usleep(2300000);
        printf("T\n");
    }
}

// Main function for the run thread.
void *run(void *arg) {
    sleep(1);
    while (1) {
        usleep(4300000);
        printf("R\n");
    }
} 

int main() {
    setbuf(stdout, NULL);
    pthread_t timer;
    pthread_t runner;
    pthread_create(&timer, NULL, doTimer, NULL);
    pthread_create(&runner, NULL, run, NULL);
    handle(NULL);
}
