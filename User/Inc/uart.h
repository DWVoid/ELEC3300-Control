#pragma once

#include "lazy.h"
#include <mutex.hpp>
#include <semaphore.hpp>

// Note: Although there is a build-in rx buffer of a decent length, you should still always consume
// the data as fast as possible or the buffer will be corrputed
class Uart {
public:
    explicit Uart(UART_HandleTypeDef* hdc) noexcept;

    ~Uart() noexcept;

    void SendBytes(const void* data, size_t length);

    // This function will block until length bytes are received
    void ExpectBytes(void* data, size_t length);

    // This function will return with whatever is available
    int ReadBytes(void* data, size_t length);

    void _CompletionIRQ(int bytes) noexcept;
private:
    struct CirBuf {
    	uint8_t B[0x100];
    	uint16_t H {0}, T{0};

    	int Read(uint8_t* data, int length) noexcept {
    		uint32_t _h = H;
    		uint32_t _t = T;
    		if (_t < _h) _t += 0x10000;
    		int i = 0;
    		for (;i < length && _h < _t; ++i, ++_h) data[i] = B[_h & 0xFF];
    		H = _h & 0xFFFF;
    		return i;
    	}

    	void Advance(int loc) noexcept {
    		T = (uint32_t(T & 0xFF00) + loc) & 0xFFFF;
    	}
    };
    UART_HandleTypeDef *mHdc;
    rstd::mutex mLockTx {};
    rstd::semaphore mHoldTx {0x7ffffff, 0}, mHoldRx {0x7ffffff, 0};
    CirBuf mRxB{};
};
