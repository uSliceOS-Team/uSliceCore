/**
 * @file osTickISR.cpp
 * @brief C-linkage bridge for the millisecond tick.
 *
 * OS::Time::Core::onTickISR() is a C++ class member and cannot be
 * called from a plain C translation unit (e.g. a vendor-generated
 * SysTick_Handler.c). This file gives it a C-callable name, declared
 * in time/osTickISR.h right next to it, so a C ISR file can drive the
 * tick without including any C++ headers.
 *
 * @author uSliceOS Team
 */

#include "osTickISR.h"
#include "osTimeCore.hpp"

extern "C" void osTickISR(void) {
  OS::Time::Core::onTickISR();
}
