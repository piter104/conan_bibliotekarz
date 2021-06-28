#include "conan.h"
#include "monitor.h"
#include <iostream>

ConanState Conan::state = ConanState::WAIT_Z; //Conan oczekuje na zlecenie

void Conan::loop(int size, int rank)
{

        debug("Melduję, że jestem Conanem");

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
                                Conan::state = ConanState::WAIT_R;
                                for (int j = 0; j < Monitor::CONANTASKNUMBER; j++)
                                {
                                        if (Monitor::queueTasks[i].cc[j] == Monitor::rank)
                                        {
                                                continue;
                                        }
                                        pkt->data = Monitor::queueTasks[i].data;
                                        pkt->tag = REQ_PZ;
                                        Monitor::sendMessage(pkt, Monitor::queueTasks[i].cc[j], REQ_PZ);
                                }
                                Monitor::my_librarian = Monitor::queueTasks[i].src;
                                //czeka na odpowiedzi
                                while (Conan::state == ConanState::WAIT_R)
                                {
                                }
                                if (Conan::state == ConanState::GET_S)
                                        break;
                        }
                }
                if (Conan::state == ConanState::GET_S)
                {
                        debug("Zlecenie jest moje, zabieram się do pracy!");
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
                        debug("Chciałbym wyprać strój! Ciekawe, czy jest miejsce w pralni?")
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
                        pkt->tag = RELEASE_P;
                        pkt->ts = Monitor::incrementLamportOnSend();
                        pthread_mutex_lock(&Monitor::mutexOccupiedLaundry);
                        for (int i = 0; i < size; i++)
                        {
                                if (i == rank || !(i % 4))
                                        continue;
                                Monitor::sendMessage(pkt, i, RELEASE_P);
                        }
                        Monitor::my_laundry_counter--;
                        Monitor::occupied_laundry--;
                        pthread_mutex_unlock(&Monitor::mutexOccupiedLaundry);
                        Conan::state = ConanState::RETURN_S;
                }
                if (Conan::state == ConanState::RETURN_S)
                {
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
                        pthread_mutex_unlock(&Monitor::mutexTakenSuits);
                        debug("Zwracam strój do magazynu!")
                        if (Monitor::queueTasks.size() == 0)
                                Conan::state = ConanState::WAIT_Z;
                        else
                                Conan::state = ConanState::TAKE_Z;
                }
        }
        sleep(5);
        pthread_join(threadNewTask, NULL);
}

void *listenForNewTasks(void *x)
{
        Monitor::listen();
}