#include "App.h"

#include <iostream>
#include <limits>
#include <algorithm>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

App::App() {
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
}

App::~App() {
    m_device.destroy();

    m_instance.destroySurfaceKHR(m_surface);

    if (m_debug_messenger.has_value()) {
        m_instance.destroyDebugUtilsMessengerEXT(m_debug_messenger.value());
    }

    m_instance.destroy();

    destroyWindow();
}

void App::createInstance() {
    u32 glfw_extension_count = 0;
    const char** glfw_extensions;

    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    if (enable_validation_layers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    if (enable_validation_layers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested, but not available");
    }

    vk::ApplicationInfo app_info("Physically-based Simulation Project",
                                 1,
                                 "Physically-based Simulation Project",
                                 1,
                                 VK_MAKE_VERSION(1, 1, 0));
    vk::InstanceCreateInfo create_info({},
                                       &app_info,
                                       0,
                                       nullptr,
                                       static_cast<u32>(extensions.size()),
                                       extensions.data());

    if (enable_validation_layers) {
        create_info.enabledLayerCount = static_cast<u32>(validation_layers.size());
        create_info.ppEnabledLayerNames = validation_layers.data();
    }

    m_instance = vk::createInstance(create_info);
}

VKAPI_ATTR VkBool32 VKAPI_CALL App::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                                    VkDebugUtilsMessageTypeFlagBitsEXT message_type,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
                                                    void* user_data) {
    if (message_severity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        std::cerr << "validation layer: " << callback_data->pMessage << std::endl;

    return VK_FALSE;
}

void App::initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    const u32 WIDTH = 800;
    const u32 HEIGHT = 600;

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

    assert(window != nullptr);

    m_window = window;
}

void App::destroyWindow() {
    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void App::mainLoop() {
    while(!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
    }
}

bool App::checkValidationLayerSupport() {
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

void App::setupDebugMessenger() {
    if (!enable_validation_layers) {
        m_debug_messenger =  {};
    }

    using severity = vk::DebugUtilsMessageSeverityFlagBitsEXT;
    using type = vk::DebugUtilsMessageTypeFlagBitsEXT;

    vk::DebugUtilsMessengerCreateInfoEXT create_info(
        {},
        severity::eVerbose | severity::eWarning | severity::eError,
        type::eGeneral | type::ePerformance | type::eValidation,
        (PFN_vkDebugUtilsMessengerCallbackEXT) debugCallback,
        nullptr
    );

     m_debug_messenger = m_instance.createDebugUtilsMessengerEXT(create_info, nullptr);
}

vk::PhysicalDevice App::pickPhysicalDevice() {
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

QueueFamilyIndices App::findQueueFamilies(const vk::PhysicalDevice &device) {
    QueueFamilyIndices indices;

    auto queue_families = device.getQueueFamilyProperties();
    u32 i = 0;
    for (const auto& family : queue_families) {
        if (family.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphics_family = i;
        }

        bool present_support = device.getSurfaceSupportKHR(i, m_surface);
        if (present_support) {
            indices.present_family = i;
        }

        if (indices.isComplete())
            break;

        i++;
    }

    return indices;
}

void App::createDevice() {
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

    m_device = device;
    m_queues = queues;
}

void App::createSurface() {
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface");
    }

    m_surface = surface;
}

bool App::checkDeviceExtensionSupport(const vk::PhysicalDevice& device) {
    auto extensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());

    for (const auto& extension : extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

SwapchainSupportDetails App::querySwapchainSupport(const vk::PhysicalDevice& device) {
    SwapchainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(m_surface);
    details.formats = device.getSurfaceFormatsKHR(m_surface);
    details.present_modes = device.getSurfacePresentModesKHR(m_surface);

    return details;
}

vk::SurfaceFormatKHR App::chooseSwapchainFormat(const std::vector<vk::SurfaceFormatKHR>& available_formats) {
    for (const auto& available_format : available_formats) {
        if (available_format.format == vk::Format::eB8G8R8A8Srgb && available_format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return available_format;
        }
    }

    return available_formats[0];
}

vk::PresentModeKHR App::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& available_present_modes) {
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == vk::PresentModeKHR::eMailbox) {
            return available_present_mode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D App::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
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

void App::createSwapchain() {
    // TODO: Implement
}

/*
void App:createComputePipeline() {
    vk::PipelineCacheCreateInfo cache_create_info({}, 0, nullptr);

    vk::PipelineCache cache = m_device.createPipelineCache(cache_create_info);
    vk::ComputePipelineCreateInfo create_info({}, //TODO);
    auto compute_pipeline = m_device.createComputePipeline(cache, create_info);
}
 */