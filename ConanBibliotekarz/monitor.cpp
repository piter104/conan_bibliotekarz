#include "main.h"
#include "monitor.h"
#include <algorithm>

unsigned int Monitor::lamport = 0;
pthread_mutex_t Monitor::lamportMutex; 

int Monitor::rank;
int Monitor::size;

bool Monitor::listening = false;

packet_t Monitor::receiveMessage() {
	packet_t packet;
    MPI_Status status;
    MPI_Recv( &packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	packet.src = status.MPI_SOURCE;
	Monitor::incrementLamportOnReceive(packet);
	return packet;
}

void Monitor::sendMessage(packet_t *packet, int target, int tag) {
	MPI_Send(packet, 1, MPI_PAKIET_T, target, tag, MPI_COMM_WORLD);
}

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

void Monitor::listen(){
	Monitor::listening = true;
	packet_t received;
	while(Monitor::listening){
		received = Monitor::receiveMessage();
        debug("Conan: Otrzymałem wiadomość o treści: %d od kolegi: %d, typ wiadomości: %d", received.data, received.src, received.tag);
		if (received.tag == 100) {
			debug("Conan: Otrzymałem zlecenie o numerze: %d od Bibliotekarza: %d", received.data, received.src);
		}
		

		// if(received.tag == I_GO){
		// 	if(!Monitor::inOnMission(packet.from))
        //                 	Monitor::onMission.push_back(packet.from);
        //         } else if(packet.tag == MISSION_FINISHED){
        //         	Monitor::onMission.erase(std::remove(Monitor::onMission.begin(), Monitor::onMission.end(), packet.from), Monitor::onMission.end());
        //         } else if (Hunters::listenPrincipal) {
		// 	pthread_mutex_lock(&Monitor::messageQMutex);
		// 	Monitor::messageQ.push(packet);
		// 	pthread_mutex_unlock(&Monitor::messageQMutex);
		// }

	// 	if(packet.tag == SHOP_REQ && Hunters::state != HuntersState::ON_MISSION){
	// 		int target = packet.from;
	// 		Monitor::shop_q.push_back(std::make_pair(packet.lamport,target));
    //                     packet.from = Monitor::rank;
    //                     packet.lamport = Monitor::getMyLamportShopQueue();
	// 		if(Hunters::state == HuntersState::WAITING_SHOP){
	// 			Monitor::sendMessage(&packet,target,TRUE);
	// 		} else {
	// 			Monitor::sendMessage(&packet,target,FALSE);
	// 		}
	// 	} else if(packet.tag == TRUE){
    //             	Monitor::ackShop++;
	// 		Monitor::shop_q.push_back(std::make_pair(packet.lamport,packet.from));
    //     	} else if(packet.tag == FALSE){
    //             	Monitor::ackShop++;
    //     	} else if(packet.tag == OUT){
	// 		//std::cout << YELLOW << Monitor::rank << "::" << packet.from << RESET << std::endl;
    //                     Monitor::ackShop++;
	// 		Monitor::inShop.erase(std::remove(Monitor::inShop.begin(), Monitor::inShop.end(), packet.from), Monitor::inShop.end());
    //             } else if(packet.tag == IN){
	// 		if(Monitor::shop_q.size()>0)
    //                             Monitor::shop_q.erase(Monitor::shop_q.begin());
    //             	Monitor::inShop.push_back(packet.from);
	// 	} else if(packet.tag == I_GO){
	// 		if(!Monitor::inOnMission(packet.from))
    //                     	Monitor::onMission.push_back(packet.from);
    //             } else if(packet.tag == MISSION_FINISHED){
    //             	Monitor::onMission.erase(std::remove(Monitor::onMission.begin(), Monitor::onMission.end(), packet.from), Monitor::onMission.end());
    //             } else if (Hunters::listenPrincipal) {
	// 		pthread_mutex_lock(&Monitor::messageQMutex);
	// 		Monitor::messageQ.push(packet);
	// 		pthread_mutex_unlock(&Monitor::messageQMutex);
	// 	}
	}	
}
