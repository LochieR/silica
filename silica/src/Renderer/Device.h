#pragma once

#include "Core/Log.h"

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

        virtual void beginFrame() = 0;
        virtual void endFrame() = 0;
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
