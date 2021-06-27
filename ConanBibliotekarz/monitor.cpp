#include "main.h"
#include "monitor.h"
#include "conan.h"
#include <algorithm>

unsigned int Monitor::lamport = 0;
//unsigned int Monitor::takeTaskLamport = 0;
pthread_mutex_t Monitor::lamportMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Monitor::takeTaskMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Monitor::mutexTakenSuits = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Monitor::mutexOccupiedLaundry = PTHREAD_MUTEX_INITIALIZER;

int Monitor::rank;
int Monitor::size;
int Monitor::reply_counter = 0;
int Monitor::NUMBER_OF_CONANS = 0;

int Monitor::reply_counter_suits = 0;
int Monitor::my_suits_counter = 0;
int Monitor::taken_suits = 0;

int Monitor::my_task = -1;
int Monitor::my_librarian = -1;

int Monitor::occupied_laundry;
int Monitor::reply_counter_laundry;
int Monitor::my_laundry_counter;

bool Monitor::listening = false;

deque<packet_t> Monitor::queueTasks;
deque<packet_t> Monitor::queueForSuits;
deque<packet_t> Monitor::queueForLaundry;
pthread_mutex_t Monitor::mutexQueueTasks = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Monitor::mutexQueueSuits = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t Monitor::mutexQueueLaundry = PTHREAD_MUTEX_INITIALIZER;

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
	MPI_Send(packet, 1, MPI_PAKIET_T, target, tag, MPI_COMM_WORLD);
}

unsigned int Monitor::incrementLamportOnSend()
{
	unsigned int lamport;
	pthread_mutex_lock(&Monitor::lamportMutex);
	Monitor::lamport = Monitor::lamport + 1;
	lamport = Monitor::lamport;
	pthread_mutex_unlock(&Monitor::lamportMutex);
	return lamport;
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
	int counter = 0;
	for (int i = 0; i < Monitor::size; i++)
	{
		if (i % 4 != 0)
		{
			counter++;
		}
	}
	Monitor::NUMBER_OF_CONANS = counter;
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
			Monitor::queueForSuits.erase(i);
			pthread_mutex_unlock(&Monitor::mutexQueueSuits);
			break;
		}
		else
			++i;
	}
}

void Monitor::deleteConanFromLaundryQueue(int data)
{
	for (auto i = Monitor::queueForLaundry.begin(); i != Monitor::queueForLaundry.end();)
	{
		if (i._M_cur->src == data)
		{
			pthread_mutex_lock(&Monitor::mutexQueueLaundry);
			Monitor::queueForLaundry.erase(i);
			pthread_mutex_unlock(&Monitor::mutexQueueLaundry);
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
				debug("Conan: Jestem w stanie: TAKE_Z, LAMPORT: %d", received.ts);
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
				 (Conan::state != ConanState::TAKE_Z && Conan::state != ConanState::WAIT_R && Conan::state != ConanState::WAIT_Z)))
			{
				pkt->data = received.data;
				//debug("Conan: Udzielam zgodę (ACK_PZ) na przyjęcie zlecenia: %d przez Conana: %d", received.data, received.src);
			}
			else
			{
				pkt->data = false;
				debug("Conan: Odmawiam zgody (ACK_PZ) na przyjęcie zlecenia: %d przez Conana: %d", received.data, received.src);
			}
			pkt->ts = Monitor::incrementLamportOnSend();
			Monitor::sendMessage(pkt, received.src, ACK_PZ);
			pthread_mutex_unlock(&Monitor::takeTaskMutex);
		}
		else if (received.tag == ACK_PZ)
		{
			if (received.data && Conan::state == ConanState::WAIT_R)
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
							pkt->ts = Monitor::incrementLamportOnSend();
							debug("Conan: Dostałem wszystkie zgody na zlecenie: %d, LAMPORT: %d", my_task, received.ts);
							//debug("Conan: Jestem w stanie GET_S");
							Conan::state = ConanState::GET_S;
							for (int i = 0; i < Monitor::CONANTASKNUMBER - 1; i++)
								Monitor::sendMessage(pkt, repliers[i], ACK_Z);
							reply_counter = 0;
						}
					}
				}
			}
			else
			{
				reply_counter = 0;
				if (Conan::state == ConanState::WAIT_R)
					Conan::state = ConanState::TAKE_Z;
			}
		}
		else if (received.tag == ACK_Z)
		{
			debug("Conan: Otrzymałem informację o przyjęciu zlecenia: %d  przez Conana: %d, LAMPORT: %d", received.data, received.src, received.ts);
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
				if (Conan::state == ConanState::WAIT_R)
					Conan::state = ConanState::TAKE_Z;
			}
		}
		else if (received.tag == REQ_S)
		{
			pkt->tag = ACK_S;
			pkt->src = Monitor::rank;
			pkt->data = Monitor::my_suits_counter;
			if ((Conan::state == ConanState::GET_S || Conan::state == ConanState::WAIT_S) && (Monitor::lamport < received.ts || (Monitor::lamport == received.ts && Monitor::rank < received.src)))
			{
				pkt->cc[0] = false;
				pkt->cc[1] = 1; // o tyle sie ubiegam
				debug("Conan: Odmawiam zgody (ACK_S) na przyjęcie stroju przez Conana: %d", received.src);
				pkt->ts = Monitor::incrementLamportOnSend();
				Monitor::sendMessage(pkt, received.src, ACK_S);
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
				if (!is_in)
				{
					pthread_mutex_lock(&Monitor::mutexQueueSuits);
					Monitor::queueForSuits.push_back(received);
					sort(Monitor::queueForSuits.begin(), Monitor::queueForSuits.end(),
						 prioritySortCriterion);
					pthread_mutex_unlock(&Monitor::mutexQueueSuits);
				}
				//debug("Conan: Udzielam zgodę (ACK_S) na przyjęcie stroju przez Conana: %d", received.src);
				pkt->ts = Monitor::incrementLamportOnSend();
				Monitor::sendMessage(pkt, received.src, ACK_S);
			}
		}
		else if (received.tag == ACK_S)
		{
			reply_counter_suits++;
			//Monitor::taken_suits += received.data;
			if (received.cc[0] == 0)
			{
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
					if (!is_in)
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
				pthread_mutex_lock(&Monitor::mutexTakenSuits);
				if (Monitor::taken_suits + my_suits_counter >= Monitor::SUITS)
				{
					pthread_mutex_unlock(&Monitor::mutexTakenSuits);
					debug("Conan: Stoję w kolejce po strój! LAMPORT: %d, zajęte stroje: %d, moje stroje: %d", received.ts, taken_suits, my_suits_counter);
					Conan::state = ConanState::WAIT_S;
				}
				else
				{
					if (Monitor::queueForSuits.front().src == rank)
					{
						pkt->tag = ACK_TS;
						pkt->src = Monitor::rank;
						pkt->ts = Monitor::incrementLamportOnSend();
						for (int i = 0; i < size; i++)
						{
							if (i == rank || !(i % 4))
								continue;
							Monitor::sendMessage(pkt, i, ACK_TS);
						}
						Monitor::deleteConanFromQueue(rank);
						taken_suits++;
						my_suits_counter++;
						pthread_mutex_unlock(&Monitor::mutexTakenSuits);
						debug("Conan: Biorę strój LAMPORT: %d, zajęte stroje: %d, moje stroje: %d", received.ts, taken_suits, my_suits_counter);
						Conan::state = ConanState::COMPLETE_Z;
					}
				}
			}
		}
		else if (received.tag == ACK_TS)
		{
			pthread_mutex_lock(&Monitor::mutexTakenSuits);
			taken_suits++;
			pthread_mutex_unlock(&Monitor::mutexTakenSuits);
			Monitor::deleteConanFromQueue(received.src);
			sort(Monitor::queueForSuits.begin(), Monitor::queueForSuits.end(),
				 prioritySortCriterion);
		}
		else if (received.tag == RELEASE_S)
		{
			pthread_mutex_lock(&Monitor::mutexTakenSuits);
			taken_suits--;
			pthread_mutex_unlock(&Monitor::mutexTakenSuits);
			if (Monitor::queueForSuits.front().src == rank && Conan::state == ConanState::WAIT_S && taken_suits + my_suits_counter < Monitor::SUITS)
			{
				pkt->tag = ACK_TS;
				pkt->src = Monitor::rank;
				pkt->ts = Monitor::incrementLamportOnSend();
				pthread_mutex_lock(&Monitor::mutexTakenSuits);
				for (int i = 0; i < size; i++)
				{
					if (i == rank || !(i % 4))
						continue;
					Monitor::sendMessage(pkt, i, ACK_TS);
				}
				Monitor::deleteConanFromQueue(rank);
				debug("Conan: RELEASE Biorę strój, LAMPORT: %d", received.ts);
				my_suits_counter++;
				taken_suits++;
				pthread_mutex_unlock(&Monitor::mutexTakenSuits);
				debug("Conan: RELEASE Liczba strojów: %d", taken_suits);
				Conan::state = ConanState::COMPLETE_Z;
			}
		}
		else if (received.tag == REQ_P)
		{
			pkt->tag = ACK_P;
			pkt->src = Monitor::rank;
			pkt->data = Monitor::my_laundry_counter;
			if ((Conan::state == ConanState::REPORT_Z || Conan::state == ConanState::WAIT_P) && (Monitor::lamport < received.ts || (Monitor::lamport == received.ts && Monitor::rank < received.src)))
			{
				pkt->cc[0] = false;
				pkt->cc[1] = 1; // o tyle sie ubiegam
				debug("Conan: Odmawiam zgody (ACK_P) na wejście do pralni Conana: %d", received.src);
				pkt->ts = Monitor::incrementLamportOnSend();
				Monitor::sendMessage(pkt, received.src, ACK_P);
			}
			else
			{
				pkt->cc[0] = true;
				pkt->cc[1] = 1; // o tyle sie ubiegam
				bool is_in = false;
				//sprawdzamy czy gosc nie jest w kolejce
				for (auto i = Monitor::queueForLaundry.begin(); i != Monitor::queueForLaundry.end();)
				{
					if (i._M_cur->src == received.src)
					{
						is_in = true;
						break;
					}
					else
						++i;
				}
				if (!is_in)
				{
					pthread_mutex_lock(&Monitor::mutexQueueLaundry);
					Monitor::queueForLaundry.push_back(received);
					sort(Monitor::queueForLaundry.begin(), Monitor::queueForLaundry.end(),
						 prioritySortCriterion);
					pthread_mutex_unlock(&Monitor::mutexQueueLaundry);
				}
				//debug("Conan: Udzielam zgodę (ACK_P) na wejście do pralni Conana: %d", received.src);
				pkt->ts = Monitor::incrementLamportOnSend();
				Monitor::sendMessage(pkt, received.src, ACK_P);
			}
		}
		else if (received.tag == ACK_P)
		{
			pthread_mutex_lock(&Monitor::mutexOccupiedLaundry);
			reply_counter_laundry++;
			pthread_mutex_unlock(&Monitor::mutexOccupiedLaundry);
			if (received.cc[0] == 0)
			{
				if (Monitor::queueForLaundry.empty())
				{
					pthread_mutex_lock(&Monitor::mutexQueueLaundry);
					Monitor::queueForLaundry.push_back(received);
					pthread_mutex_unlock(&Monitor::mutexQueueLaundry);
				}
				else
				{
					bool is_in = false;
					//sprawdzamy czy gosc nie jest w kolejce
					for (auto i = Monitor::queueForLaundry.begin(); i != Monitor::queueForLaundry.end();)
					{
						if (i._M_cur->src == received.src)
						{
							is_in = true;
							break;
						}
						else
							++i;
					}
					if (!is_in)
					{
						pthread_mutex_lock(&Monitor::mutexQueueLaundry);
						Monitor::queueForLaundry.push_back(received);
						sort(Monitor::queueForLaundry.begin(), Monitor::queueForLaundry.end(),
							 prioritySortCriterion);
						pthread_mutex_unlock(&Monitor::mutexQueueLaundry);
					}
				}
			}
			if (Monitor::reply_counter_laundry == Monitor::NUMBER_OF_CONANS - 1)
			{
				reply_counter_laundry = 0;
				if (Monitor::occupied_laundry >= Monitor::LAUNDRY)
				{
					debug("Conan: Stoję w kolejce do pralni! LAMPORT: %d, zajęte miejsca w pralni: %d, moje prania: %d", received.ts, occupied_laundry, my_laundry_counter);
					Conan::state = ConanState::WAIT_P;
				}
				else
				{
					if (Monitor::queueForLaundry.front().src == rank)
					{
						pkt->tag = ACK_TP;
						pkt->src = Monitor::rank;
						pkt->ts = Monitor::incrementLamportOnSend();
						for (int i = 0; i < size; i++)
						{
							if (i == rank || !(i % 4))
								continue;
							Monitor::sendMessage(pkt, i, ACK_TP);
						}
						Monitor::deleteConanFromLaundryQueue(rank);
						pthread_mutex_lock(&Monitor::mutexOccupiedLaundry);
						occupied_laundry++;
						pthread_mutex_unlock(&Monitor::mutexOccupiedLaundry);
						my_laundry_counter++;
						debug("Conan: Zajmuję miejsce w pralni LAMPORT: %d, zajęte miejsca w pralni: %d, moje prania: %d", received.ts, occupied_laundry, my_laundry_counter);
						Conan::state = ConanState::WASH_P;
					}
				}
			}
		}
		else if (received.tag == ACK_TP)
		{
			pthread_mutex_lock(&Monitor::mutexOccupiedLaundry);
			occupied_laundry++;
			pthread_mutex_unlock(&Monitor::mutexOccupiedLaundry);
			Monitor::deleteConanFromLaundryQueue(received.src);
			sort(Monitor::queueForLaundry.begin(), Monitor::queueForLaundry.end(),
				 prioritySortCriterion);
		}
		else if (received.tag == RELEASE_P)
		{
			pthread_mutex_lock(&Monitor::mutexOccupiedLaundry);
			occupied_laundry--;
			pthread_mutex_unlock(&Monitor::mutexOccupiedLaundry);
			if (Monitor::queueForLaundry.front().src == rank && Conan::state == ConanState::WAIT_P && occupied_laundry < Monitor::LAUNDRY)
			{
				pkt->tag = ACK_TP;
				pkt->src = Monitor::rank;
				pkt->ts = Monitor::incrementLamportOnSend();
				pthread_mutex_lock(&Monitor::mutexOccupiedLaundry);
				for (int i = 0; i < size; i++)
				{
					if (i == rank || !(i % 4))
						continue;
					Monitor::sendMessage(pkt, i, ACK_TP);
				}
				Monitor::deleteConanFromLaundryQueue(rank);
				debug("Conan: RELEASE Biorę miejsce w pralni, LAMPORT: %d", received.ts);
				my_laundry_counter++;
				occupied_laundry++;
				pthread_mutex_unlock(&Monitor::mutexOccupiedLaundry);
				debug("Conan: RELEASE Liczba zajętych miejsc w pralni: %d", occupied_laundry);
				Conan::state = ConanState::WASH_P;
			}
		}
	}
}
