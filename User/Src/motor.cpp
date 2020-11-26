#include "motor.h"
#include "exception.h"
#include <memory>
#include <chrono.hpp>
#include <thread.hpp>
#include <delegate.hpp>
#include <semaphore.hpp>

namespace {
    unsigned int ConvertDividerSTM32F103(uint32_t raw) noexcept {
        switch (raw) {
            case RCC_HCLK_DIV1:
                return 1;
            case RCC_HCLK_DIV2:
                return 2;
            case RCC_HCLK_DIV4:
                return 4;
            case RCC_HCLK_DIV8:
                return 8;
            case RCC_HCLK_DIV16:
                return 16;
            default:
                return -1; // should never reach here
        }
    }

    uintptr_t nvicTbl[8];

    int Get_STM32F103_TIM_ID(TIM_HandleTypeDef* htim) noexcept {
    	if (htim->Instance == TIM1) return 0;
    	if (htim->Instance == TIM2) return 1;
    	if (htim->Instance == TIM3) return 2;
    	if (htim->Instance == TIM4) return 3;
    	if (htim->Instance == TIM5) return 4;
    	if (htim->Instance == TIM6) return 5;
    	if (htim->Instance == TIM7) return 6;
    	if (htim->Instance == TIM8) return 7;
    	return -1;
    }

    int GetTIM_APB_STM32F103(TIM_TypeDef *instance) noexcept { return (instance == TIM1 || instance == TIM8) ? 2 : 1; }

    double Get_TIM_PCKL_Freq_STM32F103(int apb) noexcept {
        RCC_ClkInitTypeDef clkCfg;
        uint32_t flashLatUU;
        HAL_RCC_GetClockConfig(&clkCfg, &flashLatUU);
        const auto baseFreq = HAL_RCC_GetHCLKFreq();
        const auto divider = ConvertDividerSTM32F103((apb == 1) ? clkCfg.APB1CLKDivider : clkCfg.APB2CLKDivider);
        const auto TIMDivider = (divider == 1) ? 1 : (divider >> 1u);
        return static_cast<double>(baseFreq) / TIMDivider;
    }

    class Meter {
    public:
        explicit Meter(const MotorInit &init, const double sysFreq = 1000.0 / HAL_GetTickFreq()) noexcept
                : mHdc{init.Com.TIM_FG}, mChannel{init.Com.TIM_FG_Channel},
                  mFreqBus{Get_TIM_PCKL_Freq_STM32F103(GetTIM_APB_STM32F103(mHdc->Instance)) / (mHdc->Init.Prescaler + 1) },
                  mFreqTim{mFreqBus / (mHdc->Init.Period + 1)},
                  mPropTickTim{sysFreq / mFreqTim}, mPropTimTick{mFreqTim / sysFreq},
                  mWheelCircumference{3.1415926535897932384 * init.Meter.WheelDiameter},
                  mMaxPeriod{static_cast<unsigned int>(init.Meter.MaxMeasurePeriod)},
                  mMaxPeriodByTick{static_cast<unsigned int>(
                                           static_cast<double>(mMaxPeriod) / (mHdc->Init.Period + 1) * mPropTickTim
                                   )},
                  mLastTimer{0}, mLastTick{HAL_GetTick()}, mLastSpeed{0.0} {
        }

        double GetSpeed() const noexcept { return ((HAL_GetTick() - mLastTick) <= mMaxPeriodByTick) ? mLastSpeed : 0; }

        void NVICSetup() noexcept {
            nvicTbl[Get_STM32F103_TIM_ID(mHdc)] = reinterpret_cast<uintptr_t>(this);
            mHdc->IC_CaptureCallback = [](TIM_HandleTypeDef* hdc) noexcept {
            	const auto ths = reinterpret_cast<Meter*>(nvicTbl[Get_STM32F103_TIM_ID(hdc)]);
            	++(ths->mIrqCount);
            	ths->Measure();
            };
            HAL_TIM_IC_Start_IT(mHdc, mChannel);
        }

        auto GetIrqCount() const noexcept { return mIrqCount; }
    private:
        // Timer Information
        TIM_HandleTypeDef *const mHdc;
        const uint32_t mChannel;
        // Constants
        const double mFreqBus, mFreqTim, mPropTickTim, mPropTimTick, mWheelCircumference;
        const unsigned int mMaxPeriod, mMaxPeriodByTick;
        // Measurements
        uint32_t mLastTimer, mLastTick;
        double mLastSpeed;
        // Debug
        uint32_t mIrqCount {0};

        // note that for our use case a second wrap is highly unlikely,
        // as even with the 1K clock frequency the system tick counter will take over a year to overflow
        // thus, we do not care about the sys tick wrapping
        void Measure() noexcept {
            const auto timer = HAL_TIM_ReadCapturedValue(mHdc, mChannel);
            const auto tick = HAL_GetTick();
            // augment the current measurement
            const auto extraCycle = static_cast<int>(mPropTimTick * (tick - mLastTick));
            const auto timerAugmented = timer + (mHdc->Init.Period + 1) * extraCycle;
            // fix for a rounding problem
            const auto period = ((timerAugmented < mLastTimer)
                                 ? (timerAugmented + mHdc->Init.Period + 1) : timerAugmented) - mLastTimer;
            const auto frequency = ((tick - mLastTick) <= mMaxPeriodByTick) ? mFreqBus / period : 0.0;
            const auto rps = frequency / 19.0 / 6.0;
            mLastSpeed = mWheelCircumference * rps;
            mLastTimer = timer;
            mLastTick = tick;
        }
    };

    class PWMDriver {
    public:
        explicit PWMDriver(const MotorInit &init)
                : mHdc(init.Com.TIM_PWM), mChannel(init.Com.TIM_PWM_Channel), mInvert(init.Drive.Reverse),
				  mDirT(init.Com.GPIO_T_Dir), mDirN(init.Com.GPIO_N_Dir) {
            PWMStart();
        }

        void Set(double value = 0.0) {
            const auto direction = (value < 0.0) ^ mInvert;
            const auto control = std::abs(value);
            if (control >= 0.01) {
                TIM_OC_InitTypeDef sConfigOC = {0};
                sConfigOC.OCMode = TIM_OCMODE_PWM1;
                sConfigOC.Pulse = static_cast<double>(mHdc->Init.Period) * control;
                sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
                sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
                if (HAL_TIM_PWM_ConfigChannel(mHdc, &sConfigOC, mChannel) != HAL_OK) {
                	throw MachineException("HAL_TIM_ReConf_Fail");
                }
                PWMStop();
                HAL_GPIO_WritePin(mDirT, mDirN, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
                PWMStart();
            }
            else PWMStop();
        }

    private:
        void PWMStop() {
        	if (HAL_TIM_PWM_Stop(mHdc, mChannel) != HAL_OK)
        		throw MachineException("HAL_TIM_Stop_Fail");
        }

        void PWMStart() {
        	if (HAL_TIM_PWM_Start(mHdc, mChannel) != HAL_OK)
        		throw MachineException("HAL_TIM_Restart_Fail");
        }

        TIM_HandleTypeDef *mHdc;
        uint32_t mChannel;
        bool mInvert;
        GPIO_TypeDef *mDirT;
        uint16_t mDirN;
    };

    template<class T>
    static constexpr T warp(T val, T min, T max) noexcept { return (val > max ? max : (val < min ? min : val)); }

    class PIDControl {
    public:
        explicit PIDControl(const MotorDriveInit &init) noexcept
                : mIntegrationCo{init.IntegrationCo * gIntegration},
                  mDifferentialCo{init.DifferentialCo * gDifferential},
                  mProportionalCo{init.ProportionalCo * gProportional},
                  mDecayCo{init.DecayCo * gDecay} {};

        double Step(double current) noexcept {
        	const auto power = StepPower(std::abs(current), std::abs(mTarget));
        	return mTarget > 0.0 ? power : -power;
        }

        double StepPower(double current, double target) noexcept {
            const auto now = HAL_GetTick();
            const auto dt = (now - mLastTick) * mTickTime;
            const auto error = target - current;
            const auto derivative = error - mLast / dt;
            mLast = error;
            mLastTick = now;
            mIntegration = warp<double>((mIntegration + error * dt) * std::pow(mDecayCo, dt * 1000.0),
                                        gErrorMin, gErrorMax);
            mScaler = warp<double>(
            		mScaler + mProportionalCo * error + mIntegrationCo * mIntegration + mDifferentialCo * derivative,
					0.0, 10.0
				);
            const auto power = mScaler * target * 0.66666666;
            return warp<double>(power, -0.95, 0.95);

        }

        double GetScaler() const noexcept { return mScaler; }

        double GetTarget() const noexcept { return mTarget; }

        void SetTarget(const double value) noexcept { mTarget = value; }

    private:
        uint32_t mLastTick{HAL_GetTick()};
        const double mTickTime{0.001 * HAL_GetTickFreq()};
        double mTarget{0.0}, mLast{0.0}, mIntegration{0.0}, mScaler { 0.0 };
        const double mIntegrationCo, mDifferentialCo, mProportionalCo, mDecayCo;
        static constexpr float gErrorMin = -10.f, gErrorMax = 10.f;
        static constexpr float gProportional = 1.5f, gIntegration = 120.0f, gDifferential = 0.05f, gDecay = 0.0985f;
    };

    class MotorImpl final : public Motor {
    public:
        explicit MotorImpl(const MotorInit &init)
                : mMeter(init), mDriver(init), mControl(init.Drive) {}

        ~MotorImpl() override { MotorImpl::Stop(); }

        [[nodiscard]] double GetSpeed() const noexcept override { return mMeter.GetSpeed(); }

        [[nodiscard]] double GetTargetSpeed() const noexcept override { return mControl.GetTarget(); }

        void SetTargetSpeed(const double value) noexcept override { mControl.SetTarget(value); }

        double GetScaler() const noexcept override { return mControl.GetScaler(); }

        uint32_t GetIrqCount() const noexcept override { return mMeter.GetIrqCount(); }

        void Reset() noexcept override {
            mControl.SetTarget(0.0);
            mDriver.Set(0.0);
        }

        void Start() {
            mStop = false;
            mThread = rstd::thread(osPriorityNormal, 512, [this]() {
            	rstd::run([this]() noexcept { mMeter.NVICSetup(); });
                while (!mStop) CtrlStep();
                Reset();
            });
        }

        void Stop() noexcept override {
            mStop = true;
            if (mThread.joinable()) mThread.join();
        }

    private:
        Meter mMeter;
        PWMDriver mDriver;
        PIDControl mControl;
        rstd::thread mThread;
        bool mStop{true};

        void CtrlStep() {
        	rstd::run([this]() noexcept {
        		// There is a caveat here: the single-phase encoder cannot tell if a wheel is turning
        		// in the reverse direction. We can only assume that if is going in the correct direction
        		const auto velocity = GetSpeed();
        		const auto vector = GetTargetSpeed() > 0 ? velocity : -velocity;
                mDriver.Set(mControl.Step(vector));
        	});
        	rstd::this_thread::sleep_for(rstd::chrono::milliseconds(50));
        }
    };
}

std::unique_ptr<Motor> RunMotor(const MotorInit &init) {
    auto result = std::make_unique<MotorImpl>(init);
    result->Start();
    return result;
}
