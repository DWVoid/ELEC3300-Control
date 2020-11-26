#pragma once

#include "lazy.h"
#include <mutex.hpp>
#include <thread.hpp>

class Uart {
public:
    explicit Uart(UART_HandleTypeDef* hdc) noexcept;

    ~Uart() noexcept;

    void SendBytes(const void* data, size_t length);

    void ReceiveBytes(void* data, size_t length);

    void _CompletionIRQ() noexcept;
private:
    UART_HandleTypeDef *mHdc;
    rstd::mutex mLockTx {}, mLockRx {};
    rstd::thread::id mHoldTx {}, mHoldRx {};
};
