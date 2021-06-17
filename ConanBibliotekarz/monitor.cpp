#include "main.h"
#include "monitor.h"
#include "conan.h"
#include <algorithm>

unsigned int Monitor::lamport = 0;
//unsigned int Monitor::takeTaskLamport = 0;
pthread_mutex_t Monitor::lamportMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Monitor::takeTaskMutex = PTHREAD_MUTEX_INITIALIZER; 


int Monitor::rank;
int Monitor::size;
int Monitor::reply_counter = 0;

bool Monitor::listening = false;

deque<packet_t> Monitor::queueTasks;

pthread_mutex_t Monitor::mutexQueueTasks = PTHREAD_MUTEX_INITIALIZER;

packet_t Monitor::receiveMessage() {
	packet_t packet;
    MPI_Status status;
    MPI_Recv( &packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	// packet.src = status.MPI_SOURCE;
	packet.tag = status.MPI_TAG;
	Monitor::incrementLamportOnReceive(packet);
	return packet;
}

void Monitor::sendMessage(packet_t *packet, int target, int tag) {
	Monitor::incrementLamportOnSend();
	MPI_Send(packet, 1, MPI_PAKIET_T, target, tag, MPI_COMM_WORLD);
}

void Monitor::incrementLamportOnSend() {
    pthread_mutex_lock(&Monitor::lamportMutex);
    Monitor::lamport = Monitor::lamport + 1;
    pthread_mutex_unlock(&Monitor::lamportMutex);
}

void Monitor::incrementLamportOnReceive(packet_t packet) {    
	pthread_mutex_lock(&Monitor::lamportMutex);
   	Monitor::lamport = ((packet.ts > Monitor::lamport) ? packet.ts : Monitor::lamport) + 1;
    pthread_mutex_unlock(&Monitor::lamportMutex);
}

// unsigned int Monitor::getLamport() {
//     return Monitor::lamport;
// }

void Monitor::initMonitor(){
	MPI_Comm_rank(MPI_COMM_WORLD, &Monitor::rank);
    MPI_Comm_size(MPI_COMM_WORLD, &Monitor::size);
}

void Monitor::listen(){
	int repliers[Monitor::CONANTASKNUMBER - 1];
	Monitor::listening = true;
	packet_t received;
	packet_t *pkt = new packet_t;
	while(Monitor::listening){
		received = Monitor::receiveMessage();
		if (received.tag == ACK_DZ) {
			if (Conan::state == ConanState::WAIT_Z){
				Conan::state = ConanState::TAKE_Z;
			}
			pthread_mutex_lock(&Monitor::mutexQueueTasks);
			queueTasks.push_back(received);
			pthread_mutex_unlock(&Monitor::mutexQueueTasks);
			debug("Conan: Otrzymałem zlecenie o numerze: %d od Bibliotekarza: %d", received.data, received.src);
		}
		else if (received.tag == REQ_PZ) {
			pkt->tag = ACK_PZ;
			pkt->src = Monitor::rank;
			pthread_mutex_lock(&Monitor::takeTaskMutex);
			debug("Conan: moj: %d, conan_%d: %d", Monitor::lamport, received.src, received.ts);
			if(Monitor::lamport > received.ts || (Monitor::lamport == received.ts && Monitor::rank > received.src)){
				pkt->data = received.data;
				debug("Conan: Udzielam zgodę na przyjęcie zlecenia: %d przez Conana: %d", received.data, received.src);
				Monitor::sendMessage(pkt, received.src, ACK_PZ);
			}
			else {
				pkt->data = false;
				debug("Conan: Odmawiam zgody na przyjęcie zlecenia: %d przez Conana: %d", received.data, received.src);
				Monitor::sendMessage(pkt, received.src, ACK_PZ);
			}
			pthread_mutex_unlock(&Monitor::takeTaskMutex);
		}
		else if (received.tag == ACK_PZ) {
			if(received.data){
				bool is_taken = true;
				//sprawdzamy czy ktoś już nie zabrał zlecenia
				for (auto i = Monitor::queueTasks.begin(); i != Monitor::queueTasks.end();) {
					if (i._M_cur->data == received.data){
						is_taken = false;
						break;
					}
					else
						++i;
					}
				if(!is_taken){
					repliers[reply_counter] = received.src;
					if(++reply_counter == Monitor::CONANTASKNUMBER - 1){
						pkt->tag = ACK_Z;
						pkt->src = Monitor::rank;
						pkt->data = received.data;
						debug("Dostałem wszystkie zgody na zlecenie");
						Conan::state = ConanState::GET_S; 
						for(int i = 0; i < Monitor::CONANTASKNUMBER - 1; i++)
							Monitor::sendMessage(pkt, repliers[i], ACK_Z);
						reply_counter = 0;
					}
				}
			}
			else {
				reply_counter = 0;
				Conan::state = ConanState::TAKE_Z; 
			}
		}
		else if (received.tag == ACK_Z) {
			debug("Conan: Otrzymałem informację o przyjęciu zlecenia: %d  przez Conana: %d", received.data , received.src);
			pthread_mutex_lock(&Monitor::mutexQueueTasks);
			for (auto i = Monitor::queueTasks.begin(); i != Monitor::queueTasks.end();) {
				if (i._M_cur->data == received.data){
					Monitor::queueTasks.erase(i);
					break;
				}
				else
					++i;
				}
			pthread_mutex_unlock(&Monitor::mutexQueueTasks);
			reply_counter = 0;
			Conan::state = ConanState::TAKE_Z; 
		}

	
	}	
}
