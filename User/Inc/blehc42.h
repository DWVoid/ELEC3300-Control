#pragma once

#include "uart.h"

class BleHC42: public Uart {
public:
	using Uart::Uart;

	void Reset(const char* name);
};

class BArray {
public:
	using Callback = void (*)(int32_t, int32_t, int32_t) noexcept;
	BArray(
			UART_HandleTypeDef* c,
			UART_HandleTypeDef* l,
			UART_HandleTypeDef* r,
			const char* nc,
			const char* nl,
			const char* nr
	): mC(c), mL(l), mR(r), mNC(nc), mNL(nl), mNR(nr) {}

	void Start(Callback callback);

	void Stop();
private:
	BleHC42 mC, mL, mR;
	const char* mNC, *mNL, *mNR;
	Callback mCallback;
	bool mStop;

	void SysInit();
	void HandleControl();
	void HandleCmdGet(char prop);
};
