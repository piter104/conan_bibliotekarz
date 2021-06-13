#ifndef LIBRARIAN_H
#define LIBRARIAN_H
#include "main.h"

enum class LibrarianState{
	WAIT_NC, // oczekiwanie na niesfornego czytelnika
	WAIT_PZ, // oczekiwanie na przyjęcie zlecenia przez conana
	WAIT_WZ // oczekiwanie na wykonanie zlecenia
};

class Librarian {
	public:
        static void loop(int size, int rank);
		static LibrarianState state;
};

#endif