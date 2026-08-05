/**
 * @file osTickISR.h
 * @brief C-linkage declaration of the millisecond tick hook.
 *
 * Implemented in time/osTickISR.cpp as a thin bridge to
 * OS::Time::Core::onTickISR() (time/osTimeCore.hpp), so a plain C
 * ISR file (e.g. a vendor-generated SysTick_Handler.c) can drive the
 * tick without including any C++ headers.
 *
 * @author uSliceCore Team
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Advances the OS's millisecond tick by one.
 *
 * Call from a hardware timer interrupt firing at exactly 1 kHz
 * (SysTick_Handler or equivalent). Every duration in Timer/Clock
 * (time/osTime.hpp) assumes one tick equals one millisecond; any
 * other rate makes them silently wrong.
 */
void osTickISR(void);

#ifdef __cplusplus
}
#endif
