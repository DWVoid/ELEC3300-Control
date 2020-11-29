#include "blehc42.h"
#include "exception.h"
#include <memory>
#include <string>
#include <cstring>
#include <chrono.hpp>
#include <thread.hpp>
#include <delegate.hpp>

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
		rstd::this_thread::sleep_for(rstd::chrono::microseconds(300));
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

void BArray::DevRst() {
	rstd::thread cL(osPriorityAboveNormal, 1024, [this]() noexcept { mC.Reset(mNC); });
	rstd::thread rL(osPriorityAboveNormal, 1024, [this]() noexcept { mR.Reset(mNR); });
	rstd::thread lL(osPriorityAboveNormal, 1024, [this]() noexcept { mL.Reset(mNL); });
	cL.join();
	rL.join();
	lL.join();
}

void BArray::Start(Callback callback, void* user) {
	mCallback = callback;
	mCbUser = user;
	mStop = false;
	rstd::thread(osPriorityNormal, 1024, [this]() noexcept {
		DevRst();
		while (!mStop) ExpectCmd();
	}).detach();
}

void BArray::ExpectCmd() noexcept {
	char cmd[2];
	mC.ExpectBytes(cmd, 2);
	if (cmd[0] == 'O' && cmd[1] == 'K') { // CMD RESET
		std::printf("[%6d]CMD RST\n", GetSecond());
		IssueCommand(0, 0, 0);
		mC.SendBytes("OK", 2);
	}
	if (cmd[0] == 'G') HandleCmdGet(cmd[1]); // CMD GET
	if (cmd[0] == 'C') HandleControl(); // CMD CMD
}

void BArray::HandleCmdGet(char prop) noexcept {
	std::printf("[%6d]CMD G%c\n", GetSecond(), prop);
	if (prop == 'L') mC.SendBytes(mNL, std::strlen(mNL));
	if (prop == 'R') mC.SendBytes(mNR, std::strlen(mNR));
}

void BArray::HandleControl() noexcept {
	int16_t v[3];
	mC.ExpectBytes(v, sizeof(v));
	std::printf("[%6d]CMD C%d|%d|%d\n", GetSecond(), v[0], v[1], v[2]);
	IssueCommand(v[0], v[1], v[2]);
}

void BArray::IssueCommand(int32_t x, int32_t y, int32_t z) noexcept {
	if (mCallback) rstd::run([=]() noexcept { mCallback(x, y, z, mCbUser); });
}

void BArray::Stop() { mStop = true; }
