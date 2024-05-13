#pragma once

#include "Enum.hpp"

#include <cstdint>

class IDisplay
{
public:
	virtual void Init() = 0;
	virtual bool IsReady() = 0;
	virtual void SetMode(SpindleMode aMode) = 0;

	virtual void SetRequestedSpeed(int16_t setValue) = 0;
	virtual void SetCurrentSpeed(int16_t actualValue) = 0;
	virtual void SetPWMValue(int16_t pwmValue) = 0;
	virtual void SetRPMScale(const uint16_t aMinValue, const uint16_t aMaxValue) = 0;
	virtual uint32_t Update() = 0;

	IDisplay() = default;
};
