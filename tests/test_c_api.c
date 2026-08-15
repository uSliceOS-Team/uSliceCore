#include "time/osTickISR.h"
#include "uSliceCore.h"

int c_api_symbols_are_declared(void) {
    void (*tick)(void) = osTickISR;
    return tick != 0;
}

void c_call_tick(void) { osTickISR(); }
