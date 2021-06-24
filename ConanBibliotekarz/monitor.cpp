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
int Monitor::reply_counter_suits = 0;
int Monitor::my_suits_counter = 0;
int Monitor::reply_wants_s = 1;
int Monitor::my_task = -1;
int Monitor::my_librarian = -1;
int Monitor::taken_suits = 0;

bool Monitor::listening = false;

deque<packet_t> Monitor::queueTasks;
deque<packet_t> Monitor::queueForSuits;
pthread_mutex_t Monitor::mutexQueueTasks = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Monitor::mutexQueueSuits = PTHREAD_MUTEX_INITIALIZER;

bool Monitor::prioritySortCriterion(packet_t conan1, packet_t conan2)
{
	return conan1.ts < conan2.ts;
}

packet_t Monitor::receiveMessage()
{
	packet_t packet;
	MPI_Status status;
	MPI_Recv(&packet, 1, MPI_PAKIET_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	packet.tag = status.MPI_TAG;
	Monitor::incrementLamportOnReceive(packet);
	return packet;
}

void Monitor::sendMessage(packet_t *packet, int target, int tag)
{
	// Monitor::incrementLamportOnSend();
	MPI_Send(packet, 1, MPI_PAKIET_T, target, tag, MPI_COMM_WORLD);
}

void Monitor::incrementLamportOnSend()
{
	pthread_mutex_lock(&Monitor::lamportMutex);
	Monitor::lamport = Monitor::lamport + 1;
	pthread_mutex_unlock(&Monitor::lamportMutex);
}

void Monitor::incrementLamportOnReceive(packet_t packet)
{
	pthread_mutex_lock(&Monitor::lamportMutex);
	Monitor::lamport = ((packet.ts > Monitor::lamport) ? packet.ts : Monitor::lamport) + 1;
	pthread_mutex_unlock(&Monitor::lamportMutex);
}

void Monitor::initMonitor()
{
	MPI_Comm_rank(MPI_COMM_WORLD, &Monitor::rank);
	MPI_Comm_size(MPI_COMM_WORLD, &Monitor::size);
}

void Monitor::deleteTaskFromQueue(int data)
{
	for (auto i = Monitor::queueTasks.begin(); i != Monitor::queueTasks.end();)
	{
		if (i._M_cur->data == data)
		{
			pthread_mutex_lock(&Monitor::mutexQueueTasks);
			Monitor::queueTasks.erase(i);
			pthread_mutex_unlock(&Monitor::mutexQueueTasks);
			break;
		}
		else
			++i;
	}
}

void Monitor::deleteConanFromQueue(int data)
{
	for (auto i = Monitor::queueForSuits.begin(); i != Monitor::queueForSuits.end();)
	{
		if (i._M_cur->src == data)
		{
			pthread_mutex_lock(&Monitor::mutexQueueSuits);
			Monitor::queueTasks.erase(i);
			pthread_mutex_unlock(&Monitor::mutexQueueSuits);
			break;
		}
		else
			++i;
	}
}

void Monitor::listen()
{
	int taken_suits_counter = 0;
	int repliers[Monitor::CONANTASKNUMBER - 1];
	Monitor::listening = true;
	packet_t received;
	packet_t *pkt = new packet_t;
	packet_t *suits = new packet_t;
	while (Monitor::listening)
	{
		received = Monitor::receiveMessage();
		if (received.tag == ACK_DZ)
		{
			if (Conan::state == ConanState::WAIT_Z)
			{
				Conan::state = ConanState::TAKE_Z;
				debug("Conan: Jestem w stanie: TAKE_Z");
			}
			pthread_mutex_lock(&Monitor::mutexQueueTasks);
			queueTasks.push_back(received);
			pthread_mutex_unlock(&Monitor::mutexQueueTasks);
			//debug("Conan: Otrzymałem zlecenie o numerze: %d od Bibliotekarza: %d", received.data, received.src);
		}
		else if (received.tag == REQ_PZ)
		{
			pkt->tag = ACK_PZ;
			pkt->src = Monitor::rank;
			pthread_mutex_lock(&Monitor::takeTaskMutex);
			if (
				Monitor::my_task != received.data &&
				(Monitor::lamport > received.ts ||
				 (Monitor::lamport == received.ts && Monitor::rank > received.src) ||
				 (Conan::state != ConanState::TAKE_Z && Conan::state != ConanState::WAIT_Z)))
			{
				pkt->data = received.data;
				//debug("Conan: Udzielam zgodę (ACK_PZ) na przyjęcie zlecenia: %d przez Conana: %d", received.data, received.src);
			}
			else
			{
				pkt->data = false;
				//debug("Conan: Odmawiam zgody (ACK_PZ) na przyjęcie zlecenia: %d przez Conana: %d", received.data, received.src);
			}
			Monitor::sendMessage(pkt, received.src, ACK_PZ);
			Monitor::incrementLamportOnSend();
			pthread_mutex_unlock(&Monitor::takeTaskMutex);
		}
		else if (received.tag == ACK_PZ)
		{
			if (received.data && Conan::state == ConanState::WAIT_Z)
			{
				bool is_taken = true;
				//sprawdzamy czy ktoś już nie zabrał zlecenia
				for (auto i = Monitor::queueTasks.begin(); i != Monitor::queueTasks.end();)
				{
					if (i._M_cur->data == received.data)
					{
						is_taken = false;
						break;
					}
					else
						++i;
				}
				if (!is_taken)
				{
					bool is_in = false;
					for (int z = 0; z < reply_counter; z++)
					{
						if (repliers[z] == received.src)
						{
							is_in = true;
						}
					}
					if (!is_in && my_task == -1)
					{
						repliers[reply_counter] = received.src;

						if (++reply_counter == Monitor::CONANTASKNUMBER - 1)
						{
							my_task = received.data;
							pkt->tag = ACK_Z;
							pkt->src = Monitor::rank;
							pkt->data = received.data;
							debug("Conan: Dostałem wszystkie zgody na zlecenie: %d", my_task);
							//debug("Conan: Jestem w stanie GET_S");
							Conan::state = ConanState::GET_S;
							for (int i = 0; i < Monitor::CONANTASKNUMBER - 1; i++)
								Monitor::sendMessage(pkt, repliers[i], ACK_Z);
							Monitor::incrementLamportOnSend();
							reply_counter = 0;
						}
					}
				}
			}
			else
			{
				reply_counter = 0;
				if (Conan::state == ConanState::WAIT_Z)
					Conan::state = ConanState::TAKE_Z;
			}
		}
		else if (received.tag == ACK_Z)
		{
			debug("Conan: Otrzymałem informację o przyjęciu zlecenia: %d  przez Conana: %d", received.data, received.src);
			if (Monitor::my_task == received.data)
			{
				debug("Conan: MAMY KONFLIKT PANOWIE");
				Conan::state = ConanState::TAKE_Z;
				my_task = -1;
			}
			else
			{
				Monitor::deleteTaskFromQueue(received.data);
				reply_counter = 0;
				if (Conan::state == ConanState::WAIT_Z)
					Conan::state = ConanState::TAKE_Z;
			}
		}
		else if (received.tag == REQ_S)
		{
			pkt->tag = ACK_S;
			pkt->src = Monitor::rank;
			pkt->data = Monitor::my_suits_counter;
			if (Conan::state == ConanState::GET_S && (Monitor::lamport < received.ts || (Monitor::lamport == received.ts && Monitor::rank < received.src)))
			{
				pkt->cc[0] = false;
				pkt->cc[1] = 1; // o tyle sie ubiegam
				//debug("Conan: Odmawiam zgody (ACK_S) na przyjęcie stroju przez Conana: %d", received.src);
				Monitor::sendMessage(pkt, received.src, ACK_S);
				Monitor::incrementLamportOnSend();
			}
			else
			{
				pkt->cc[0] = true;
				pkt->cc[1] = 1; // o tyle sie ubiegam
				bool is_in = false;
				//sprawdzamy czy gosc nie jest w kolejce
				for (auto i = Monitor::queueForSuits.begin(); i != Monitor::queueForSuits.end();)
				{
					if (i._M_cur->src == received.src)
					{
						is_in = true;
						break;
					}
					else
						++i;
				}
				if (is_in)
				{
					Monitor::queueForSuits.push_back(received);
					sort(Monitor::queueForSuits.begin(), Monitor::queueForSuits.end(),
						 prioritySortCriterion);
				}
				//debug("Conan: Udzielam zgodę (ACK_S) na przyjęcie stroju przez Conana: %d", received.src);
				Monitor::sendMessage(pkt, received.src, ACK_S);
				Monitor::incrementLamportOnSend();
			}
		}
		else if (received.tag == ACK_S)
		{
			reply_counter_suits++;
			//Monitor::taken_suits += received.data;
			if (received.cc[0] == 0)
			{
				reply_wants_s++;
				if (Monitor::queueForSuits.empty())
				{
					pthread_mutex_lock(&Monitor::mutexQueueSuits);
					Monitor::queueForSuits.push_back(received);
					pthread_mutex_unlock(&Monitor::mutexQueueSuits);
				}
				else
				{
					bool is_in = false;
					//sprawdzamy czy gosc nie jest w kolejce
					for (auto i = Monitor::queueForSuits.begin(); i != Monitor::queueForSuits.end();)
					{
						if (i._M_cur->src == received.src)
						{
							is_in = true;
							break;
						}
						else
							++i;
					}
					if (is_in)
					{
						pthread_mutex_lock(&Monitor::mutexQueueSuits);
						Monitor::queueForSuits.push_back(received);
						sort(Monitor::queueForSuits.begin(), Monitor::queueForSuits.end(),
							 prioritySortCriterion);
						pthread_mutex_unlock(&Monitor::mutexQueueSuits);
					}
				}
			}
			if (Monitor::reply_counter_suits == Monitor::NUMBER_OF_CONANS - 1)
			{
				reply_counter_suits = 0;
				if (Monitor::taken_suits >= Monitor::SUITS)
				{
					debug("Conan: Stoję w kolejce po strój!");
					Conan::state = ConanState::WAIT_S;
				}
				else
				{
					if (Monitor::queueForSuits.front().src == rank)
					{
						pkt->tag = ACK_TS;
						pkt->src = Monitor::rank;
						for (int i = 0; i < size; i++)
						{
							if (i == rank || !(i % 4))
								continue;
							Monitor::sendMessage(pkt, i, ACK_TS);
						}
						Monitor::incrementLamportOnSend();
						Monitor::deleteConanFromQueue(rank);
						taken_suits++;
						my_suits_counter++;
						debug("Conan: Biorę strój");
						Conan::state = ConanState::COMPLETE_Z;
					}
				}
			}
		}
		else if (received.tag == ACK_TS)
		{
			taken_suits++;
			Monitor::deleteConanFromQueue(received.src);
			sort(Monitor::queueForSuits.begin(), Monitor::queueForSuits.end(),
				 prioritySortCriterion);
		}
		else if (received.tag == RELEASE_S)
		{
			taken_suits--;
			if (Monitor::queueForSuits.front().src == rank && Conan::state == ConanState::WAIT_S && taken_suits < Monitor::SUITS)
			{
				pkt->tag = ACK_TS;
				pkt->src = Monitor::rank;

				for (int i = 0; i < size; i++)
				{
					if (i == rank || !(i % 4))
						continue;
					Monitor::sendMessage(pkt, i, ACK_TS);
				}
				Monitor::incrementLamportOnSend();
				Monitor::deleteConanFromQueue(rank);
				debug("Conan: Biorę strój");
				my_suits_counter++;
				taken_suits++;
				Conan::state = ConanState::COMPLETE_Z;
			}
		}
	}
}
