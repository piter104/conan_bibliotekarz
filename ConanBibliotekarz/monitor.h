#ifndef MONITOR_H
#define MONITOR_H

class Monitor {
	public:
    	static unsigned int lamport;
		static pthread_mutex_t lamportMutex;
        static int rank;
        static int size;

		static void incrementLamportOnSend();
		static void incrementLamportOnReceive(packet_t packet);
		static unsigned int getLamport();
		static void initMonitor();
};
#endif
