#include "blehc42.h"
#include <cstring>
#include <string>
#include <memory>
#include <chrono.hpp>
#include <thread.hpp>
#include <delegate.hpp>
#include "exception.h"

static int GetSecond() noexcept {
	using namespace rstd::chrono;
	return std::chrono::duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

static void ExpectHC42(Uart* x, const std::string& s) {
	const auto buf = std::unique_ptr<uint8_t[]>(new uint8_t[s.size() + 2]); // there is a CRLF ending
	x->ExpectBytes(buf.get(), s.size() + 2);
	if (std::memcmp(buf.get(), s.c_str(), s.size()) != 0) throw MachineException("HC42 EXPECT ERR");
	std::printf("[%6d]BLE:%s\n", GetSecond(), s.c_str());
}

void BleHC42::Reset(const char* name) {
	std::printf("[%6d]BLE:%s SI\n", GetSecond(), name);
	// Device Reset
	{
		SendBytes("AT+DEFAULT", 10);
		ExpectHC42(this, "OK+DEFAULT");
	}
	// Reset the device id
	{
		const auto sn = std::string(name);
		const auto cmd = "AT+NAME=" + sn;
		const auto exp = "OK+NAME=" + sn;
		SendBytes(cmd.c_str(), cmd.size());
		ExpectHC42(this, exp);
	}
	std::printf("[%6d]BLE:%s DI\n", GetSecond(), name);
}

void BArray::Start(Callback callback) {
	mCallback = callback;
	mStop = false;
	rstd::thread(osPriorityNormal, 1024, [this]() noexcept {
		{
			rstd::thread cL(osPriorityAboveNormal, 1024, [this]() noexcept { mC.Reset(mNC); });
			//rstd::thread rL(osPriorityAboveNormal, 1024, [this]() noexcept { mR.Reset(mNR); });
			//rstd::thread lL(osPriorityAboveNormal, 1024, [this]() noexcept { mL.Reset(mNL); });
			cL.join();
			//rL.join();
			//lL.join();
		}
		char cmd[2];
		mC.ExpectBytes(cmd, 2);
		mC.SendBytes("OK", 2);
		std::printf("[%6d]COM EST\n", GetSecond());
		while (!mStop) {
			mC.ExpectBytes(cmd, 2);
			if (cmd[0] == 'G') HandleCmdGet(cmd[1]);
			if (cmd[0] == 'C') HandleControl();
		}
	}).detach();
}

void BArray::HandleCmdGet(char prop) {
	std::printf("[%6d]CMD G%c\n", GetSecond(), prop);
	if (prop == 'L') mC.SendBytes(mNL, std::strlen(mNL));
	if (prop == 'R') mC.SendBytes(mNR, std::strlen(mNR));
}

void BArray::HandleControl() {
	int16_t v[3];
	mC.ExpectBytes(v, sizeof(v));
	std::printf("[%6d]CMD C%d|%d|%d\n", GetSecond(), v[0], v[1], v[2]);
	if (mCallback) rstd::run([this, v]() noexcept { mCallback(v[0], v[1], v[2]); });
}

void BArray::Stop() { mStop = true; }
