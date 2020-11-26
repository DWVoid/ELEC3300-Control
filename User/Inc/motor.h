#pragma once

#include "machine.h"
#include <memory>

class Motor {
public:
    virtual ~Motor() = default;

    [[nodiscard]] virtual double GetSpeed() const noexcept = 0;

    [[nodiscard]] virtual double GetTargetSpeed() const noexcept = 0;

    virtual void SetTargetSpeed(double value) = 0;

    virtual double GetScaler() const noexcept = 0;

    virtual uint32_t GetIrqCount() const noexcept = 0;

    virtual void Reset() noexcept = 0;

    virtual void Stop() noexcept = 0;
};

std::unique_ptr<Motor> RunMotor(const MotorInit &init);
