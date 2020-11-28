#include "uart.h"
#include <delegate.hpp>

namespace {
    Uart *&Slot(int id) {
        static Uart *slots[5]{nullptr};
        return slots[id];
    }

    int HdcId(UART_HandleTypeDef *hdc) noexcept {
        const auto ins = hdc->Instance;
        if (ins == USART1) return 0;
        if (ins == USART2) return 1;
        if (ins == USART3) return 2;
        if (ins == UART4) return 3;
        if (ins == UART5) return 4;
        return -1;
    }
}

extern "C" void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart) {
    auto slot = Slot(HdcId(huart));
    if (slot) slot->_CompletionIRQ(-1);
}

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
	// We cannot really do much here. UART does not have conjestion control
    auto slot = Slot(HdcId(huart));
    if (slot) slot->_CompletionIRQ(256);
}

extern "C" void USER_UART_IRQHandler(UART_HandleTypeDef *huart) {
	if(RESET != __HAL_UART_GET_FLAG(huart, UART_FLAG_IDLE)) {
	    __HAL_UART_CLEAR_IDLEFLAG(huart);
		const auto loc = 256 - __HAL_DMA_GET_COUNTER(huart->hdmarx);
	    auto slot = Slot(HdcId(huart));
	    if (slot) slot->_CompletionIRQ(loc);
	}
}

Uart::Uart(UART_HandleTypeDef *hdc) noexcept: mHdc(hdc) {
	Slot(HdcId(hdc)) = this;
	// Start the DMA RX immediately
	HAL_UART_Receive_DMA(mHdc, reinterpret_cast<uint8_t*>(mRxB.B), sizeof(mRxB.B));
}

Uart::~Uart() noexcept { Slot(HdcId(mHdc)) = nullptr; }

void Uart::SendBytes(const void* data, size_t length) {
	mLockTx.lock();
	HAL_UART_Transmit_DMA(
			mHdc,
			const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data)),
			length
	);
	mHoldTx.acquire();
}

void Uart::ExpectBytes(void* data, size_t length) {
	int done = 0;
	for (;;) {
		done += ReadBytes(reinterpret_cast<uint8_t*>(data) + done, length - done);
		if (done == length) return;
		mHoldRx.acquire();
	}
}

int Uart::ReadBytes(void* data, size_t length) {
	return mRxB.Read(reinterpret_cast<uint8_t*>(data), length);
}

void Uart::_CompletionIRQ(int bytes) noexcept {
	if (bytes < 0) {
		mLockTx.unlock();
		mHoldTx.release();
	}
	else {
		mRxB.Advance(bytes);
		mHoldRx.release(); // We have some data
	}
}
