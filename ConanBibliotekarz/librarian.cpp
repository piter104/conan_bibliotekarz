#include "librarian.h"
#include "monitor.h"

void Librarian::loop(int size, int rank){
    debug("Jestę bibliotekarzę - %d", rank)
	packet_t *pkt = new packet_t;
    pkt->src = Monitor::rank;
    packet_t received;
	while(1){
		sleep(10);
        int target = rand()%4;
        pkt->data = rand()%100;
        Monitor::sendMessage(pkt, target, 2);
        debug("Wysłałem wiadomość o treści: %d do kolegi: %d", pkt->data, target);
        received = Monitor::receiveMessage();
        debug("Otrzymałem wiadomość o treści: %d od kolegi: %d", received.data, received.src);
        debug("Jestę bibliotekarzę - %d", rank)
    }

}