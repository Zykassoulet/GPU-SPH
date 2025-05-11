#define VMA_IMPLEMENTATION
#include "VulkanContext.h"
#include "PhysicsEngine.h"

#include <iostream>
#include <limits>
#include <algorithm>

#ifndef NDEBUG
    #define VMA_DEBUG_LOG(format, ...) printf(format, ##__VA_ARGS__)
#endif


VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

VulkanContext::VulkanContext() {
    vk::DynamicLoader dl;
    auto vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
    initWindow();
    createInstance();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_instance);
    setupDebugMessenger();
    createSurface();
    createDevice();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(m_device);
    createVmaAllocator();
    createComputeCommandPool();
    createGraphicsCommandPool();
    createSwapchain();
    createImageViews();
}

VulkanContext::~VulkanContext() {

    for (auto imageView : m_swapchain_image_views) {
        m_device.destroyImageView(imageView);
    }

    m_device.destroyImageView(m_depth_image_view);

    destroyImage(m_depth_image);
    
    m_device.destroySwapchainKHR(m_swapchain);

    destroyVmaAllocator();
    m_device.destroy();

    m_instance.destroySurfaceKHR(m_surface);

    if (m_debug_messenger.has_value()) {
        m_instance.destroyDebugUtilsMessengerEXT(m_debug_messenger.value());
    }

    m_instance.destroy();

    destroyWindow();
}

void VulkanContext::createInstance() {
    u32 glfw_extension_count = 0;
    const char** glfw_extensions;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

#ifdef __APPLE__
    extensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#endif

    auto instance_version = vk::enumerateInstanceVersion();
    std::cout << "Vulkan version: " << VK_API_VERSION_MAJOR(instance_version) << "." << VK_API_VERSION_MINOR(instance_version)
        << "." << VK_API_VERSION_PATCH(instance_version) << std::endl;
    auto instance_extensions = vk::enumerateInstanceExtensionProperties();
    std::cout << "Supported extensions:" << std::endl;
    for (const auto& ext : instance_extensions) {
        std::cout << (const char*) ext.extensionName << std::endl;
    }

    if (enable_validation_layers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available");
    }

    vk::ApplicationInfo app_info("Physically-based Simulation Project",
                                 1,
                                 "Physically-based Simulation Project",
                                 1,
                                 VK_MAKE_VERSION(1, 2, 0));

    vk::InstanceCreateFlags flags = {};

#ifdef __APPLE__
    flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    vk::InstanceCreateInfo create_info(flags, &app_info, 0, nullptr, static_cast<u32>(extensions.size()), extensions.data());

    std::cout << "Requested extensions:" << std::endl;
    for (const auto& ext : extensions) {
        std::cout << ext << std::endl;
    }

    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<u32>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    m_instance = vk::createInstance(create_info);
}

VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                    VkDebugUtilsMessageTypeFlagBitsEXT message_type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                    void* user_data) {
    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        std::cerr << "validation layer: " << callback_data->pMessage << std::endl;

    return VK_FALSE;
}

void VulkanContext::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    const u32 WIDTH = 800;
    const u32 HEIGHT = 600;

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    assert(window != nullptr);

    m_window = window;
}

void VulkanContext::destroyWindow() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool VulkanContext::checkValidationLayerSupport() {
    auto available_layers = vk::enumerateInstanceLayerProperties();

    for (const char* layer_name : validation_layers) {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
            }
        }

        if (!layer_found) {
            return false;
        }
    }
    return true;
}

void VulkanContext::setupDebugMessenger() {
    if (!enable_validation_layers) {
        m_debug_messenger =  {};
    }
    else {
        using severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
        using type = vk::DebugUtilsMessageTypeFlagBitsEXT;

        vk::DebugUtilsMessengerCreateInfoEXT create_info(
            {},
            severity::eWarning | severity::eError,
            type::ePerformance | type::eValidation | type::eGeneral,
            (PFN_vkDebugUtilsMessengerCallbackEXT) debugCallback,
            nullptr
        );

         m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(create_info, nullptr);

    }

}

vk::PhysicalDevice VulkanContext::pickPhysicalDevice() {
    auto physical_devices = m_instance.enumeratePhysicalDevices();

    auto is_device_suitable = [&](const vk::PhysicalDevice& p) {
        auto props = p.getProperties();
        auto features = p.getFeatures();
        auto indices = findQueueFamilies(p);
        bool extensions_supported = checkDeviceExtensionSupport(p);
        bool swapchain_adequate = false;

        if (extensions_supported) {
            auto swapchain_support = querySwapchainSupport(p);
            swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
        }

        return indices.isComplete() && extensions_supported && swapchain_adequate;
    };

    for (const auto& physical_device : physical_devices) {
        if (is_device_suitable(physical_device)) {
            return physical_device;
        }
    }

    throw std::runtime_error("No suitable physical device found");
}

QueueFamilyIndices VulkanContext::findQueueFamilies(const vk::PhysicalDevice &device) {
    QueueFamilyIndices indices;

    auto queue_families = device.getQueueFamilyProperties();
    u32 i = 0;
    for (const auto& family : queue_families) {
        if (family.queueFlags & (vk::QueueFlagBits::eGraphics)) {
            indices.graphics_family = i;
        }

        bool present_support = device.getSurfaceSupportKHR(i, m_surface);
        if (present_support) {
            indices.present_family = i;
        }

        if (family.queueFlags & (vk::QueueFlagBits::eCompute)) {
            indices.compute_family = i;
        }

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

void VulkanContext::createDevice() {
    auto physical_device = pickPhysicalDevice();
    auto indices = findQueueFamilies(physical_device);

    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos;
    auto unique_families = indices.getUniqueFamilies();
    for (u32 queue_family : unique_families) {
        auto queue_priorites = std::array<float, 1>{1.0f};
        vk::DeviceQueueCreateInfo queue_create_info({}, queue_family, queue_priorites);
        queue_create_infos.push_back(queue_create_info);
    }

    vk::PhysicalDeviceFeatures physical_device_features;

    vk::DeviceCreateInfo create_info({}, queue_create_infos, {}, device_extensions, &physical_device_features);

    auto device = physical_device.createDevice(create_info);

    Queues queues;
    queues.graphics = device.getQueue(indices.graphics_family.value(), 0);
    queues.present = device.getQueue(indices.present_family.value(), 0);
    queues.compute = device.getQueue(indices.compute_family.value(), 0);
    queues.indices = indices;

    m_queue_family_indices = indices;
    m_physical_device = physical_device;
    m_device = device;
    m_queues = queues;
}

void VulkanContext::createSurface() {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    m_surface = surface;
}

bool VulkanContext::checkDeviceExtensionSupport(const vk::PhysicalDevice& device) {
    auto extensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

SwapchainSupportDetails VulkanContext::querySwapchainSupport(const vk::PhysicalDevice& device) {
    SwapchainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(m_surface);
    details.formats = device.getSurfaceFormatsKHR(m_surface);
    details.present_modes = device.getSurfacePresentModesKHR(m_surface);

    return details;
}

vk::SurfaceFormatKHR VulkanContext::chooseSwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }

    return available_formats[0];
}

vk::PresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<u32>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        vk::Extent2D actualExtent = {
            static_cast<u32>(width),
            static_cast<u32>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

void VulkanContext::createSwapchain() {
    SwapchainSupportDetails swapChainSupport = querySwapchainSupport(m_physical_device);

    vk::SurfaceFormatKHR surfaceFormat = chooseSwapchainFormat(swapChainSupport.formats);
    m_swapchain_image_format = surfaceFormat.format;
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.present_modes);
    m_window_extent = chooseSwapExtent(swapChainSupport.capabilities);

    u32 imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    vk::SwapchainCreateInfoKHR swapchain_info({}, m_surface, imageCount, surfaceFormat.format, surfaceFormat.colorSpace, m_window_extent, 1, vk::ImageUsageFlagBits::eColorAttachment);

    auto indices = std::array { m_queue_family_indices.graphics_family.value(), m_queue_family_indices.present_family.value() };
    if (m_queue_family_indices.graphics_family != m_queue_family_indices.present_family) {
        swapchain_info.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = indices.data();
    }
    else {
        swapchain_info.imageSharingMode = vk::SharingMode::eExclusive;
        swapchain_info.queueFamilyIndexCount = 0; // Optional
        swapchain_info.pQueueFamilyIndices = nullptr; // Optional
    }

    swapchain_info.presentMode = presentMode;
    swapchain_info.clipped = VK_FALSE;
    m_swapchain = m_device.createSwapchainKHR(swapchain_info);

    m_swapchain_images = m_device.getSwapchainImagesKHR(m_swapchain);

    vk::Extent3D depth_image_extent = {
            m_window_extent.width,
            m_window_extent.height,
            1
    };


    m_depth_format = vk::Format::eD32Sfloat;

    auto depth_image_create_info = VulkanImage::create_info(m_depth_format, vk::ImageUsageFlagBits::eDepthStencilAttachment, depth_image_extent);
    VmaAllocationCreateInfo depth_image_alloc_info = {};
    depth_image_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    depth_image_alloc_info.requiredFlags = static_cast<VkMemoryPropertyFlags>(vk::MemoryPropertyFlagBits::eDeviceLocal);

    vmaCreateImage(m_allocator, reinterpret_cast<const VkImageCreateInfo *>(&depth_image_create_info), &depth_image_alloc_info,
                   reinterpret_cast<VkImage *>(&m_depth_image.image), &m_depth_image.allocation, nullptr);

    auto depth_image_view_create_info = VulkanImage::view_create_info(m_depth_format, m_depth_image.image, vk::ImageAspectFlagBits::eDepth);

    m_depth_image_view = m_device.createImageView(depth_image_view_create_info);
}

void VulkanContext::createImageViews() {
    for (int i = 0; i < m_swapchain_images.size(); i++) {
        vk::ComponentMapping mapping;
        vk::ImageSubresourceRange subresource_range(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1);
        vk::ImageViewCreateInfo image_view_info({}, m_swapchain_images[i], vk::ImageViewType::e2D, m_swapchain_image_format, mapping, subresource_range);
        m_swapchain_image_views.push_back(m_device.createImageView(image_view_info));
    }
}


VulkanBuffer VulkanContext::createBuffer(vk::BufferUsageFlags usage, const u32 object_size, const u32 object_count, VmaAllocationCreateInfo alloc_info) {
    vk::BufferCreateInfo buffer_create_info {
        {},                    // Flags
        object_count * object_size,                                 // Size
        usage,    // Usage
        vk::SharingMode::eExclusive                // Sharing mode
    };

    alloc_info.usage = VMA_MEMORY_USAGE_AUTO;

    return {m_allocator, buffer_create_info, object_size, object_count, alloc_info };
}

VulkanBuffer VulkanContext::createCPUAccessibleBuffer(vk::BufferUsageFlags usage, const u32 object_size, const u32 object_count) {
    VmaAllocationCreateInfo alloc_info = {};
    alloc_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

    return createBuffer(usage, object_size, object_count, alloc_info);
}

void VulkanContext::createVmaAllocator() {
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
    allocatorCreateInfo.physicalDevice = m_physical_device;
    allocatorCreateInfo.device = m_device;
    allocatorCreateInfo.instance = m_instance;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    vmaCreateAllocator(&allocatorCreateInfo, &m_allocator);
}


void VulkanContext::destroyVmaAllocator() {
    vmaDestroyAllocator(m_allocator);
}

void VulkanContext::destroyImage(VulkanImage image) {
    vmaDestroyImage(m_allocator, image.image, image.allocation);
}

void VulkanContext::createComputeCommandPool() {
    vk::CommandPoolCreateInfo create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_queues.indices.compute_family.value());
    m_compute_command_pool = m_device.createCommandPool(create_info);
}

void VulkanContext::createGraphicsCommandPool() {
    vk::CommandPoolCreateInfo create_info(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, m_queues.indices.graphics_family.value());
    m_graphics_command_pool = m_device.createCommandPool(create_info);
}

