#include "main.h"
#include "monitor.h"
#include "librarian.h"
#include "conan.h"
/* wątki */
#include <pthread.h>

MPI_Datatype MPI_PAKIET_T;

void check_thread_support(int provided)
{
    // printf("THREAD SUPPORT: chcemy %d. Co otrzymamy?\n", provided);
    switch (provided)
    {
    case MPI_THREAD_SINGLE:
        printf("Brak wsparcia dla wątków, kończę\n");
        /* Nie ma co, trzeba wychodzić */
        // fprintf(stderr, "Brak wystarczającego wsparcia dla wątków - wychodzę!\n");
        MPI_Finalize();
        exit(-1);
        break;
    case MPI_THREAD_FUNNELED:
        // printf("tylko te wątki, ktore wykonaly mpi_init_thread mogą wykonać wołania do biblioteki mpi\n");
        break;
    case MPI_THREAD_SERIALIZED:
        /* Potrzebne zamki wokół wywołań biblioteki MPI */
        // printf("tylko jeden watek naraz może wykonać wołania do biblioteki MPI\n");
        break;
    case MPI_THREAD_MULTIPLE:
        // printf("Pełne wsparcie dla wątków\n"); /* tego chcemy. Wszystkie inne powodują problemy */
        break;
    default:
        printf("Nikt nic nie wie\n");
    }
}

/* srprawdza, czy są wątki, tworzy typ MPI_PAKIET_T
*/
void inicjuj(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    check_thread_support(provided);

    const int nitems = 5;
    int blocklengths[5] = {1, 1, 1, 1, Monitor::CONANTASKNUMBER};
    MPI_Datatype typy[5] = {MPI_UNSIGNED, MPI_INT, MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint offsets[5];
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);
    offsets[3] = offsetof(packet_t, tag);
    offsets[4] = offsetof(packet_t, cc);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &MPI_PAKIET_T);
    MPI_Type_commit(&MPI_PAKIET_T);
}

/* zwalnia przydzielony typ MPI_PAKIET_T
   wywoływane w funkcji main przed końcem
*/
void finalizuj()
{
    MPI_Type_free(&MPI_PAKIET_T);
    MPI_Finalize();
}

int main(int argc, char **argv)
{
    /* Tworzenie wątków, inicjalizacja itp */
    inicjuj(&argc, &argv); // tworzy wątek
    Monitor::initMonitor();

    // Pasowanie na Conana bądź Bibliotekarza
    if (Monitor::rank % 4)
    {
        Conan::loop(Monitor::size, Monitor::rank);
    }
    else
    {
        Librarian::loop(Monitor::size, Monitor::rank);
    }
    finalizuj();
    return 0;
}
