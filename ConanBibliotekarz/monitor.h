#ifndef MONITOR_H
#define MONITOR_H


class Monitor {
	private:
		static bool listening;
	public:
		const static int CONANTASKNUMBER = 3;
    	static unsigned int lamport;
		static unsigned int takeTaskLamport;
		static pthread_mutex_t takeTaskMutex;
		static pthread_mutex_t lamportMutex;
        static int rank;
        static int size;
		static int reply_counter;
		static deque<packet_t> queueTasks;

		static pthread_mutex_t mutexQueueTasks;

		static packet_t receiveMessage();
		static void sendMessage(packet_t *packet, int target, int tag);
		static void incrementLamportOnSend();
		static void incrementLamportOnReceive(packet_t packet);
		static unsigned int getLamport();
		static void initMonitor();
		static void listen();
};
#endif
