#include "lcd.h"
#include "motor.h"
#include "blehc42.h"
#include "machine.h"
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

static uint32_t gIdleCounter;

extern "C" void vApplicationIdleHook(void)
{
   /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   to 1 in FreeRTOSConfig.h. It will be called on each iteration of the idle
   task. It is essential that code added to this hook function never attempts
   to block in any way (for example, call xQueueReceive() with a block time
   specified, or call vTaskDelay()). If the application makes use of the
   vTaskDelete() API function (as this demo application does) then it is also
   important that vApplicationIdleHook() is permitted to return to its calling
   function, because it is the responsibility of the idle task to clean up
   memory allocated by the kernel to any task that has since been deleted. */
	++gIdleCounter;
}

static constexpr double Pi = 3.1415926535897932384;

extern "C" int GetBRK(void) noexcept;

class Machine {
public:
    explicit Machine(const MachineInit &init) noexcept:
            mLeftMotor(RunMotor(init.LeftMotor)), mRightMotor(RunMotor(init.RightMotor)),
			mBArr(nullptr) {
    	LateInit(init);
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

    void LateInit(const MachineInit &init) noexcept {
    	AllocateConsole();
    	InitPeriferals(init);
    	MotorSetStartup();
    	DebugLoopStart();
    }

    void AllocateConsole() noexcept {
    	LCD_INIT();
    	rstd::console::init(30, 20 - 5);
    	rstd::console::set_display_handler([](const char* text, int line) noexcept {
    		LCD_DrawString_Grid(0, 4 + line, text);
    		rstd::this_thread::yield();
    	});
    }

    void RunCmdControl(int32_t x, int32_t y, int32_t z) noexcept {
		const auto rx = static_cast<double>(x) / 1000.0;
		const auto ry = static_cast<double>(y) / 1000.0;
		const auto angular = [&]() noexcept -> double {
    		// fast exit for two special situations
			if (y == 0) {
				if (x > 0) return 0.5 * Pi;
				if (x < 0) return 1.5 * Pi;
				return 0.0;
			}
    		// convert the relative coordinated back to floating point
			const auto baseAng = std::atan(rx / ry);
			return (y > 0)
					? ((x >= 0) ? baseAng : (2 * Pi + baseAng))
					: Pi + baseAng;
		};
		const auto setPower = [&, this](double A, double B, bool invert) noexcept {
			if (invert) {
				mLeftMotor->SetTargetSpeed(A);
				mRightMotor->SetTargetSpeed(B);
			}
			else {
				mLeftMotor->SetTargetSpeed(B);
				mRightMotor->SetTargetSpeed(A);
			}
		};
		const auto eval = [&, this]() noexcept {
    		const auto distance2 = rx * rx + ry * ry;
    		if (distance2 < 0.25) {
    			// Speed set (0, 0), distance too short
    	        mLeftMotor->SetTargetSpeed(0.0);
    	        mRightMotor->SetTargetSpeed(0.0);
    	        return;
    		}
			const auto angle = angular();
			const auto angleNav = ((angle > Pi) ? (angle - 2 * Pi) : angle);
			const auto detour = std::max(-1.0, std::min(1.0, std::abs(angleNav * 2 / Pi)));
			// determine the overall driving power
			const auto power = (distance2 > 1) ? 0.5 : (std::sqrt(distance2) - 0.5);
			if (detour < 0.1) {
				// 10% turning black zone
    	        mLeftMotor->SetTargetSpeed(power);
    	        mRightMotor->SetTargetSpeed(power);
			}
			else {
				// determine the turning radius with the logarithmic curve
				const auto radius = -std::log2(detour);
				static constexpr auto rH = 0.1865;
				if (radius > rH) {
					const auto A = std::min(0.7, power / radius * (radius + rH));
					const auto B = std::min(0.7, power / radius * (radius - rH));
					setPower(A, B, angleNav > 0.0);
				}
				else {
					const auto A = std::min(0.7, power);
					const auto B = std::max(-0.7, -power / (rH + radius) * (rH - radius));
					setPower(A, B, angleNav > 0.0);
				}
			}
		};
		eval();
	}

    void InitPeriferals(const MachineInit &init) noexcept {
    	mBArr = std::make_unique<BArray>(
    			init.BArray.Center,
    			init.BArray.Left,
    			init.BArray.Right,
    			init.BArray.CenterName,
    			init.BArray.LeftName,
    			init.BArray.RightName
    	);
    	mBArr->Start([](auto x, auto y, auto z, void* u) noexcept {
    		reinterpret_cast<Machine*>(u)->RunCmdControl(x, y, z);
    	}, reinterpret_cast<void*>(this));
    }

    void MotorSetStartup() noexcept {
    	// Set Init Speed
        mLeftMotor->SetTargetSpeed(0.0);
        mRightMotor->SetTargetSpeed(0.0);
        // 24V Enable
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
    }

    void DebugLoopStart() noexcept {
    	 mDebugDisplay = rstd::thread(osPriorityBelowNormal, 512, [this]() noexcept {
    		 for (;;) {
    			 rstd::run([this]() noexcept { DisplayDebug(); });
    			 rstd::this_thread::sleep_for(rstd::chrono::milliseconds(10));
    		 }
    	 });
    }

    void PrintClock() noexcept {
    	using std::chrono::duration_cast;
    	using namespace rstd::chrono;
    	const auto now = system_clock::now();
    	const auto duration = now.time_since_epoch();
    	const int mill = duration_cast<milliseconds>(duration).count();
    	const int sec = mill / 1000;
    	const int min = sec / 60;
    	const int hour = min / 60;
    	std::snprintf(mLine, 30, "%d:%2d:%2d.3%d|%10ldT", hour, min % 60, sec % 60, mill % 1000, gIdleCounter);
    	gIdleCounter = 0;
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
    }
};

static Machine &GetInstance(MachineInit *init = nullptr) {
    static Machine instance{*init};
    return instance;
}

void Machine_Configure(MachineInit *init) {
    GetInstance(init);
}

