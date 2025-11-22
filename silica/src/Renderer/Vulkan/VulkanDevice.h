#pragma once

#include "VulkanInstance.h"
#include "Renderer/Device.h"

#include <nvrhi/nvrhi.h>
#include <nvrhi/vulkan.h>
#include <vulkan/vulkan.h>

#include <array>
#include <vector>

namespace silica {

    struct QueueFamilyIndices
    {
        uint32_t GraphicsFamily = static_cast<uint32_t>(-1);
        uint32_t PresentFamily = static_cast<uint32_t>(-1);

        bool isComplete() const { return GraphicsFamily != static_cast<uint32_t>(-1) && PresentFamily != static_cast<uint32_t>(-1); }
    };

    struct SwapchainSupportDetails
    {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    class VulkanDevice : public Device
    {
    public:
        VulkanDevice(VulkanInstance* instance, const DeviceInfo& deviceInfo);
        virtual ~VulkanDevice();

        virtual void beginFrame() override;
        virtual void endFrame() override;
    protected:
        virtual void destroy() override;
        virtual void invalidate() noexcept override;
    private:
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createDispatchLoaderDynamic();
        void createNVRHIDevice();
        void createCommandPool();
        void createSyncObjects();
        void createSwapchain();
        void destroySwapchain();

        void loadExtensions();
    private:
        VulkanInstance* m_Instance = nullptr;

        VkPhysicalDevice m_PhysicalDevice = nullptr;
        VkDevice m_Device = nullptr;
        VkQueue m_GraphicsQueue = nullptr;
        VkQueue m_PresentQueue = nullptr;
        VkCommandPool m_CommandPool = nullptr;
        std::array<VkCommandBuffer, SIL_FRAMES_IN_FLIGHT> m_EndOfFrameCommandBuffers;

        std::array<VkSemaphore, SIL_FRAMES_IN_FLIGHT> m_EndOfFrameSemaphores;
		std::array<VkSemaphore, SIL_FRAMES_IN_FLIGHT> m_PresentSemaphores;
		std::array<VkFence, SIL_FRAMES_IN_FLIGHT> m_InFlightFences;

        uint32_t m_FrameIndex = 0;

        VkSwapchainKHR m_Swapchain = nullptr;
		VkFormat m_ImageFormat;
		VkExtent2D m_Extent;

        struct SwapchainImage
		{
			VkImage Image;
			nvrhi::TextureHandle NVRHIHandle;
		};

		std::vector<SwapchainImage> m_SwapchainImages;
		nvrhi::Format m_SwapchainImageFormat;

        uint32_t m_SwapchainIndex = 0;
        uint32_t m_SwapchainImageCount;

        nvrhi::vulkan::DeviceHandle m_NvrhiDevice;
        nvrhi::CommandListHandle m_EndOfFrameCommandList;

        class MessageCallback : public nvrhi::IMessageCallback
        {
        public:
            MessageCallback() = default;
            virtual ~MessageCallback() = default;

            virtual void message(nvrhi::MessageSeverity severity, const char* messageText) override
            {
                switch (severity)
                {
                case nvrhi::MessageSeverity::Info: SIL_INFO("[nvrhi] {}", messageText); break;
                case nvrhi::MessageSeverity::Warning: SIL_WARN("[nvrhi] {}", messageText); break;
                case nvrhi::MessageSeverity::Error: SIL_ERROR("[nvrhi] {}", messageText); break;
                case nvrhi::MessageSeverity::Fatal: SIL_ASSERT_OR_ERROR(false, "[nvrhi] {}", messageText); break;
                default:
                    break;
                }
            }
        };

        MessageCallback m_MessageCallback;
    };

    namespace utils {

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(GLFWwindow* window, const VkSurfaceCapabilitiesKHR& capabilities);
        VkFormat findDepthFormat(VkPhysicalDevice device);
        uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

    }

}
