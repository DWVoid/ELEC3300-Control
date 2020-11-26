#include <mutex.hpp>

namespace rstd {
    osMutexAttr_t &_mutex_base::GetConfigA() noexcept {
        static osMutexAttr_t attr = {
                .name = nullptr,
                .attr_bits = osMutexPrioInherit | osMutexRobust,
                .cb_mem = nullptr,
                .cb_size = 0
        };
        return attr;
    }

    osMutexAttr_t &_mutex_base::GetConfigB() noexcept {
        static osMutexAttr_t attr = {
                .name = nullptr,
                .attr_bits = osMutexPrioInherit | osMutexRobust,
                .cb_mem = nullptr,
                .cb_size = 0
        };
        return attr;
    }
}