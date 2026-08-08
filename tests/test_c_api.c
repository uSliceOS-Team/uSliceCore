#include "tasks/osTaskManager.h"
#include "time/osTickISR.h"
#include "uSliceCore.h"

int c_api_symbols_are_declared(void) {
    void (*manager)(void) = osTaskManager;
    void (*tick)(void) = osTickISR;
    return manager != 0 && tick != 0;
}

void c_call_tick(void) { osTickISR(); }
