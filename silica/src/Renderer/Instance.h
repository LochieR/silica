#pragma once

#include "Resource.h"
#include "Device.h"

#include <glfw/glfw3.h>

namespace silica {

#define SIL_FRAMES_IN_FLIGHT 2

    enum class RendererAPI
    {
        Vulkan = 0,
        D3D12,
        D3D11
    };

    struct InstanceInfo
    {
        RendererAPI API = RendererAPI::Vulkan;
        GLFWwindow* Window = nullptr;
    };

    class Instance
    {
    public:
        virtual ~Instance() = default;

        virtual std::shared_ptr<Device> createDevice(const DeviceInfo& deviceInfo) = 0;
    };

    std::unique_ptr<Instance> createInstance(const InstanceInfo& instanceInfo);

}
