#ifndef GLOBALH
#define GLOBALH

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <queue>
#include <map>
#include <deque>
#include <algorithm>

using namespace std;

/* boolean */
#define TRUE 1
#define FALSE 0

extern int rank;
extern int size;

typedef struct
{
    unsigned int ts; /* timestamp (zegar lamporta */
    int src;         /* pole nie przesyłane, ale ustawiane w main_loop */
    int data;        /* przykładowe pole z danymi; można zmienić nazwę na bardziej pasującą */
    int tag;         // typ wiadomości
    int cc[3];
} packet_t;
extern MPI_Datatype MPI_PAKIET_T;

/* Typy wiadomości */
#define ACK_DZ 100    //tag nowe zlecenie
#define REQ_PZ 110    //tag request o zlecenie
#define ACK_PZ 120    //tag zgoda lub odmowa
#define ACK_Z 130     //tag info że zajal zlecenie
#define REQ_S 140     //tag prośba o strój
#define ACK_S 150     //tag info czy pozwalam na strój
#define ACK_TS 160    //tag info że wziąłem strój
#define ACK_WZ 170    //tag info że skończyłem zlecenie
#define RELEASE_S 180 //tag info że oddałem strój,
#define REQ_P 190 //tag request o miejsce w pralni
#define ACK_P 200 //tag info czy pozwalam na pralnie
#define ACK_TP 210 //tag info zająłem miejsce w pralni
#define RELEASE_P 220 //Info że zwolniłem miejsce w pralni


#ifdef DEBUG
#define debug(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, ##__VA_ARGS__, 27, 0, 37);
#else
#define debug(...) ;
#endif

#define P_WHITE printf("%c[%d;%dm", 27, 1, 37);
#define P_BLACK printf("%c[%d;%dm", 27, 1, 30);
#define P_RED printf("%c[%d;%dm", 27, 1, 31);
#define P_GREEN printf("%c[%d;%dm", 27, 1, 33);
#define P_BLUE printf("%c[%d;%dm", 27, 1, 34);
#define P_MAGENTA printf("%c[%d;%dm", 27, 1, 35);
#define P_CYAN printf("%c[%d;%d;%dm", 27, 1, 36);
#define P_SET(X) printf("%c[%d;%dm", 27, 1, 31 + (6 + X) % 7);
#define P_CLR printf("%c[%d;%dm", 27, 0, 37);

/* printf ale z kolorkami i automatycznym wyświetlaniem RANK. Patrz debug wyżej po szczegóły, jak działa ustawianie kolorków */
#define println(FORMAT, ...) printf("%c[%d;%dm [%d]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, ##__VA_ARGS__, 27, 0, 37);
#endif
