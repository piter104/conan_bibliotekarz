#ifndef MONITOR_H
#define MONITOR_H

class Monitor {
	private:
		static bool listening;
	public:
    	static unsigned int lamport;
		static pthread_mutex_t lamportMutex;
        static int rank;
        static int size;

		static packet_t receiveMessage();
		static void sendMessage(packet_t *packet, int target, int tag);
		static void incrementLamportOnSend();
		static void incrementLamportOnReceive(packet_t packet);
		static unsigned int getLamport();
		static void initMonitor();
		static void listen();
};
#endif
