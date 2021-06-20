#ifndef MONITOR_H
#define MONITOR_H

class Monitor
{
private:
	static bool listening;

public:
	const static int CONANTASKNUMBER = 3;
	const static int SUITS = 1;
	const static int NUMBER_OF_CONANS = 6;
	static unsigned int lamport;
	static pthread_mutex_t takeTaskMutex;
	static pthread_mutex_t lamportMutex;
	static int rank;
	static int size;
	static int reply_counter;
	static int reply_counter_suits;
	static int taken_suits;
	static int my_suits_counter;
	static int my_task;
	static int my_librarian;
	static int reply_wants_s;
	static deque<packet_t> queueTasks;
	static deque<packet_t> queueForSuits;

	static pthread_mutex_t mutexQueueTasks;
	static pthread_mutex_t mutexQueueSuits;

	static packet_t receiveMessage();
	static void sendMessage(packet_t *packet, int target, int tag);
	static void incrementLamportOnSend();
	static void incrementLamportOnReceive(packet_t packet);
	static void initMonitor();
	static void deleteTaskFromQueue(int data);
	static void deleteConanFromQueue(int data);
	static void listen();
	static bool prioritySortCriterion(packet_t conan1, packet_t conan2);
};
#endif
