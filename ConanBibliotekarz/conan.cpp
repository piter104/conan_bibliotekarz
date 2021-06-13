#include "conan.h"
#include "monitor.h"
#include <iostream>

ConanState state = ConanState::WAIT_Z;

void Conan::loop(int size, int rank){
	while(1){
        sleep(10);
        debug("Jestę Conanę - %d", rank);
	}
}