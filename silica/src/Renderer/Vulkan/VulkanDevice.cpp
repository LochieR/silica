#include "VulkanDevice.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/vulkan.h>
#include <vulkan/vulkan.hpp>

#include <set>
#include <string>
#include <algorithm>

namespace vk::detail {
    DispatchLoaderDynamic defaultDispatchLoaderDynamic;
}

namespace silica {

    constexpr static bool s_EnableValidationLayers
#ifdef SIL_DEBUG
        = true;
#else
        = false;
#endif

    const static std::vector<const char*> s_ValidationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };    

    const static std::vector<const char*> s_DeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
#ifdef SIL_PLATFORM_MAC
        "VK_KHR_portability_subset"
#endif
    };

    namespace utils {

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices{};

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    indices.GraphicsFamily = i;

                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

                if (presentSupport)
                    indices.PresentFamily = i;

                if (indices.isComplete())
                    break;

                i++;
            }

            return indices;
        }

        static bool checkDeviceExtensionSupport(VkPhysicalDevice device)
        {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions;

            for (const char* ext : s_DeviceExtensions)
                requiredExtensions.insert(std::string(ext));

            for (const auto& extension : availableExtensions)
            {
                requiredExtensions.erase(std::string(extension.extensionName));
            }

            return requiredExtensions.empty();
        }

        SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            SwapchainSupportDetails details;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.Capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

            if (formatCount != 0)
            {
                details.Formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.Formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

            if (presentModeCount != 0)
            {
                details.PresentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.PresentModes.data());
            }

            return details;
        }

        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }

            SIL_ASSERT(false, "Failed to find suitable memory type!");
            return 0;
        }

        static bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface)
        {
            QueueFamilyIndices indices = findQueueFamilies(device, surface);

            bool extensionsSupported = checkDeviceExtensionSupport(device);

            bool swapchainAdequate = false;
            if (extensionsSupported)
            {
                SwapchainSupportDetails swapChainSupport = querySwapchainSupport(device, surface);
                swapchainAdequate = !swapChainSupport.Formats.empty() && !swapChainSupport.PresentModes.empty();
            }

            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

            return
                indices.isComplete() &&
                extensionsSupported &&
                swapchainAdequate &&
                supportedFeatures.sampleRateShading &&
                supportedFeatures.samplerAnisotropy;
        }

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
        {
            for (const auto& availableFormat : availableFormats)
            {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                {
                    return availableFormat;
                }
            }

            return availableFormats[0];
        }

        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
        {
            for (const auto& availablePresentMode : availablePresentModes)
            {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    return availablePresentMode;
                }
            }

            return VK_PRESENT_MODE_FIFO_KHR;
        }

        VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities)
        {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            {
                return capabilities.currentExtent;
            }
            else
            {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }

        static VkFormat findSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
        {
            for (VkFormat format : candidates)
            {
                VkFormatProperties properties;
                vkGetPhysicalDeviceFormatProperties(device, format, &properties);

                if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
                    return format;
                else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
                    return format;
            }

            SIL_ASSERT(false, "Failed to find supported Vulkan format!");
            return candidates[0];
        }

        VkFormat findDepthFormat(VkPhysicalDevice device)
        {
            return findSupportedFormat(
                device,
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
        }

        static bool hasStencilComponent(VkFormat format)
        {
            return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
        }

        static inline nvrhi::Format convertFormat(VkFormat format)
        {
            switch (format)
			{
			case VK_FORMAT_UNDEFINED:				 return nvrhi::Format::UNKNOWN;
			case VK_FORMAT_R8_UINT:					 return nvrhi::Format::R8_UINT;
			case VK_FORMAT_R8_SINT:					 return nvrhi::Format::R8_SINT;
			case VK_FORMAT_R8_UNORM:				 return nvrhi::Format::R8_UNORM;
			case VK_FORMAT_R8_SNORM:				 return nvrhi::Format::R8_SNORM;
			case VK_FORMAT_R8G8_UINT:				 return nvrhi::Format::RG8_UINT;
			case VK_FORMAT_R8G8_SINT:				 return nvrhi::Format::RG8_SINT;
			case VK_FORMAT_R8G8_UNORM:				 return nvrhi::Format::RG8_UNORM;
			case VK_FORMAT_R8G8_SNORM:				 return nvrhi::Format::RG8_SNORM;
			case VK_FORMAT_R16_UINT:				 return nvrhi::Format::R16_UINT;
			case VK_FORMAT_R16_SINT:				 return nvrhi::Format::R16_SINT;
			case VK_FORMAT_R16_UNORM:				 return nvrhi::Format::R16_UNORM;
			case VK_FORMAT_R16_SNORM:				 return nvrhi::Format::R16_SNORM;
			case VK_FORMAT_R16_SFLOAT:				 return nvrhi::Format::R16_FLOAT;
			case VK_FORMAT_B4G4R4A4_UNORM_PACK16:	 return nvrhi::Format::BGRA4_UNORM;
			case VK_FORMAT_B5G6R5_UNORM_PACK16:		 return nvrhi::Format::B5G6R5_UNORM;
			case VK_FORMAT_B5G5R5A1_UNORM_PACK16:	 return nvrhi::Format::B5G5R5A1_UNORM;
			case VK_FORMAT_R8G8B8A8_UINT:			 return nvrhi::Format::RGBA8_UINT;
			case VK_FORMAT_R8G8B8A8_SINT:			 return nvrhi::Format::RGBA8_SINT;
			case VK_FORMAT_R8G8B8A8_UNORM:			 return nvrhi::Format::RGBA8_UNORM;
			case VK_FORMAT_R8G8B8A8_SNORM:			 return nvrhi::Format::RGBA8_SNORM;
			case VK_FORMAT_B8G8R8A8_UNORM:			 return nvrhi::Format::BGRA8_UNORM;
			case VK_FORMAT_R8G8B8A8_SRGB:			 return nvrhi::Format::SRGBA8_UNORM;
			case VK_FORMAT_B8G8R8A8_SRGB:			 return nvrhi::Format::SBGRA8_UNORM;
			case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return nvrhi::Format::R10G10B10A2_UNORM;
			case VK_FORMAT_B10G11R11_UFLOAT_PACK32:	 return nvrhi::Format::R11G11B10_FLOAT;
			case VK_FORMAT_R16G16_SINT:				 return nvrhi::Format::RG16_SINT;
			case VK_FORMAT_R16G16_UINT:				 return nvrhi::Format::RG16_UINT;
			case VK_FORMAT_R16G16_UNORM:			 return nvrhi::Format::RG16_UNORM;
			case VK_FORMAT_R16G16_SNORM:			 return nvrhi::Format::RG16_SNORM;
			case VK_FORMAT_R16G16_SFLOAT:			 return nvrhi::Format::RG16_FLOAT;
			case VK_FORMAT_R32_UINT:				 return nvrhi::Format::R32_UINT;
			case VK_FORMAT_R32_SINT:				 return nvrhi::Format::R32_SINT;
			case VK_FORMAT_R32_SFLOAT:				 return nvrhi::Format::R32_FLOAT;
			case VK_FORMAT_R16G16B16A16_UINT:		 return nvrhi::Format::RGBA16_UINT;
			case VK_FORMAT_R16G16B16A16_SINT:		 return nvrhi::Format::RGBA16_SINT;
			case VK_FORMAT_R16G16B16A16_SFLOAT:		 return nvrhi::Format::RGBA16_FLOAT;
			case VK_FORMAT_R16G16B16A16_UNORM:		 return nvrhi::Format::RGBA16_UNORM;
			case VK_FORMAT_R16G16B16A16_SNORM:		 return nvrhi::Format::RGBA16_SNORM;
			case VK_FORMAT_R32G32_UINT:				 return nvrhi::Format::RG32_UINT;
			case VK_FORMAT_R32G32_SINT:				 return nvrhi::Format::RG32_SINT;
			case VK_FORMAT_R32G32_SFLOAT:			 return nvrhi::Format::RG32_FLOAT;
			case VK_FORMAT_R32G32B32_UINT:			 return nvrhi::Format::RGB32_UINT;
			case VK_FORMAT_R32G32B32_SINT:			 return nvrhi::Format::RGB32_SINT;
			case VK_FORMAT_R32G32B32_SFLOAT:		 return nvrhi::Format::RGB32_FLOAT;
			case VK_FORMAT_R32G32B32A32_UINT:		 return nvrhi::Format::RGBA32_UINT;
			case VK_FORMAT_R32G32B32A32_SINT:		 return nvrhi::Format::RGBA32_SINT;
			case VK_FORMAT_R32G32B32A32_SFLOAT:		 return nvrhi::Format::RGBA32_FLOAT;
			case VK_FORMAT_D16_UNORM:				 return nvrhi::Format::D16;
			case VK_FORMAT_D24_UNORM_S8_UINT:		 return nvrhi::Format::D24S8;
			case VK_FORMAT_D32_SFLOAT:				 return nvrhi::Format::D32;
			case VK_FORMAT_D32_SFLOAT_S8_UINT:		 return nvrhi::Format::D32S8;
			case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:	 return nvrhi::Format::BC1_UNORM;
			case VK_FORMAT_BC1_RGBA_SRGB_BLOCK:		 return nvrhi::Format::BC1_UNORM_SRGB;
			case VK_FORMAT_BC2_UNORM_BLOCK:			 return nvrhi::Format::BC2_UNORM;
			case VK_FORMAT_BC2_SRGB_BLOCK:			 return nvrhi::Format::BC2_UNORM_SRGB;
			case VK_FORMAT_BC3_UNORM_BLOCK:			 return nvrhi::Format::BC3_UNORM;
			case VK_FORMAT_BC3_SRGB_BLOCK:			 return nvrhi::Format::BC3_UNORM_SRGB;
			case VK_FORMAT_BC4_UNORM_BLOCK:			 return nvrhi::Format::BC4_UNORM;
			case VK_FORMAT_BC4_SNORM_BLOCK:			 return nvrhi::Format::BC4_SNORM;
			case VK_FORMAT_BC5_UNORM_BLOCK:			 return nvrhi::Format::BC5_UNORM;
			case VK_FORMAT_BC5_SNORM_BLOCK:			 return nvrhi::Format::BC5_SNORM;
			case VK_FORMAT_BC6H_UFLOAT_BLOCK:		 return nvrhi::Format::BC6H_UFLOAT;
			case VK_FORMAT_BC6H_SFLOAT_BLOCK:		 return nvrhi::Format::BC6H_SFLOAT;
			case VK_FORMAT_BC7_UNORM_BLOCK:			 return nvrhi::Format::BC7_UNORM;
			case VK_FORMAT_BC7_SRGB_BLOCK:			 return nvrhi::Format::BC7_UNORM_SRGB;

			default: return nvrhi::Format::UNKNOWN;
            }
        }

    }

    VulkanDevice::VulkanDevice(VulkanInstance* instance, const DeviceInfo &deviceInfo)
        : Device(), m_Instance(instance)
    {
        pickPhysicalDevice();
        createLogicalDevice();
        createDispatchLoaderDynamic();
        createNVRHIDevice();
    }

    VulkanDevice::~VulkanDevice()
    {
        destroy();
    }

    void VulkanDevice::destroy()
    {
        if (m_Valid && m_Instance)
        {
            getNvrhiDevice<nvrhi::DeviceHandle>()->runGarbageCollection();
            resetNvrhiDevice();

            vkDeviceWaitIdle(m_Device);
            
            vkDestroyDevice(m_Device, m_Instance->getAllocator());
        }
    }
    
    void VulkanDevice::invalidate() noexcept
    {
        m_Valid = false;
        m_Instance = nullptr;
    }

    void VulkanDevice::pickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance->getInstance(), &deviceCount, nullptr);

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance->getInstance(), &deviceCount, devices.data());

        for (const auto& device : devices)
        {
            if (utils::isDeviceSuitable(device, m_Instance->getSurface()))
            {
                m_PhysicalDevice = device;
                break;
            }
        }

        SIL_ASSERT(m_PhysicalDevice, "Failed to find a suitable GPU!");

        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        
        SIL_INFO("Using device: {}", properties.deviceName);
    }

    void VulkanDevice::createLogicalDevice()
    {
        QueueFamilyIndices indices = utils::findQueueFamilies(m_PhysicalDevice, m_Instance->getSurface());

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { indices.GraphicsFamily, indices.PresentFamily };

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo& queueCreateInfo = queueCreateInfos.emplace_back();
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
        }

        VkPhysicalDeviceFeatures deviceFeatures{};
        deviceFeatures.sampleRateShading = VK_TRUE;
        deviceFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = (uint32_t)s_DeviceExtensions.size();
        createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

        if (s_EnableValidationLayers)
        {
            createInfo.enabledLayerCount = (uint32_t)s_ValidationLayers.size();
            createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        VkPhysicalDeviceFloat16Int8FeaturesKHR float16FeaturesEnable{};
        float16FeaturesEnable.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
        float16FeaturesEnable.shaderFloat16 = VK_TRUE;

        VkPhysicalDeviceVulkan11Features vulkan11Features{};
        vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        vulkan11Features.shaderDrawParameters = VK_TRUE;

        VkPhysicalDeviceTimelineSemaphoreFeatures timelineSemaphoreFeatures{};
		timelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		timelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

        vulkan11Features.pNext = &timelineSemaphoreFeatures;
        float16FeaturesEnable.pNext = &vulkan11Features;
        createInfo.pNext = &float16FeaturesEnable;

        VkResult result = vkCreateDevice(m_PhysicalDevice, &createInfo, m_Instance->getAllocator(), &m_Device);
        VK_CHECK(result, "Failed to create Vulkan device!");

        loadExtensions();

        VK_DEBUG_NAME(m_Device, DEVICE, m_Device, "VulkanRenderer::m_Device");
        VK_DEBUG_NAME(m_Device, SURFACE_KHR, m_Instance->getSurface(), "VulkanRenderer::m_Surface");

        vkGetDeviceQueue(m_Device, indices.GraphicsFamily, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.PresentFamily, 0, &m_PresentQueue);

        VK_DEBUG_NAME(m_Device, QUEUE, m_GraphicsQueue, "VulkanRenderer::m_GraphicsQueue");
        VK_DEBUG_NAME(m_Device, QUEUE, m_PresentQueue, "VulkanRenderer::m_PresentQueue");
    }

    void VulkanDevice::createNVRHIDevice()
    {
        QueueFamilyIndices indices = utils::findQueueFamilies(m_PhysicalDevice, m_Instance->getSurface());

        std::vector<const char*> instanceExtensions = utils::getRequiredInstanceExtensions();

        nvrhi::vulkan::DeviceDesc deviceDesc{};
        deviceDesc.errorCB = &m_MessageCallback;
        deviceDesc.instance = m_Instance->getInstance();
        deviceDesc.physicalDevice = m_PhysicalDevice;
        deviceDesc.device = m_Device;
        deviceDesc.graphicsQueue = m_GraphicsQueue;
        deviceDesc.graphicsQueueIndex = indices.GraphicsFamily;
        deviceDesc.allocationCallbacks = const_cast<VkAllocationCallbacks*>(m_Instance->getAllocator());
        deviceDesc.numInstanceExtensions = instanceExtensions.size();
        deviceDesc.instanceExtensions = instanceExtensions.data();
        deviceDesc.numDeviceExtensions = s_DeviceExtensions.size();
        deviceDesc.deviceExtensions = const_cast<const char**>(s_DeviceExtensions.data());

        nvrhi::DeviceHandle device = nvrhi::vulkan::createDevice(deviceDesc);
        setNvrhiDevice(&device);
    }

    void VulkanDevice::createDispatchLoaderDynamic()
    {
        vk::detail::defaultDispatchLoaderDynamic.init(m_Instance->getInstance(), m_Device);
    }

    void VulkanDevice::createCommandPool()
    {
        VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		VkResult result = vkCreateCommandPool(m_Device, &poolInfo, m_Instance->getAllocator(), &m_CommandPool);
		VK_CHECK(result, "Failed to create Vulkan command pool!");

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandBufferCount = SIL_FRAMES_IN_FLIGHT;
		allocInfo.commandPool = m_CommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		
		result = vkAllocateCommandBuffers(m_Device, &allocInfo, m_EndOfFrameCommandBuffers.data());
		VK_CHECK(result, "Failed to allocate Vulkan command buffers!");
    }

    void VulkanDevice::createSyncObjects()
    {
        VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < SIL_FRAMES_IN_FLIGHT; i++)
		{
			VkResult result = vkCreateSemaphore(m_Device, &semaphoreInfo, m_Instance->getAllocator(), &m_PresentSemaphores[i]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");

			result = vkCreateSemaphore(m_Device, &semaphoreInfo, m_Instance->getAllocator(), &m_EndOfFrameSemaphores[i]);
			VK_CHECK(result, "Failed to create Vulkan semaphore!");

			result = vkCreateFence(m_Device, &fenceInfo, m_Instance->getAllocator(), &m_InFlightFences[i]);
			VK_CHECK(result, "Failed to create Vulkan fence!");
		}
    }

    void VulkanDevice::createSwapchain()
    {
        VkSwapchainKHR oldSwapchain = m_Swapchain;
		m_SwapchainImages.clear();

		SwapchainSupportDetails swapchainSupport = utils::querySwapchainSupport(m_PhysicalDevice, m_Instance->getSurface());
		VkSurfaceFormatKHR surfaceFormat = utils::chooseSwapSurfaceFormat(swapchainSupport.Formats);
		VkPresentModeKHR presentMode = utils::chooseSwapPresentMode(swapchainSupport.PresentModes);
		VkExtent2D extent = utils::chooseSwapExtent(m_Instance->getWindow(), swapchainSupport.Capabilities);

		m_SwapchainImageFormat = utils::convertFormat(surfaceFormat.format);

		uint32_t imageCount = swapchainSupport.Capabilities.minImageCount + 1;

		if (swapchainSupport.Capabilities.maxImageCount > 0 && imageCount > swapchainSupport.Capabilities.maxImageCount)
			imageCount = swapchainSupport.Capabilities.maxImageCount;

		m_SwapchainImageCount = imageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_Instance->getSurface();
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = nvrhi::vulkan::convertFormat(m_SwapchainImageFormat);
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.oldSwapchain = oldSwapchain;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		QueueFamilyIndices indices = utils::findQueueFamilies(m_PhysicalDevice, m_Instance->getSurface());
		uint32_t queueFamilyIndices[] = { indices.GraphicsFamily, indices.PresentFamily };

		if (indices.GraphicsFamily != indices.PresentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}
		
		createInfo.preTransform = swapchainSupport.Capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;

		VkResult result = vkCreateSwapchainKHR(m_Device, &createInfo, m_Instance->getAllocator(), &m_Swapchain);
		VK_CHECK(result, "Failed to create Vulkan swapchain");

		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_SwapchainImageCount, nullptr);
		std::vector<VkImage> images(m_SwapchainImageCount);
		vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &m_SwapchainImageCount, images.data());

		for (VkImage image : images)
		{
			SwapchainImage scImage;
			scImage.Image = image;

			nvrhi::TextureDesc textureDesc = nvrhi::TextureDesc()
				.setWidth(extent.width)
				.setHeight(extent.height)
				.setSampleCount(1)
				.setSampleQuality(0)
				.setFormat(m_SwapchainImageFormat)
				.setDebugName("Swapchain Buffer")
				.setIsRenderTarget(true)
				.setIsUAV(false)
				.setInitialState(nvrhi::ResourceStates::Present)
				.setKeepInitialState(true);

			scImage.NVRHIHandle = getNvrhiDevice<nvrhi::DeviceHandle>()->createHandleForNativeTexture(nvrhi::ObjectTypes::VK_Image, scImage.Image, textureDesc);
			m_SwapchainImages.push_back(scImage);
		}

		m_SwapchainIndex = 0;

		if (oldSwapchain)
		{
			vkDestroySwapchainKHR(m_Device, oldSwapchain, m_Instance->getAllocator());
		}
    }

    void VulkanDevice::destroySwapchain()
    {
        if (m_Device)
		{
			vkDeviceWaitIdle(m_Device);
		}
		if (m_Swapchain)
		{
			vkDestroySwapchainKHR(m_Device, m_Swapchain, m_Instance->getAllocator());
			m_Swapchain = nullptr;
		}

		m_SwapchainImages.clear();
    }

    void VulkanDevice::loadExtensions()
    {
        exts::vkSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(m_Device, "vkSetDebugUtilsObjectNameEXT");
    }

}
