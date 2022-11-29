#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vk_mem_alloc.h>

#include <optional>
#include <set>
#include <cassert>

#include "utils.h"
#include "PhysicsEngine.h"
#include "VulkanHelpers.h"

struct QueueFamilyIndices {
    std::optional<u32> graphics_family;
    std::optional<u32> present_family;
    std::optional<u32> compute_family;

    bool isComplete() const {
        return graphics_family.has_value() && present_family.has_value() && compute_family.has_value();
    }

    std::set<u32> getUniqueFamilies() const {
        assert(isComplete());
        return std::set<u32> {graphics_family.value(), present_family.value(), compute_family.value()};
    }
};

struct Queues {
    vk::Queue graphics;
    vk::Queue present;
    vk::Queue compute;
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

    void createInstance();
    static bool checkValidationLayerSupport();
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                        VkDebugUtilsMessageTypeFlagBitsEXT message_type,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                        void* user_data);
    void setupDebugMessenger();
    void createSurface();
    static bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);
    SwapchainSupportDetails querySwapchainSupport(const vk::PhysicalDevice& device);
    QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device);
    vk::PhysicalDevice pickPhysicalDevice();
    void createDevice();

    void createVmaAllocator();

    vk::SurfaceFormatKHR chooseSwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes);
    vk::Extent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapchain();


    VulkanBuffer createBuffer(const u32 buffer_size, const u32 family_index);
    vk::MemoryType findMemoryType();

    void createComputePipeline();

    // GLFW stuff
    void initWindow();
    void destroyWindow();

    // member variables
    vk::Instance m_instance;
    vk::DispatchLoaderDynamic m_dispatcher;
    std::optional<vk::DebugUtilsMessengerEXT> m_debug_messenger;
    vk::Device m_device;
    VmaAllocator m_allocator;
    QueueFamilyIndices m_queue_family_indices;
    Queues m_queues;
    GLFWwindow* m_window;
    vk::SurfaceKHR m_surface;
    vk::SwapchainKHR m_swapchain;
    vk::PhysicalDevice m_physical_device;
    PhysicsEngine m_physics_engine;
    i32* compute_in_buffer_ptr;

    void destroyVmaAllocator();

    friend class GPURadixSorter;
};




