#include "conan.h"
#include "monitor.h"
#include <iostream>

ConanState Conan::state = ConanState::WAIT_Z; //Conan oczekuje na zlecenie

void Conan::loop(int size, int rank){

        debug("Jestę Conanę - %d, stan: WAIT_Z", rank);

        //Conan oczekuje na nowe zlecenie
        pthread_t threadNewTask;
        pthread_create(&threadNewTask, NULL, &listenForNewTasks, NULL);
        packet_t *pkt = new packet_t;
        pkt->src = Monitor::rank;
        int counter = 0;

        while (1) {
                if (Conan::state == ConanState::TAKE_Z){
                        for (int i=0; i< Monitor::queueTasks.size(); i++){
                                debug("nowe zlecenie od bibliotekarza: %d, stan: TAKE_Z", Monitor::queueTasks[i].src);
                                for (int j = 0; j < Monitor::CONANTASKNUMBER; j++){
                                        if(Monitor::queueTasks[i].cc[j] == Monitor::rank){
                                                continue;
                                        }
                                        Monitor::takeTaskLamport = Monitor::getLamport();
                                        pkt->data = Monitor::queueTasks[i].data;
                                        pkt->tag = REQ_PZ;
                                        pthread_mutex_lock(&Monitor::takeTaskMutex);
                                        pkt->ts = Monitor::takeTaskLamport;
                                        pthread_mutex_unlock(&Monitor::takeTaskMutex);
                                        Monitor::sendMessage(pkt, Monitor::queueTasks[i].cc[j], 2);
                                        Conan::state = ConanState::WAIT_Z; 
                                        debug("Conan: Wysłałem ACK_Z na zleceni o numerze: %d do Conana: %d", pkt->data, Monitor::queueTasks[i].cc[j]);
                                }
                                // czeka na odpowiedzi
                                while(Conan::state == ConanState::WAIT_Z){
                                }
                        }
                }
                if (Conan::state == ConanState::GET_S){
                        debug("Conan: Chcę wziąć strój, ale jeszcze tego nie zaimplementowaliście :(");
                        sleep(10);
                }
        }
        sleep(5);
        pthread_join(threadNewTask, NULL);
}

void *listenForNewTasks (void* x) {
	Monitor::listen();
}	