#ifndef MONITOR_H
#define MONITOR_H

class Monitor
{
public:
	const static int CONANTASKNUMBER = 3;
	const static int SUITS = 3;
	const static int LAUNDRY = 1;

	static unsigned int lamport;
	static int NUMBER_OF_CONANS; 
	static pthread_mutex_t takeTaskMutex;
	static pthread_mutex_t lamportMutex;
	static int rank;
	static int size;
	static int reply_counter;

	static int reply_counter_suits;
	static int taken_suits;
	static int my_suits_counter;

	static int occupied_laundry;
	static int reply_counter_laundry;
	static int my_laundry_counter;

	static int my_task;
	static int my_librarian;
	static deque<packet_t> queueTasks;
	static deque<packet_t> queueForSuits;
	static deque<packet_t> queueForLaundry;

	static pthread_mutex_t mutexQueueTasks;
	static pthread_mutex_t mutexQueueSuits;
	static pthread_mutex_t mutexQueueLaundry;

	static pthread_mutex_t mutexTakenSuits;
	static pthread_mutex_t mutexOccupiedLaundry;

	static packet_t receiveMessage();
	static void sendMessage(packet_t *packet, int target, int tag);
	static unsigned int incrementLamportOnSend();
	static void incrementLamportOnReceive(packet_t packet);
	static void initMonitor();
	static void deleteTaskFromQueue(int data);
	static void deleteConanFromQueue(int data);
	static void deleteConanFromLaundryQueue(int data);
	static void listen();
	static bool prioritySortCriterion(packet_t conan1, packet_t conan2);
};
#endif
