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
                                for (int j = 0; j < Monitor::CONANTASKNUMBER; j++)
                                {
                                        if (Monitor::queueTasks[i].cc[j] == Monitor::rank)
                                        {
                                                continue;
                                        }
                                        pkt->data = Monitor::queueTasks[i].data;
                                        pkt->tag = REQ_PZ;
                                        pthread_mutex_lock(&Monitor::lamportMutex);
                                        pkt->ts = Monitor::lamport;
                                        pthread_mutex_unlock(&Monitor::lamportMutex);
                                        Monitor::sendMessage(pkt, Monitor::queueTasks[i].cc[j], REQ_PZ);
                                        Conan::state = ConanState::WAIT_Z;
                                        debug("Conan: Wysłałem REQ_PZ o zlecenie o numerze: %d do Conana: %d", pkt->data, Monitor::queueTasks[i].cc[j]);
                                }
                                //czeka na odpowiedzi
                                while (Conan::state == ConanState::WAIT_Z)
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
                        pthread_mutex_lock(&Monitor::lamportMutex);
                        pkt->ts = Monitor::lamport;
                        pthread_mutex_unlock(&Monitor::lamportMutex);
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
                        debug("Conan: Wysłałem REQ_S do wszystkich Conanów");
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
                        Monitor::sendMessage(pkt, Monitor::my_librarian, ACK_WZ);
                        Monitor::my_task = -1;
                        Conan::state = ConanState::RETURN_S;
                        debug("Conan: Jestem w pralni której nie zaimplementowaliście!!!");
                        sleep(5);
                }
                if (Conan::state == ConanState::RETURN_S)
                {
                        if (Monitor::queueTasks.size() == 0)
                                Conan::state = ConanState::WAIT_Z;
                        else
                                Conan::state = ConanState::TAKE_Z;
                        pkt->tag = RELEASE_S;
                        for (int i = 0; i < size; i++)
                        {
                                if (i == rank || !(i % 4))
                                        continue;
                                Monitor::sendMessage(pkt, i, RELEASE_S);
                        }
                        Monitor::my_suits_counter--;
                        Monitor::taken_suits--;
                        debug("Conan: Oddałem strój ziomeczki!");
                }
        }
        sleep(5);
        pthread_join(threadNewTask, NULL);
}

void *listenForNewTasks(void *x)
{
        Monitor::listen();
}