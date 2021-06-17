#ifndef MONITOR_H
#define MONITOR_H

class Monitor
{
private:
	static bool listening;

public:
	const static int CONANTASKNUMBER = 3;
	const static int SUITS = 3;
	static unsigned int lamport;
	static pthread_mutex_t takeTaskMutex;
	static pthread_mutex_t lamportMutex;
	static int rank;
	static int size;
	static int reply_counter;
	static int my_suits_counter;
	static int my_task;
	static deque<packet_t> queueTasks;
	static deque<int> queueForSuits;

	static pthread_mutex_t mutexQueueTasks;

	static packet_t receiveMessage();
	static void sendMessage(packet_t *packet, int target, int tag);
	static void incrementLamportOnSend();
	static void incrementLamportOnReceive(packet_t packet);
	static void initMonitor();
	static void deleteTaskFromQueue(int data);
	static void listen();
};
#endif
