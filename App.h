#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#include <optional>
#include <set>
#include <cassert>

#include "utils.h"

struct QueueFamilyIndices {
    std::optional<u32> graphics_family;
    std::optional<u32> present_family;

    bool isComplete() const {
        return graphics_family.has_value() && present_family.has_value();
    }

    std::set<u32> getUniqueFamilies() const {
        assert(isComplete());
        return std::set<u32> {graphics_family.value(), present_family.value()};
    }
};

struct Queues {
    vk::Queue graphics;
    vk::Queue present;
};

struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

class App {
public:
    App();

    // Delete copy constructor and copy assignment
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void mainLoop();

    ~App();

private:
    // Vulkan stuff
    inline static const std::vector<const char*> validation_layers = {
            "VK_LAYER_KHRONOS_validation"
    };;

    inline static const std::vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    #ifdef NDEBUG
    static const bool enable_validation_layers = false;
    #else
    static const bool enable_validation_layers = true;
    #endif

    static vk::Instance createInstance();
    static bool checkValidationLayerSupport();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                        VkDebugUtilsMessageTypeFlagBitsEXT message_type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                        void* user_data);
    static std::optional<vk::DebugUtilsMessengerEXT> setupDebugMessenger(vk::Instance& instance);
    static vk::SurfaceKHR createSurface(vk::Instance& instance, GLFWwindow* window);
    static bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);
    static SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
    static QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface);
    static vk::PhysicalDevice pickPhysicalDevice(vk::Instance& instance, const vk::SurfaceKHR& surface);
    static std::tuple<vk::Device, Queues> createDevice(vk::Instance& instance, const vk::SurfaceKHR& surface);

    static vk::SurfaceFormatKHR chooseSwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats);
    static vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes);
    static vk::Extent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
    static vk::SwapchainKHR createSwapchain(vk::Device& device, GLFWwindow* window);

    // GLFW stuff
    static GLFWwindow* initWindow();
    static void destroyWindow(GLFWwindow* window);

    // member variables
    vk::Instance m_instance;
    vk::DispatchLoaderDynamic m_dispatcher;
    std::optional<vk::DebugUtilsMessengerEXT> m_debug_messenger;
    vk::Device m_device;
    Queues m_queues;
    GLFWwindow* m_window;
    vk::SurfaceKHR m_surface;
};




