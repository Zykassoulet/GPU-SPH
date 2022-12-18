#pragma once

#include "VulkanInclude.h"
#include <GLFW/glfw3.h>

#include "utils.h"

#include "VulkanHelpers.h"

#include <optional>
#include <set>

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
    QueueFamilyIndices indices;
};

struct SwapchainSupportDetails {
    vk::SurfaceCapabilitiesKHR capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR> present_modes;
};

class VulkanContext {
public:
    VulkanContext();

    // Delete copy constructor and copy assignment
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;

    ~VulkanContext();

private:
    // Vulkan stuff
    inline static const std::vector<const char*> validation_layers = {
            "VK_LAYER_KHRONOS_validation"
    };;

    inline static const std::vector<const char*> device_extensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
     //       VK_GOOGLE_HLSL_FUNCTIONALITY1_EXTENSION_NAME,
     //       VK_GOOGLE_USER_TYPE_EXTENSION_NAME
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


    VulkanBuffer createBuffer(vk::BufferUsageFlags usage, const u32 object_size, const u32 object_count = 1, VmaAllocationCreateInfo alloc_info = {});
    VulkanBuffer createCPUAccessibleBuffer(vk::BufferUsageFlags usage, const u32 object_size, const u32 object_count = 1);

    void createComputeCommandPool();
    void createGraphicsCommandPool();

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
    vk::CommandPool m_compute_command_pool;
    vk::CommandPool m_graphics_command_pool;

    void destroyVmaAllocator();

    friend class GPURadixSorter;
    friend class ZIndexer;
    friend class SimulatorComputeStage;
    friend class PhysicsEngine;
    friend class App;
    friend class DensityCompute;
    friend class VelocityCompute;
    friend class PositionCompute;
    friend class Renderer;
};
