#include "conan.h"
#include "monitor.h"
#include <iostream>

ConanState Conan::state = ConanState::WAIT_Z; //Conan oczekuje na zlecenie

void Conan::loop(int size, int rank)
{

        debug("Jestę Conanę - %d, stan: WAIT_Z", rank);

        //Conan oczekuje na nowe zlecenie
        pthread_t threadNewTask;
        pthread_create(&threadNewTask, NULL, &listenForNewTasks, NULL);
        packet_t *pkt = new packet_t;
        pkt->src = Monitor::rank;
        int counter = 0;
        while (1)
        {
                if (Conan::state == ConanState::TAKE_Z)
                {
                        for (int i = 0; i < Monitor::queueTasks.size(); i++)
                        {
                                if (Monitor::queueTasks[i].data <= 0)
                                        continue;
                                pkt->ts = Monitor::incrementLamportOnSend();
                                for (int j = 0; j < Monitor::CONANTASKNUMBER; j++)
                                {
                                        if (Monitor::queueTasks[i].cc[j] == Monitor::rank)
                                        {
                                                continue;
                                        }
                                        pkt->data = Monitor::queueTasks[i].data;
                                        pkt->tag = REQ_PZ;
                                        Monitor::sendMessage(pkt, Monitor::queueTasks[i].cc[j], REQ_PZ);
                                        debug("Conan: Wysłałem REQ_PZ o zlecenie o numerze: %d do Conana: %d, LAMPORT: %d", pkt->data, Monitor::queueTasks[i].cc[j], pkt->ts);
                                }
                                Conan::state = ConanState::WAIT_R;
                                debug("Conan: Jestem w stanie WAIT_R");
                                //czeka na odpowiedzi
                                while (Conan::state == ConanState::WAIT_R)
                                {
                                        Monitor::my_librarian = Monitor::queueTasks[i].src;
                                }
                                if (Conan::state == ConanState::GET_S)
                                        break;
                        }
                }
                if (Conan::state == ConanState::GET_S)
                {
                        pkt->tag = REQ_S;
                        pkt->ts = Monitor::incrementLamportOnSend();
                        for (int i = 0; i < size; i++)
                        {
                                if (i == rank || !(i % 4))
                                        continue;
                                Monitor::sendMessage(pkt, i, REQ_S);
                        } 
                        pthread_mutex_lock(&Monitor::mutexQueueSuits);
                        Monitor::queueForSuits.push_back(*pkt);
                        sort(Monitor::queueForSuits.begin(), Monitor::queueForSuits.end(),
                             Monitor::prioritySortCriterion);
                        pthread_mutex_unlock(&Monitor::mutexQueueSuits);
                        Conan::state = ConanState::WAIT_S;
                        debug("Conan: Wysłałem REQ_S do wszystkich Conanów, LAMPORT: %d", pkt->ts);
                        while (Conan::state == ConanState::WAIT_S)
                        {
                        }
                }
                if (Conan::state == ConanState::COMPLETE_Z)
                {
                        debug("Wykonuję zlecenie!");
                        Monitor::deleteTaskFromQueue(Monitor::my_task);
                        sleep(rand() % 10 + 10);
                        debug("Wykonałem zlecenie!");
                        Conan::state = ConanState::REPORT_Z;
                }
                if (Conan::state == ConanState::REPORT_Z)
                {
                        pkt->data = true;
                        pkt->tag = ACK_WZ;
                        pkt->ts = Monitor::incrementLamportOnSend();
                        Monitor::sendMessage(pkt, Monitor::my_librarian, ACK_WZ);
                        Monitor::my_task = -1;
                        pkt->tag = REQ_P;
                        pkt->ts = Monitor::incrementLamportOnSend();
                        for (int i = 0; i < size; i++)
                        {
                                if (i == rank || !(i % 4))
                                        continue;
                                Monitor::sendMessage(pkt, i, REQ_P);
                        }
                        pthread_mutex_lock(&Monitor::mutexQueueLaundry);
                        Monitor::queueForLaundry.push_back(*pkt);
                        sort(Monitor::queueForLaundry.begin(), Monitor::queueForLaundry.end(),
                             Monitor::prioritySortCriterion);
                        pthread_mutex_unlock(&Monitor::mutexQueueLaundry);
                        Conan::state = ConanState::WAIT_P;
                        debug("Conan: Wysłałem REQ_P do wszystkich Conanów, LAMPORT: %d", pkt->ts);
                        while (Conan::state == ConanState::WAIT_P)
                        {
                        }
                        sleep(5);
                }
                if (Conan::state == ConanState::WASH_P)
                {
                        debug("Piorę!");
                        sleep(rand() % 10 + 10);
                        debug("Wyprałem!");
                        Conan::state = ConanState::RETURN_S;
                }
                if (Conan::state == ConanState::RETURN_S)
                {
                        if (Monitor::queueTasks.size() == 0)
                                Conan::state = ConanState::WAIT_Z;
                        else
                                Conan::state = ConanState::TAKE_Z;
                        pkt->tag = RELEASE_S;
                        pkt->ts = Monitor::incrementLamportOnSend();
                        pthread_mutex_lock(&Monitor::mutexTakenSuits);
                        for (int i = 0; i < size; i++)
                        {
                                if (i == rank || !(i % 4))
                                        continue;
                                Monitor::sendMessage(pkt, i, RELEASE_S);
                        }
                        Monitor::my_suits_counter--;
                        Monitor::taken_suits--;
                        Monitor::my_laundry_counter--;
                        Monitor::occupied_laundry--;
                        pthread_mutex_unlock(&Monitor::mutexTakenSuits);
                        debug("Conan: Oddałem strój ziomeczki! LAMPORT: %d, zajęte stroje: %d, zajęte miejsca w pralni: %d", pkt->ts, Monitor::taken_suits, Monitor::occupied_laundry);
                }
        
        }
        sleep(5);
        pthread_join(threadNewTask, NULL);
}

void *listenForNewTasks(void *x)
{
        Monitor::listen();
}