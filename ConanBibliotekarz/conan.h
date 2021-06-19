#ifndef CONAN_H
#define CONAN_H
#include "main.h"

enum class ConanState{
WAIT_Z, //oczekiwanie na zlecenie
TAKE_Z, // przyjmowanie zlecenia
WAIT_S, //oczekiwanie na wolny strój
GET_S, //wypożyczenie stroju
COMPLETE_Z, //wykonanie zlecenia
REPORT_Z, //oddanie zlecenia
WAIT_P, //oczekiwanie na wejście do pralni
RETURN_S //zwrot stroju do magazynu
};

class Conan {
	public:
        static void loop(int size, int rank);
		static ConanState state;
};

void *listenForNewTasks (void* x);

#endif