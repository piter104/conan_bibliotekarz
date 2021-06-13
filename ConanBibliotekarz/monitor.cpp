#include "main.h"
#include "monitor.h"
#include <algorithm>

unsigned int Monitor::lamport = 0;
pthread_mutex_t Monitor::lamportMutex; 

int Monitor::rank;
int Monitor::size;


void Monitor::incrementLamportOnSend() {
    pthread_mutex_lock(&Monitor::lamportMutex);
    Monitor::lamport += 1;
    pthread_mutex_unlock(&Monitor::lamportMutex);
}

void Monitor::incrementLamportOnReceive(packet_t packet) {    
	pthread_mutex_lock(&Monitor::lamportMutex);
   	Monitor::lamport = std::max((unsigned) packet.ts, Monitor::lamport) + 1;
    	pthread_mutex_unlock(&Monitor::lamportMutex);
}

unsigned int Monitor::getLamport() {
    return Monitor::lamport;
}

void Monitor::initMonitor(){
	MPI_Comm_rank(MPI_COMM_WORLD, &Monitor::rank);
    MPI_Comm_size(MPI_COMM_WORLD, &Monitor::size);
}

