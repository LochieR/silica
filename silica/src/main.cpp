#include <iostream>

#include "Renderer/Device.h"
#include "Renderer/Instance.h"

int main()
{
    SIL_SETUP_LOG({ &std::cout }, {}, "%c[%H:%M:%S] %m%c");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "silica", nullptr, nullptr);

    silica::InstanceInfo instanceInfo{};
    instanceInfo.API = silica::RendererAPI::Vulkan;
    instanceInfo.Window = window;

    silica::DeviceInfo deviceInfo{};

    std::unique_ptr<silica::Instance> instance = silica::createInstance(instanceInfo);
    std::shared_ptr<silica::Device> device = instance->createDevice(deviceInfo);

    while (!glfwWindowShouldClose(window))
    {
        device->beginFrame();

        device->endFrame();

        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}
