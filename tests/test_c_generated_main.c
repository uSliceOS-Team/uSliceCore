/**
 * @file test_c_generated_main.c
 * @brief A plain-C main drives one pass over a generated C++ task registry.
 *
 * A production C main can call the generated usliceTaskManager() without
 * knowing anything about the C++ task registry. The bounded pass keeps this
 * host-side integration test finite while exercising the generated C API.
 */

#include "generated_manager.h"

int main(void) {
    usliceTaskManagerPass();
    return 0;
}
