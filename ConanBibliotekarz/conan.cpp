#include "conan.h"
#include "monitor.h"
#include <iostream>

ConanState Conan::state = ConanState::WAIT_Z; //Conan oczekuje na zlecenie

void Conan::loop(int size, int rank){

        debug("Jestę Conanę - %d, stan: WAIT_Z", rank);

        //Conan oczekuje na nowe zlecenie
        pthread_t threadNewTask;
        pthread_create(&threadNewTask, NULL, &listenForNewTasks, NULL);

        while (1) {
                if (Conan::state == ConanState::TAKE_Z){
                        for (int i=0; i< Monitor::queueTasks.size(); i++){
                                debug("nowe zlecenie od bibliotekarza: %d, stan: TAKE_Z", Monitor::queueTasks.front().src);
                        }
                sleep(10);
                }
        }

        
        
        
        // Conan::state = ConanState::TAKE_Z;
        
        // packet_t *pkt = new packet_t;
        // pkt->src = Monitor::rank;
        // packet_t received;
        // while(1){
        // sleep(10);
        // int target = rand()%4;
        // pkt->data = rand()%100;
        // // Monitor::sendMessage(pkt, target, 2);
        // // debug("Conan: Wysłałem wiadomość o treści: %d do kolegi: %d", pkt->data, target);
        // // received = Monitor::receiveMessage();
        // // debug("Conan: Otrzymałem wiadomość o treści: %d od kolegi: %d", received.data, received.src);
        // }

        pthread_join(threadNewTask, NULL);

        
}

void *listenForNewTasks (void* x) {
	Monitor::listen();
}	