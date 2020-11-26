#include "machine.h"
#include <delegate.hpp>

extern "C" MachineInit* GetInit();

extern "C" void rtmain(void *) {
    rstd::run([]() noexcept { Machine_Configure(GetInit()); });
    rstd::hold_indefinitely();
}
