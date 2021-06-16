#include "librarian.h"
#include "monitor.h"

LibrarianState Librarian::state = LibrarianState::WAIT_NC; //Bibliotekarz czeka na niesfornego czytelnika

void Librarian::loop(int size, int rank){
    debug("Jestę bibliotekarzę - %d", rank)
	packet_t *pkt = new packet_t;
    pkt->src = Monitor::rank;
    packet_t received;
	while(1){
        if (state == LibrarianState::WAIT_NC){
		    sleep(rand()%10); //bibliotekarz czeka losową ilość czasu na nowe zlecenie
            debug("Bibliotekarz: Pojawiło się nowe zlecenie!");

            int conans = 0;
            int chosenConans[Monitor::CONANTASKNUMBER];
            int target;
            int taskNumber = rand()%100;
            while (conans < Monitor::CONANTASKNUMBER) {
                target = rand()%size;
                if (target%4) {
                    bool alreadySended = false;
                    for (int i=0; i<conans; i++) {
                        if (chosenConans[i] == target){
                            alreadySended = true;
                            break;
                        }
                    }
                    if (!alreadySended){
                        chosenConans[conans] = target;
                        conans++;
                    }
                }
            }
            for (int i = 0; i < conans; i++){
                pkt->cc[i] = chosenConans[i];
            }
            for (int i = 0; i < conans; i++){
                pkt->data = taskNumber;
                pkt->tag = ACK_DZ;
                Monitor::sendMessage(pkt, chosenConans[i], 2);
                debug("Bibliotekarz: Wysłałem zlecenie o numerze: %d do Conana: %d", pkt->data, chosenConans[i]);
            }
        state = LibrarianState::WAIT_PZ;
        } else {
            debug("Bibliotekarz: Czekammmmm");
            sleep(10);
        }
        received = Monitor::receiveMessage();
        debug("Bibliotekarz: Otrzymałem wiadomość o treści: %d od Conana: %d", received.data, received.src);
    }

}