#include "machine.h"
#include "lcd.h"
#include "motor.h"
#include "blehc42.h"
#include <cstdio>
#include <memory>
#include <FreeRTOS.h>
#include <thread.hpp>
#include <chrono.hpp>
#include <console.hpp>
#include <delegate.hpp>

/*
this should be placed in sysmem.c
int GetBRK(void) {
	  extern uint8_t _end;
	  return __sbrk_heap_end - &_end;
}
*/

extern "C" int GetBRK(void) noexcept;

class Machine {
public:
    explicit Machine(const MachineInit &init) noexcept:
            mLeftMotor(RunMotor(init.LeftMotor)), mRightMotor(RunMotor(init.RightMotor)),
			mBArr(nullptr) {
        LCD_INIT();
        mLeftMotor->SetTargetSpeed(0.3);
        mRightMotor->SetTargetSpeed(0.3);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
        mDebugDisplay = rstd::thread(osPriorityNormal, 512, [this]() noexcept {
        	for (;;) {
        		rstd::run([this]() noexcept { DisplayDebug(); });
        		rstd::this_thread::sleep_for(rstd::chrono::milliseconds(10));
        	}
        });
        rstd::console::init(30, 20 - 5);
        rstd::console::set_display_handler([](const char* text, int line) noexcept {
        	LCD_DrawString_Grid(0, 4 + line, text);
        });
    }

    void DisplayDebug() noexcept {
        mScan = 0;
        PrintClock();
        PrintMemStats();
        PrintMotorStats();
        rstd::console::flush();
    }

private:
    std::unique_ptr<Motor> mLeftMotor, mRightMotor;
    std::unique_ptr<BArray> mBArr;
    rstd::thread mDebugDisplay;
    char mLine[31];
    int mScan = 0;

    void PrintClock() noexcept {
    	using std::chrono::duration_cast;
    	using namespace rstd::chrono;
    	const auto now = system_clock::now();
    	const auto duration = now.time_since_epoch();
    	const int mill = duration_cast<milliseconds>(duration).count();
    	const int sec = mill / 1000;
    	const int min = sec / 60;
    	const int hour = min / 60;
    	std::snprintf(mLine, 30, "%d:%d:%d.%d", hour, min % 60, sec % 60, mill % 1000);
        LCD_DrawString_Grid(0, mScan++, mLine);
    }

    void PrintMemStats() noexcept {
    	const auto max = configTOTAL_HEAP_SIZE - xPortGetMinimumEverFreeHeapSize();
    	const auto now = configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize();
    	std::snprintf(
    			mLine, 30, "H%6d%%%3dN%6d%%%3dB%6d",
				max, max * 100 / configTOTAL_HEAP_SIZE, now, now * 100 / configTOTAL_HEAP_SIZE,
				GetBRK()
		);
        LCD_DrawString_Grid(0, mScan++, mLine);
    }

    void PrintMotorStats() noexcept {
    	std::snprintf(mLine, 30, " L%11f,  R%11f",
    			mLeftMotor->GetSpeed(), mRightMotor->GetSpeed()
		);
    	LCD_DrawString_Grid(0, mScan++, mLine);
    	std::snprintf(mLine, 30, "LX%11f, RX%11f",
    			mLeftMotor->GetScaler(), mRightMotor->GetScaler()
		);
    	LCD_DrawString_Grid(0, mScan++, mLine);
    	/*std::snprintf(mLine, 30, "QL%11u, QR%11u",
    			mLeftMotor->GetIrqCount(), mRightMotor->GetIrqCount()
		);
    	LCD_DrawString_Grid(0, mScan++, mLine);*/
    }
};

static Machine &GetInstance(MachineInit *init = nullptr) {
    static Machine instance{*init};
    return instance;
}

void Machine_Configure(MachineInit *init) {
    GetInstance(init);
}

