#include "test_framework.hpp"
#include "time/osTimeCore.hpp"
#include "uSliceCore.h"

extern "C" int c_api_symbols_are_declared(void);
extern "C" void c_call_tick(void);

int main() {
    CHECK(c_api_symbols_are_declared());

    const std::uint32_t before = OS::Time::Core::getSystemTick();
    osTickISR();
    CHECK_EQ(OS::Time::Core::getSystemTick(), before + 1u);
    c_call_tick();
    CHECK_EQ(OS::Time::Core::getSystemTick(), before + 2u);

    return TEST_SUMMARY("test_c_api");
}
