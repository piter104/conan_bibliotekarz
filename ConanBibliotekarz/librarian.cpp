#include "librarian.h"
#include "monitor.h"

LibrarianState Librarian::state = LibrarianState::WAIT_NC; //Bibliotekarz czeka na niesfornego czytelnika

void Librarian::loop(int size, int rank)
{
    debug("Jestę bibliotekarzę - %d, stan: WAIT_NC", rank);
    packet_t *pkt = new packet_t;
    pkt->src = Monitor::rank;
    packet_t received;
    while (1)
    {
        if (Librarian::state == LibrarianState::WAIT_NC)
        {
            sleep(rand() % 10); //bibliotekarz czeka losową ilość czasu na nowe zlecenie
            debug("Bibliotekarz: Pojawiło się nowe zlecenie!");
            Librarian::state = LibrarianState::WAIT_PZ;
            debug("Bibliotekarz: Proszę się dogadać kto wykona zlecenie. Jestem w stanie WAIT_PZ");

            int conans = 0;
            int chosenConans[Monitor::CONANTASKNUMBER];
            int target;
            int taskNumber = 100 + rank;
            while (conans < Monitor::CONANTASKNUMBER)
            {
                target = rand() % size;
                if (target % 4)
                {
                    bool alreadySended = false;
                    for (int i = 0; i < conans; i++)
                    {
                        if (chosenConans[i] == target)
                        {
                            alreadySended = true;
                            break;
                        }
                    }
                    if (!alreadySended)
                    {
                        chosenConans[conans] = target;
                        pkt->cc[conans] = chosenConans[conans];
                        conans++;
                    }
                }
            }
            pkt->ts = Monitor::incrementLamportOnSend();
            for (int i = 0; i < conans; i++)
            {
                pkt->data = taskNumber;
                pkt->tag = ACK_DZ;
                Monitor::sendMessage(pkt, chosenConans[i], ACK_DZ);
                debug("Bibliotekarz: Wysłałem zlecenie o numerze: %d do Conana: %d, lamport: %d", pkt->data, chosenConans[i], pkt->ts);
            }  
            
            Librarian::state = LibrarianState::WAIT_WZ;
        }
        else
        {
            debug("Bibliotekarz: Dalej Conanie! Czekam az skończysz misje. Jestem w stanie WAIT_WZ");
            while (1)
            {
                received = Monitor::receiveMessage();
                if (received.tag == ACK_WZ)
                {
                    debug("Bibliotekarz: Dzięki Conanie! Dobrze się spisałeś. Lecę ogarniać kolejnych niesfornych czytelników. LAMPORT: %d", received.ts);
                    Librarian::state = LibrarianState::WAIT_NC;
                    break;
                }
            }
        }
    }
}