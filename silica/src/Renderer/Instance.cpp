#include "Instance.h"

#include "Vulkan/VulkanInstance.h"

namespace silica {

    std::unique_ptr<Instance> createInstance(const InstanceInfo& instanceInfo)
    {
        switch (instanceInfo.API)
        {
        case RendererAPI::Vulkan: return std::make_unique<VulkanInstance>(instanceInfo);
        default:
            return nullptr;
        }

        return nullptr;
    }

}
