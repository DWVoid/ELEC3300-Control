#include "uart.h"

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

extern "C" void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    auto slot = Slot(HdcId(huart));
    if (slot) slot->_CompletionIRQ();
}

Uart::Uart(UART_HandleTypeDef *hdc) noexcept: mHdc(hdc) { Slot(HdcId(hdc)) = this; }

Uart::~Uart() noexcept { Slot(HdcId(mHdc)) = nullptr; }

void Uart::SendBytes(const void* data, size_t length) {
	mLockTx.lock();
	HAL_UART_Transmit_DMA(
			mHdc,
			const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(data)),
			length
	);
	mHoldTx = rstd::this_thread::get_id();
	osThreadSuspend(mHoldTx.native());
}

void Uart::ReceiveBytes(void* data, size_t length) {
	mLockRx.lock();
	HAL_UART_Receive_DMA(mHdc, reinterpret_cast<uint8_t*>(data), length);
	mHoldRx = rstd::this_thread::get_id();
	osThreadSuspend(mHoldRx.native());
}

void Uart::_CompletionIRQ() noexcept {
	if (mHdc->gState == HAL_UART_STATE_READY && mHoldTx != rstd::thread::id()) {
		mLockTx.unlock();
		osThreadResume(mHoldTx.native());
		mHoldTx = rstd::thread::id();
	}
	if (mHdc->RxState == HAL_UART_STATE_READY && mHoldRx != rstd::thread::id()) {
		mLockRx.unlock();
		osThreadResume(mHoldRx.native());
		mHoldRx = rstd::thread::id();
	}
}
