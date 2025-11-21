#pragma once

#include "VulkanExtensions.h"

#include "Core/Assert.h"
#include "Renderer/Instance.h"

#include <vulkan/vulkan.h>

#include <vector>

namespace silica {

#define VK_CHECK(result, msg) SIL_ASSERT_OR_ERROR(result == VK_SUCCESS, msg)

#ifdef SIL_DEBUG
#define VK_DEBUG_NAME(device, type, object, nameCStr)                        \
    {                                                                        \
        VkDebugUtilsObjectNameInfoEXT info{};                                \
        info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;    \
        info.objectType = VK_OBJECT_TYPE_##type;                            \
        info.objectHandle = (uint64_t)object;                                \
        info.pObjectName = nameCStr;                                        \
                                                                            \
        exts::vkSetDebugUtilsObjectNameEXT(device, &info);                    \
    }
#else
#define VK_DEBUG_NAME(...)
#endif

    class VulkanInstance : public Instance
    {
    public:
        VulkanInstance(const InstanceInfo& instanceInfo);
        virtual ~VulkanInstance();

        virtual std::shared_ptr<Device> createDevice(const DeviceInfo& deviceInfo) override;

        VkInstance getInstance() const { return m_Instance; }
        const VkAllocationCallbacks* getAllocator() const { return m_Allocator; }
        VkDebugUtilsMessengerEXT getDebugMessenger() const { return m_DebugMessenger; }
        VkSurfaceKHR getSurface() const { return m_Surface; }
        GLFWwindow* getWindow() const { return m_Info.Window; }
    private:
        void createInstance();
        void createSurface();
    private:
        InstanceInfo m_Info;

        VkInstance m_Instance = nullptr;
        VkAllocationCallbacks* m_Allocator = nullptr;
        VkDebugUtilsMessengerEXT m_DebugMessenger = nullptr;
        VkSurfaceKHR m_Surface = nullptr;
        
        std::vector<std::shared_ptr<Resource>> m_Resources;
    };

    namespace utils {
        
        bool checkValidationLayerSupport();
        std::vector<const char*> getRequiredInstanceExtensions();

    }

}
