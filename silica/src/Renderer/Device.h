#pragma once

#include "Resource.h"

namespace silica {

    class Instance;

    struct DeviceInfo
    {
    };

    class Device : public Resource
    {
    public:
        Device();
        ~Device();
    protected:
        void setNvrhiDevice(void* nativeDevice);
        void resetNvrhiDevice();
        void* getNvrhiDevice();

        template<typename T>
        T& getNvrhiDevice()
        {
            return *reinterpret_cast<T*>(getNvrhiDevice());
        }
    private:
        struct NvImpl;
        std::unique_ptr<NvImpl> m_Nv;
    };

}
