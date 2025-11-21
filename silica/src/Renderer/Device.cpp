#include "Device.h"

#include <nvrhi/nvrhi.h>

namespace silica {

    struct Device::NvImpl
    {
        nvrhi::DeviceHandle Device = nullptr;
    };

    Device::Device()
        : m_Nv(std::make_unique<NvImpl>())
    {
    }

    Device::~Device() = default;

    void Device::setNvrhiDevice(void* nativeDevice)
    {
        m_Nv->Device = *reinterpret_cast<nvrhi::DeviceHandle*>(nativeDevice);
    }

    void Device::resetNvrhiDevice()
    {
        m_Nv->Device = nullptr;
    }

    void* Device::getNvrhiDevice()
    {
        return &m_Nv->Device;
    }
}
