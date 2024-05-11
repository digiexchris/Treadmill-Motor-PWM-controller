#pragma once

double ScaleValue(double value, double minOld, double maxOld, double minNew, double maxNew)
{
    return (value - minOld) / (maxOld - minOld) * (maxNew - minNew) + minNew;
}