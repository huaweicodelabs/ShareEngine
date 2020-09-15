#pragma once
#include <vector>
#include "Synergy/SynergyServiceImpl.h"

class MySendProgressListener : public WinShareKit::SendProgressListener
{
public:
    MySendProgressListener();
    ~MySendProgressListener();
    virtual void OnProgress(const WinShareKit::HwShareProgress& progress) override;
    virtual void OnStatusChange(const WinShareKit::HwShareStatus& status) override;
};

class MyDeviceChangeListener : public WinShareKit::DeviceChangeListener
{
public:
    MyDeviceChangeListener();
    ~MyDeviceChangeListener();
    virtual void OnDeviceFound(const WinShareKit::HwShareDevice& device) override;
    virtual void OnDeviceLost(const WinShareKit::HwShareDevice& device) override;
};
