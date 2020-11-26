#include <FreeRTOS.h>
#include <new>
#include <cstdint>

namespace {
    void *allocA(std::size_t size) noexcept { return pvPortMalloc(size); }

    void *allocB(std::size_t size) {
        const auto x = allocA(size);
        if (!x) throw std::bad_alloc(); else return x;
    }

    void free(void *ptr) { vPortFree(ptr); }

#if __cpp_aligned_new
    void *allocAA(std::size_t size, std::align_val_t align) noexcept {
        const auto ali = static_cast<size_t>(align);
        if (ali <= portBYTE_ALIGNMENT) return allocA(size);
        const auto z = reinterpret_cast<uintptr_t>(allocA(size + ali));
        const auto x = z + ali;
        const auto x2 = x - x % ali;
        *reinterpret_cast<uint8_t *>(x2 - 1) = x2 - z;
        return reinterpret_cast<void *>(x2);
    }

    void *allocAB(std::size_t size, std::align_val_t align) noexcept {
        const auto x = allocAA(size, align);
        if (!x) throw std::bad_alloc(); else return x;
    }

    void freeA(void *ptr, std::align_val_t align) noexcept {
        const auto ali = static_cast<size_t>(align);
        if (ali <= portBYTE_ALIGNMENT) free(ptr);
        if (ptr) {
            const auto x2 = reinterpret_cast<uintptr_t>(ptr);
            const auto z = x2 - (*reinterpret_cast<uint8_t *>(x2 - 1));
            free(reinterpret_cast<void *>(z));
        }
    }
#endif
}

void *operator new(std::size_t size) { return allocB(size); }

void *operator new[](std::size_t size) { return allocB(size); }

void operator delete(void *ptr) noexcept { free(ptr); }

void operator delete[](void *ptr) noexcept { free(ptr); }

void operator delete(void *ptr, std::size_t) noexcept { free(ptr); }

void operator delete[](void *ptr, std::size_t) noexcept { free(ptr); }

void *operator new(std::size_t size, const std::nothrow_t &) noexcept { return allocA(size); }

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept { return allocA(size); }

void operator delete(void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

void operator delete[](void *ptr, const std::nothrow_t &) noexcept { free(ptr); }

#if __cpp_aligned_new
void *operator new(std::size_t size, std::align_val_t align) { return allocAB(size, align); }

void *operator new(std::size_t size, std::align_val_t align, const std::nothrow_t &) noexcept {
    return allocAA(size, align);
}

void operator delete(void *ptr, std::align_val_t align) noexcept { freeA(ptr, align); }

void operator delete(void *ptr, std::align_val_t align, const std::nothrow_t &) noexcept { freeA(ptr, align); }

void *operator new[](std::size_t size, std::align_val_t align) { return allocAB(size, align); }

void *operator new[](std::size_t size, std::align_val_t align, const std::nothrow_t &) noexcept {
    return allocAA(size, align);
}

void operator delete[](void *ptr, std::align_val_t align) noexcept { freeA(ptr, align); }

void operator delete[](void *ptr, std::align_val_t align, const std::nothrow_t &) noexcept { freeA(ptr, align); }

void operator delete(void *ptr, std::size_t size, std::align_val_t align) noexcept { freeA(ptr, align); }

void operator delete[](void *ptr, std::size_t size, std::align_val_t align) noexcept { freeA(ptr, align); }
#endif
