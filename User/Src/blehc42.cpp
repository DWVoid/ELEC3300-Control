#include "blehc42.h"
#include <cstring>
#include <delegate.hpp>

void BleHC42::Reset(const char* name) {
	// Device Reset
	{
		char uu[10];
		SendBytes("AT+DEFAULT", 10);
		ReceiveBytes(uu, 10);
	}
	// Enable iBeacon
	{
		char uu[9];
		SendBytes("AT+IBEN=1", 9);
		ReceiveBytes(uu, 9);
	}
	// Reset the device id
	{
		int len = std::strlen(name);
		SendBytes("AT+NAME=", 8);
		SendBytes(name, len);
		char uu[20];
		ReceiveBytes(uu, 8 + len);
	}
}

void BArray::SysInit() {
	mC.Reset(mNC);
	mR.Reset(mNR);
	mL.Reset(mNL);
}

void BArray::Start(Callback callback) {
	mCallback = callback;
	mStop = false;
	rstd::thread(osPriorityNormal, 512, [this]() noexcept {
		mC.SendBytes("OK", 2);
		while (!mStop) {
			char cmd[2];
			mC.ReceiveBytes(cmd, 2);
			if (cmd[0] == 'G') HandleCmdGet(cmd[1]);
			if (cmd[1] == 'C') HandleControl();
		}
	}).detach();
}

void BArray::HandleCmdGet(char prop) {
	if (prop == 'L') mC.SendBytes(mNL, std::strlen(mNL));
	if (prop == 'R') mC.SendBytes(mNR, std::strlen(mNR));
}

void BArray::HandleControl() {
	int16_t v[3];
	mC.ReceiveBytes(v, sizeof(v));
	if (mCallback) rstd::run([this, v]() noexcept { mCallback(v[0], v[1], v[2]); });
}

void BArray::Stop() { mStop = true; }
