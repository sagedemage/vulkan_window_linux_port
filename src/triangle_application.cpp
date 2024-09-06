/* Local header files */
#include "triangle_application.hpp"

bool TriangleApplication::QueueFamilyIndices::IsComplete() {
    // Checks if the graphicsFamily and presentFamily objects
    // contain a value
    return graphics_family.has_value() && present_family.has_value();
}

void TriangleApplication::Run() {
    InitWindow();
    InitVulkan();
    MainLoop();
    CleanUp();
}

void TriangleApplication::InitWindow() {
    /* Initialize the GLFW window */
    // Initialize GLFW libary
    glfwInit();

    // Inform GLFW to not create an OpenGL context
    // GLFW was originally designed to create an OpenGL context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // Enable handling resized windows
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    // Create the GLFW window
    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan window", nullptr, nullptr);

    // Get reference to the GLFWwindow (pointer)
    glfwSetWindowUserPointer(window, this);

    // Detect resizes
    glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
}

void TriangleApplication::InitVulkan() {
    /* Initialize Vulkan */
    CreateInstance();
    SetupDebugMessenger();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    CreateImageViews();
    CreateRenderPass();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void TriangleApplication::MainLoop() {
    /* Main game loop */
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        DrawFrame();
    }

    /* This helps to prevent any asynchronous issues with drawing a frame
    with the drawFrame method. It is not a good idea to clean up resources
    while drawing and presenation operations are happening. */

    // Wait for operations in a specific command queue to be finished
    vkDeviceWaitIdle(device);
}

void TriangleApplication::CleanUp() {
    /* Clean up resources */
    CleanupSwapChain();

    vkDestroyPipeline(device, graphics_pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeline_layout, nullptr);

    vkDestroyRenderPass(device, render_pass, nullptr);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, image_available_semaphores[i], nullptr);
        vkDestroySemaphore(device, render_finished_semaphores[i], nullptr);
        vkDestroyFence(device, in_flight_fences[i], nullptr);
    }

    vkDestroyCommandPool(device, command_pool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (ENABLE_VALIDATION_LAYERS) {
        DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void TriangleApplication::CheckExtensionSupport() {
    /* Checking for extension support */
    // Count the amount of supported extensions
    uint32_t extension_count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count, nullptr);

    // Query extension details
    std::vector<VkExtensionProperties> extensions(extension_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &extension_count,
                                           extensions.data());

    std::cout << "available extensions: " << std::endl;
    for (const auto& extension : extensions) {
        std::cout << '\t' << extension.extensionName << '\n';
    }
}

void TriangleApplication::CreateInstance() {
    /* Create instance */
    if (ENABLE_VALIDATION_LAYERS && !CheckValidationLayerSupport()) {
        throw std::runtime_error(
            "validation layers requested, but not available!");
    }

    // Fill in some information about the application
    // This data is optional.
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    // Informs the Vulkan driver which global extensions and
    // validation layers we want to use
    VkInstanceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    // Retreive the required list of extensions
    auto extensions = GetRequiredExtensions();

    // Add the extensions to inferface with the window system for GLFW
    create_info.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};

    // Modify the VkInstanceCreateInfo struct to include the validation layer
    // names if they are enabled
    if (ENABLE_VALIDATION_LAYERS) {
        create_info.enabledLayerCount =
            static_cast<uint32_t>(VALIDATION_LAYERS.size());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();

        PopulateDebugMessengerCreateInfo(debug_create_info);
        create_info.pNext = &debug_create_info;
    } else {
        create_info.enabledLayerCount = 0;
        create_info.pNext = nullptr;
    }

    // Create an instance
    if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error(
            "vkCreateInstance ERROR: failed to create instance!");
    }

    // CheckExtensionSupport();
}

bool TriangleApplication::CheckValidationLayerSupport() {
    /* Checks if all of the requested layers are available */
    // List all of the available layers
    uint32_t layer_count = 0;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    // Check if all of the layers in the validationLayers exist in the
    // availableLayers list
    for (const char* layer_name : VALIDATION_LAYERS) {
        bool layer_found = false;

        for (const auto& layer_properties : available_layers) {
            if (strcmp(layer_name, layer_properties.layerName) == 0) {
                layer_found = true;
                break;
            }
        }

        if (!layer_found) {
            return false;
        }
    }

    return true;
}

std::vector<const char*> TriangleApplication::GetRequiredExtensions() {
    /* Retrieve the required list of extensions based on if the
    validation layers are enabled or disabled */
    uint32_t glfw_extension_count = 0;
    const char** glfw_extensions = nullptr;

    // This function returns an array of required Vulkan instance extensions
    // for creating Vulkan surfaces on GLFW windows
    glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions,
                                        glfw_extensions + glfw_extension_count);

    if (ENABLE_VALIDATION_LAYERS) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}

VKAPI_ATTR VkBool32 VKAPI_CALL TriangleApplication::DebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
    VkDebugUtilsMessageTypeFlagsEXT message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
    void* p_user_data) {
    /* Debug to the console for validation layers */
    std::string debug_msg = "validation layer: " +
                            static_cast<std::string>(p_callback_data->pMessage);
    std::cerr << debug_msg << std::endl;

    if (message_severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        // Message is important enought to show
        debug_msg = "validation layer ERROR: " +
                    static_cast<std::string>(p_callback_data->pMessage);
        throw std::runtime_error(debug_msg.c_str());
    }

    return VK_FALSE;
}

void TriangleApplication::SetupDebugMessenger() {
    if (!ENABLE_VALIDATION_LAYERS) {
        return;
    }

    // Fill in the details about the messenger and its callback
    VkDebugUtilsMessengerCreateInfoEXT create_info{};
    PopulateDebugMessengerCreateInfo(create_info);

    // Create the extension object if it is available
    if (CreateDebugUtilsMessengerEXT(instance, &create_info, nullptr,
                                     &debug_messenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

VkResult TriangleApplication::CreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
    const VkAllocationCallbacks* p_allocator,
    VkDebugUtilsMessengerEXT* p_debug_messenger) {
    /* Proxy function to create the debug messenger */
    // Looks up the address of the VkDebugUtilsMessengerEXT object
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_debug_messenger);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void TriangleApplication::DestroyDebugUtilsMessengerEXT(
    VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
    const VkAllocationCallbacks* p_allocator) {
    /* Proxy function to destroy the debug messenger */
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

    if (func != nullptr) {
        func(instance, debug_messenger, p_allocator);
    }
}

void TriangleApplication::PopulateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT& create_info) {
    /* Fill in the details about the messenger and its callback to the structure
     */
    create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    create_info.pfnUserCallback = DebugCallback;
}

void TriangleApplication::PickPhysicalDevice() {
    // Count the number of graphics cards with Vulkan support
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

    if (device_count == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    // Allocate an array to hold all of the VkPhysicalDevice handles
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(instance, &device_count, devices.data());

    // Device suitability checks
    for (const auto& device : devices) {
        if (IsDeviceSuitable(device)) {
            physical_device = device;
            break;
        }
    }

    if (physical_device == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }
}

bool TriangleApplication::IsDeviceSuitable(VkPhysicalDevice device) {
    /* Queue family lookup function to ensure the device can process the
    commands to use */
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensions_supported = CheckDeviceExtensionSupport(device);

    // Verify swap chain support is adequate
    bool swap_chain_adequate = false;
    if (extensions_supported) {
        SwapChainSupportDetails swap_chain_support =
            QuerySwapChainSupport(device);
        swap_chain_adequate = !swap_chain_support.formats.empty() &&
                              !swap_chain_support.present_modes.empty();
    }

    return indices.IsComplete() && extensions_supported && swap_chain_adequate;
}

TriangleApplication::QueueFamilyIndices TriangleApplication::FindQueueFamilies(
    VkPhysicalDevice device) {
    /* Logic to find queue family indices to populate the struct with */
    QueueFamilyIndices indices;

    // The process to retreive the list of queue families
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count,
                                             queue_families.data());

    // Find at lest one queue family that supports VK_QUEUE_GRAPHICS_BIT
    int i = 0;
    for (const auto& queue_family : queue_families) {
        if (queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }

        // Look for a queue family that is capable of presenting to the window
        // surface
        VkBool32 present_support = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface,
                                             &present_support);

        if (present_support) {
            indices.present_family = i;
        }

        if (indices.IsComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

void TriangleApplication::CreateLogicalDevice() {
    // Specify the queues to be created
    QueueFamilyIndices indices = FindQueueFamilies(physical_device);

    // Set multiple VkDeviceQueueCreateInfo structs to create a queue
    // from both families which are mandatory for the required queues
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;

    std::set<uint32_t> unique_queue_families = {};

    if (indices.graphics_family.has_value() &&
        indices.present_family.has_value()) {
        unique_queue_families.insert(indices.graphics_family.value());
        unique_queue_families.insert(indices.present_family.value());
    } else {
        throw std::runtime_error(
            "Indices's graphics and present families contain no value!");
    }

    float queue_priority = 1.0F;

    for (uint32_t queue_family : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_create_info{};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = queue_family;
        queue_create_info.queueCount = 1;
        queue_create_info.pQueuePriorities = &queue_priority;
        queue_create_infos.push_back(queue_create_info);
    }

    // Specify the device features to be used
    VkPhysicalDeviceFeatures device_features{};

    // Create the logical device
    VkDeviceCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount =
        static_cast<uint32_t>(queue_create_infos.size());
    create_info.pQueueCreateInfos = queue_create_infos.data();
    create_info.pEnabledFeatures = &device_features;

    // Enabling device extensions
    // Using a swapchain requires enabling the VK_KHR_swapchain
    create_info.enabledExtensionCount =
        static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
    create_info.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();

    // Specify the validation layers for the logical device if the validation
    // layers is enabled
    if (ENABLE_VALIDATION_LAYERS) {
        create_info.enabledLayerCount =
            static_cast<uint32_t>(VALIDATION_LAYERS.size());
        create_info.ppEnabledLayerNames = VALIDATION_LAYERS.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    // Instantiate the logical device
    if (vkCreateDevice(physical_device, &create_info, nullptr, &device) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    if (indices.graphics_family.has_value() &&
        indices.present_family.has_value()) {
        // Retrieve queue handles
        vkGetDeviceQueue(device, indices.graphics_family.value(), 0,
                         &graphics_queue);
        vkGetDeviceQueue(device, indices.present_family.value(), 0,
                         &present_queue);
    } else {
        throw std::runtime_error(
            "Indices's graphics and present Families contain no value!");
    }
}

void TriangleApplication::CreateSurface() {
    // Cross platform way to create the window surface via GLFW
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

bool TriangleApplication::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    /* Enumerate the extensions and check if all of the required
    extensions are included in them */
    uint32_t extension_count = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                         nullptr);

    std::vector<VkExtensionProperties> available_extensions(extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count,
                                         available_extensions.data());

    std::set<std::string> required_extensions(DEVICE_EXTENSIONS.begin(),
                                              DEVICE_EXTENSIONS.end());

    for (const auto& extension : available_extensions) {
        required_extensions.erase(extension.extensionName);
    }

    return required_extensions.empty();
}

TriangleApplication::SwapChainSupportDetails
TriangleApplication::QuerySwapChainSupport(VkPhysicalDevice device) {
    TriangleApplication::SwapChainSupportDetails details;

    // Query basic surface capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    // Querying the supported surface formats
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                         nullptr);

    if (format_count != 0) {
        details.formats.resize(format_count);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &format_count,
                                             details.formats.data());
    }

    // Querying the supported presentation modes
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &present_mode_count, nullptr);

    if (present_mode_count != 0) {
        details.present_modes.resize(present_mode_count);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

VkSurfaceFormatKHR TriangleApplication::ChooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& available_formats) {
    // Go through a list to find if the preferred combination is available
    //
    // VK_COLOR_SPACE_SRGB_NONLINEAR_KHR: SRGB color space is supported
    // or not supported
    //
    // VK_FORMAT_B8G8R8A8_SRGB: store the BGRA channels in that order
    // with each channels containing a 8 bit unsigned integer.
    // This would result in a total of 32 bits per pixel.
    for (const auto& available_format : available_formats) {
        if (available_format.format == VK_FORMAT_B8G8R8A8_SRGB &&
            available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return available_format;
        }
    }

    return available_formats[0];
}

VkPresentModeKHR TriangleApplication::ChooseSwapPresentMode(
    const std::vector<VkPresentModeKHR>& available_present_modes) {
    // VK_PRESENT_MODE_MAILBOX_KHR: This helps to avoid tearing and maintain a
    // low letency. Use this if energy is not an issue.
    //
    // VK_PRESENT_MODE_FIFO_KHR: If energy usage is an issue, use this.
    // This is recommended for mobile devices.
    for (const auto& available_present_mode : available_present_modes) {
        if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return available_present_mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D TriangleApplication::ChooseSwapExtent(
    const VkSurfaceCapabilitiesKHR& capabilities) {
    // The swap extent is the resolution of the swap chain images and it's
    // usually always exactly equal to the resolution of the window that the
    // program draws in pixels.
    //
    // Using a high CPI display like Apple's Retina display), screen coordinates
    // don't correspond to pixels. The issue is that the resolution of the
    // window in pixels will be larger than the resolution in screen
    // coordinates.
    // The orginal WIDTH and HEIGHT variables will not work.
    // The glfwGetFramebufferSize function is required to query the resolution
    // of the window in pixels.
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actual_extent = {static_cast<uint32_t>(width),
                                static_cast<uint32_t>(height)};

    actual_extent.width =
        std::clamp(actual_extent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);

    actual_extent.height =
        std::clamp(actual_extent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actual_extent;
}

void TriangleApplication::CreateSwapChain() {
    SwapChainSupportDetails swap_chain_support =
        QuerySwapChainSupport(physical_device);
    VkSurfaceFormatKHR surface_format =
        ChooseSwapSurfaceFormat(swap_chain_support.formats);
    VkPresentModeKHR present_mode =
        ChooseSwapPresentMode(swap_chain_support.present_modes);
    VkExtent2D extent = ChooseSwapExtent(swap_chain_support.capabilities);

    // Decide how many images the program would like to have in the swap chain.
    // Request one more image than the minimum to prevent waiting on the driver
    // to complete internal operations before we can get another image to render
    // to.
    uint32_t image_count = swap_chain_support.capabilities.minImageCount + 1;

    // Make sure the program does not exceed the maximum number of images while
    // doing this. 0 means there is no maximum.
    if (swap_chain_support.capabilities.maxImageCount > 0 &&
        image_count > swap_chain_support.capabilities.maxImageCount) {
        image_count = swap_chain_support.capabilities.maxImageCount;
    }

    // Specify the details of the swap chain
    VkSwapchainCreateInfoKHR create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = surface;

    // Specify the details of the swap chain images
    create_info.minImageCount = image_count;
    create_info.imageFormat = surface_format.format;
    create_info.imageColorSpace = surface_format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    // Handle swap chain images that will be used across multiple queue
    // families.
    //
    // VK_SHARING_MODE_EXCLUSIVE: An image is owned by one queue family at a
    // time and this requires explicit ownership tranfers. This option offers
    // the best performance.
    //
    // VK_SHARING_MODE_CONCURRENT: Images can be used across multiple queue
    // faimilies without explicit onwership tranfers.

    QueueFamilyIndices indices = FindQueueFamilies(physical_device);

    std::array<uint32_t, 2> queue_family_indices = {0};

    if (indices.graphics_family.has_value() &&
        indices.present_family.has_value()) {
        queue_family_indices[0] = indices.graphics_family.value();
        queue_family_indices[1] = indices.present_family.value();
    } else {
        throw std::runtime_error(
            "Indices's graphics and present Families contain no value!");
    }

    if (indices.graphics_family != indices.present_family) {
        create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        create_info.queueFamilyIndexCount = 2;
        create_info.pQueueFamilyIndices = queue_family_indices.data();
    } else {
        create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;      // Optional
        create_info.pQueueFamilyIndices = nullptr;  // Optional
    }

    // Specify the transformation to be applied to images in the
    // swap chain if it is supported
    create_info.preTransform = swap_chain_support.capabilities.currentTransform;

    // Specify if the alpha channel should be used for blending with other
    // windows in the window system. It is a good idea to ignore the alpha
    // channel via VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR.
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Specify the presentation engine.
    // Enabling clipping provides the program the best performace.
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;

    // Existing non-retried swapchain currently associated with the surface
    create_info.oldSwapchain = VK_NULL_HANDLE;

    // Create the swap chain
    if (vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    // Retrieve the swap chain images
    // 1. First query the final number of images via vkGetSwapchainImagesKHR.
    // 2. Resize the container.
    // 3. Retrieve the handles via the vkGetSwapchainImagesKHR again.
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count, nullptr);
    swap_chain_images.resize(image_count);
    vkGetSwapchainImagesKHR(device, swap_chain, &image_count,
                            swap_chain_images.data());

    // Store the format and extent for the swap chain images
    swap_chain_image_format = surface_format.format;
    swap_chain_extent = extent;
}

void TriangleApplication::CreateImageViews() {
    swap_chain_image_views.resize(swap_chain_images.size());

    for (size_t i = 0; i < swap_chain_images.size(); i++) {
        // Parameters for image view creation
        VkImageViewCreateInfo create_info{};
        create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.image = swap_chain_images[i];

        // Specify how the image data should be interpreted
        create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        create_info.format = swap_chain_image_format;

        // Swizzle the color channels around
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // The subresource describes the image's purpose
        // and which part of the image should be accessed.
        //
        // The images will be used as color targets without
        // any mipmapping levels or multiple layers.
        create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        create_info.subresourceRange.baseMipLevel = 0;
        create_info.subresourceRange.levelCount = 1;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;

        // Create the image view
        if (vkCreateImageView(device, &create_info, nullptr,
                              &swap_chain_image_views[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image views!");
        }
    }
}

void TriangleApplication::CreateGraphicsPipeline() {
    // Retreive the vertex and fragment shader code
    auto vert_shader_code = ReadFile("shaders/vert.spv");
    auto frag_shader_code = ReadFile("shaders/frag.spv");

    // Create shader modules
    VkShaderModule vert_shader_module = CreateShaderModule(vert_shader_code);
    VkShaderModule frag_shader_module = CreateShaderModule(frag_shader_code);

    // Fill in the structure for the vertex shader
    VkPipelineShaderStageCreateInfo vert_shader_stage_info{};
    vert_shader_stage_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vert_shader_module;
    vert_shader_stage_info.pName = "main";

    // Fill in the structure for the fragment shader
    VkPipelineShaderStageCreateInfo frag_shader_stage_info{};
    frag_shader_stage_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = frag_shader_module;
    frag_shader_stage_info.pName = "main";

    // Define an attray that contains these two structures
    std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages = {
        vert_shader_stage_info, frag_shader_stage_info};

    // Fill in the information for the vertex input
    VkPipelineVertexInputStateCreateInfo vertex_input_info{};
    vertex_input_info.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_info.vertexBindingDescriptionCount = 0;
    vertex_input_info.pVertexBindingDescriptions = nullptr;  // Optional
    vertex_input_info.vertexAttributeDescriptionCount = 0;
    vertex_input_info.pVertexAttributeDescriptions = nullptr;  // Optional

    // Fill in the information for the input assembly
    VkPipelineInputAssemblyStateCreateInfo input_assembly{};
    input_assembly.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly.primitiveRestartEnable = VK_FALSE;

    // Specify viewport and scissor
    // A viewport describes the reigion of the framebuffer that the output will
    // be rendered to.
    //
    // Difference between Viewport and Scissor
    // A viewport define the transformation from the image to the framebuffer
    // A scissor define which regions pixels will actually be stored

    // Fill in the information for viewport state
    VkPipelineViewportStateCreateInfo viewport_state{};
    viewport_state.sType =
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state.viewportCount = 1;
    viewport_state.scissorCount = 1;

    // Fill in the information for the rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    // polygonMode termines how the fragments are generated for geometry.
    // The following modes are available:
    // VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
    // VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
    // VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0F;
    // The cullMode varaible determines the type of face culling to use
    // You can disable culling, cull the front faces, cull the back faces
    // or both
    // The frontFace variable specifies the vertex order for faces to be
    // considered front-facing.
    // It can be clockwise or counterclockwise
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0F;  // Optional
    rasterizer.depthBiasClamp = 0.0F;           // Optional
    rasterizer.depthBiasSlopeFactor = 0.0F;     // Optional

    // Fill in the information for multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0F;           // Optional
    multisampling.pSampleMask = nullptr;             // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // Optional
    multisampling.alphaToOneEnable = VK_FALSE;       // Optional

    // Color blending is when after a fragment shader has returned a color,
    // it needs to be combined with the color that is already in the
    // framebuffer.

    // Configure color blending
    VkPipelineColorBlendAttachmentState color_blend_attachment{};
    color_blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor =
        VK_BLEND_FACTOR_ONE;  // Optional
    color_blend_attachment.dstColorBlendFactor =
        VK_BLEND_FACTOR_ZERO;                               // Optional
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;  // Optional
    color_blend_attachment.srcAlphaBlendFactor =
        VK_BLEND_FACTOR_ONE;  // Optional
    color_blend_attachment.dstAlphaBlendFactor =
        VK_BLEND_FACTOR_ZERO;                               // Optional
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;  // Optional

    // Fill in the information for color blending state
    VkPipelineColorBlendStateCreateInfo color_blending{};
    color_blending.sType =
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blending.logicOpEnable = VK_FALSE;
    color_blending.logicOp = VK_LOGIC_OP_COPY;  // Optional
    color_blending.attachmentCount = 1;
    color_blending.pAttachments = &color_blend_attachment;
    color_blending.blendConstants[0] = 0.0F;  // Optional
    color_blending.blendConstants[1] = 0.0F;  // Optional
    color_blending.blendConstants[2] = 0.0F;  // Optional
    color_blending.blendConstants[3] = 0.0F;  // Optional

    // Dynamic State
    // Fill in the dynamic state's information
    std::vector<VkDynamicState> dynamic_states = {VK_DYNAMIC_STATE_VIEWPORT,
                                                  VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamic_state{};
    dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state.dynamicStateCount =
        static_cast<uint32_t>(dynamic_states.size());
    dynamic_state.pDynamicStates = dynamic_states.data();

    // Fill in the information for the pipeline layout
    VkPipelineLayoutCreateInfo pipeline_layout_info;
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_info.setLayoutCount = 0;             // Optional
    pipeline_layout_info.pSetLayouts = nullptr;          // Optional
    pipeline_layout_info.pushConstantRangeCount = 0;     // Optional
    pipeline_layout_info.pPushConstantRanges = nullptr;  // Optional
    pipeline_layout_info.flags =
        VK_PIPELINE_LAYOUT_CREATE_INDEPENDENT_SETS_BIT_EXT;
    pipeline_layout_info.pNext = NULL;

    // Create pipeline layout
    if (vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr,
                               &pipeline_layout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    // Describe the graphics pipeline information
    VkGraphicsPipelineCreateInfo pipeline_info{};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.pVertexInputState = &vertex_input_info;
    pipeline_info.pInputAssemblyState = &input_assembly;
    pipeline_info.pViewportState = &viewport_state;
    pipeline_info.pRasterizationState = &rasterizer;
    pipeline_info.pMultisampleState = &multisampling;
    pipeline_info.pDepthStencilState = nullptr;  // Optional
    pipeline_info.pColorBlendState = &color_blending;
    pipeline_info.pDynamicState = &dynamic_state;
    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass;
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;  // Optional;
    pipeline_info.basePipelineIndex = -1;               // Optional

    // Create graphics pipeline
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info,
                                  nullptr, &graphics_pipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // Destroy shader modules
    vkDestroyShaderModule(device, frag_shader_module, nullptr);
    vkDestroyShaderModule(device, vert_shader_module, nullptr);
}

std::vector<char> TriangleApplication::ReadFile(const std::string& filename) {
    // load binary data from a file
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file: " + filename + "!");
    }

    // determine the size of the file and allocate a buffer
    std::streamsize file_size = static_cast<std::streamsize>(file.tellg());
    std::vector<char> buffer(file_size);

    // seek back to the beginning of the file and read all of the bytes
    // seekg: sets the position of the next chracter to be extracted
    // from the input stream
    file.seekg(0);
    file.read(buffer.data(), file_size);

    file.close();
    return buffer;
}

VkShaderModule TriangleApplication::CreateShaderModule(
    const std::vector<char>& code) {
    // Specify the information for the shader module
    VkShaderModuleCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    create_info.codeSize = code.size();
    create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());

    // Create shader module
    VkShaderModule shader_module = nullptr;
    if (vkCreateShaderModule(device, &create_info, nullptr, &shader_module) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }
    return shader_module;
}

void TriangleApplication::CreateRenderPass() {
    /* Attachement description */
    // Describe the color buffer attachment represented by one of the images
    // from the swap chain
    VkAttachmentDescription color_attachment{};
    color_attachment.format = swap_chain_image_format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;

    // loadOp and storeOp determine what to do with the data in the attachment
    // before and after rendering
    /*
    The following choices for loadOp:
    - VK_ATTACHMENT_LOAD_OP_LOAD: Preserve the existing contents of the
    attachment
    - VK_ATTACHMENT_LOAD_OP_CLEAR: Clear the values to a constant at the start
    - VK_ATTACHMENT_LOAD_OP_DONT_CARE: Existing contents are undefined
    */

    /*
    The two choices for storeOp:
    - VK_ATTACHMENT_STORE_OP_STORE: Rendered contents will be stored in memeory
    and can be read later
    - VK_ATTACHMENT_STORE_OP_DONT_CARE: Contetns of the framebuffer will be
    undefined after the rendering operation
    */
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // loadOp and storeOp apply to the color and depth data and stencilLoadOp
    // and stencilStoreOp apply to the stencil data.
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Textures and framebuffers are represented by VkImage objects with a
    // certain pixel format. However the layout of the pixels in memeory can
    // change based on what you're trying to do with an image.

    /* Common layouts are:
    - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Images used as color attachment
    - VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Images to be presented in the swap chain
    - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Images to be used as destinatino for
    a memory copy operation
    - VK_IMAGE_LAYOUT_UNDEFINED: we don't care about the layout of the image
    */

    // The initialLayout specifies which layout the image will contain before
    // the render pass begins. The finalLayout specifies the layout to
    // automatically transition to when the render pass finishes.
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    /* Subpasses and attachment references */
    // Describe the color attachement references.
    // The attachment parameter specifies the attachment to reference by its
    // index.
    // The layout specifies which layout we would like the attachment to
    // have during a subpass VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL layout
    // gives the best performance.
    VkAttachmentReference color_attachment_ref{};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Describe the subpass
    VkSubpassDescription subpass{};

    // Vulkan may support compute subpasses in the future, we have to be
    // explcit about this being a graphics subpass.
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // The index of the attachement in this array is directly referenced from
    // the fragment shader with the layout(location = 0) out vec4 outColor
    // directive!
    /* The other types of attachements that can be referenced by a subpass:
    - pInputAttachments: Attachements that are read from a shader
    - pResolveAttachments: Attachments used for multisampling color attachments
    - pDepthStencilAttachment: Attachment for depth and stencil data
    - pPreserveAttachments: Attachments that are not used by this subpass, but
    for which the data must be preserved
    */
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    /* Subpass dependencies */
    VkSubpassDependency dependency{};

    // The two fields specify the indices of the dependency and the
    // dependent subpass.
    // The special value VK_SUBPASS_EXTERNAL refers to implicit subpass before
    // or after the render pass depending on whether it is specifed in
    // srcSubpass or dstSubpass.
    // The index 0 refers to the first and only one subpass.
    // The dstSubpass must always be higher than srcSubpass to prevent cycles in
    // the dependency graph (unless one of the subpasses is
    // VK_SUBPASS_EXTERNAL).
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;

    // The two fields specify the operations to wait on and the stages in which
    // these operations occur. We need to wait for the swap chain to finish
    // reading from the image before we can access it. This can be accomplished
    // by waiting on the color attachment output stage itself.
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    // The operations that should wait on this are in the color attachment stage
    // and involve the writing of the color attachment. These settings will
    // prevent the transition from happening until it's actually necessary (and
    // allowed): when we want to start writing colors to it.
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    /* Render pass */
    // Describe the informatioon for the render pass
    VkRenderPassCreateInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void TriangleApplication::CreateFramebuffers() {
    // Resize the container to hold all of the framebuffers
    swap_chain_framebuffers.resize(swap_chain_image_views.size());

    // iterate through the image views and create the framebuffers from them
    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        std::array<VkImageView, 1> attachments = {swap_chain_image_views[i]};

        // Describe the framebuffer information
        VkFramebufferCreateInfo framebuffer_info{};
        framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_info.renderPass = render_pass;
        framebuffer_info.attachmentCount = 1;
        framebuffer_info.pAttachments = attachments.data();
        framebuffer_info.width = swap_chain_extent.width;
        framebuffer_info.height = swap_chain_extent.height;
        framebuffer_info.layers = 1;

        // Create the framebuffer
        if (vkCreateFramebuffer(device, &framebuffer_info, nullptr,
                                &swap_chain_framebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void TriangleApplication::CreateCommandPool() {
    QueueFamilyIndices queue_family_indices =
        FindQueueFamilies(physical_device);

    /* Possible flags for command pools:
    - VK_COMMAND_POOL_CREATE_TRANSIENT_BIT: Hint that command buffers are
    rerecorded with new commands very often. This may change memory allocation
    behavior.
    - VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT: Allow command buffers to
    be rerecorded individually. Without this flag, they all have to be reset
    together.
    */

    // This will be recording a command buffer every frame, we want to be able
    // to reset and rerecord over it.
    // The VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT flag bit must
    // be set for the command pool.

    // Describe the pool information
    VkCommandPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (queue_family_indices.graphics_family.has_value()) {
        pool_info.queueFamilyIndex =
            queue_family_indices.graphics_family.value();
    } else {
        throw std::runtime_error("Graphics family has no value!");
    }

    // Create the command pool
    if (vkCreateCommandPool(device, &pool_info, nullptr, &command_pool) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}

void TriangleApplication::CreateCommandBuffers() {
    /* The level parameter specifies if the allocated command bufers are primary
     * or secondary command buffers.
     - VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for
     execution but it cannot be called from the other command buffers.
     - VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but it
     can be called from the primary command buffers.
     */
    // Resize command buffers vector
    command_buffers.resize(MAX_FRAMES_IN_FLIGHT);

    // Describe the allocation information for the command buffers
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount =
        static_cast<uint32_t>(command_buffers.size());

    // Allocate command buffers
    if (vkAllocateCommandBuffers(device, &alloc_info, command_buffers.data()) !=
        VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void TriangleApplication::RecordCommandBuffer(VkCommandBuffer command_buffer,
                                              uint32_t image_index) {
    /* Command buffer recording */
    /* The flags parameter specifies how we're going to use the command buffer.
    The following flags are available:
    - VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer wil be
    rerecorded right after executing it once.
    - VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary
    command buffer that will be entirely within a single render pass.
    - VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be
    resubmitted while it is also already pending an execution.
    */
    // Describe the details about the usage of this specific command buffer
    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = 0;                   // Optional
    begin_info.pInheritanceInfo = nullptr;  // Optional

    // Record the command buffer
    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS) {
        throw std::runtime_error("failed to begin recording command buffer!");
    }

    /* Starting a render pass */
    // Describe the render pass information
    VkRenderPassBeginInfo render_pass_info{};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass;
    render_pass_info.framebuffer = swap_chain_framebuffers[image_index];

    // The two parameters define the size of the render area
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = swap_chain_extent;

    // The two parameters define the clear values to use for
    // VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as the load operation for the
    // color attachment. I've defined the clear color to simply be black with
    // 100% opacity.
    VkClearValue clear_color = {{{0.0F, 0.0F, 0.0F, 1.0F}}};
    render_pass_info.clearValueCount = 1;
    render_pass_info.pClearValues = &clear_color;

    /* The final parameter defines how the drawing commands within the render
    pass will be provided. It can have one of the two values:
    - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in
    the primary command buffer itself and no secondary command buffers will be
    executed.
    - VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass
    commands will be executed from the secondary command buffers.
    */

    // Begin render pass
    vkCmdBeginRenderPass(command_buffer, &render_pass_info,
                         VK_SUBPASS_CONTENTS_INLINE);

    /* Basic draw commands */
    // Bind the graphics pipeline
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      graphics_pipeline);

    // Set the viewport and scissor state in the command buffer before issuing
    // the draw command.
    VkViewport viewport{};
    viewport.x = 0.0F;
    viewport.y = 0.0F;
    viewport.width = static_cast<float>(swap_chain_extent.width);
    viewport.height = static_cast<float>(swap_chain_extent.height);
    viewport.minDepth = 0.0F;
    viewport.maxDepth = 1.0F;
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swap_chain_extent;
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);

    /* The vkCmdDraw function has the following parameters aside from the
     * command buffer:
     - vertexCount: Number of vertices to draw
     - instanceCount: Used for instanced rendering, set 1 if you're not doing
     that.
     - firstVertex: Used as offset into the vertex buffer, defines the lowest
     value of gl_VertexIndex.
     - firstInstance: Used as an offset for instanced rendering, defines the
     lowest value of gl_InstanceIndex.
     */

    // Issue the draw command for the triangle
    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    /* Finishing up */
    // End the render pass
    vkCmdEndRenderPass(command_buffer);

    // Finish recording the command buffer
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to record command buffer!");
    }
}

void TriangleApplication::DrawFrame() {
    /* Outline of a frame
    At a high level, rendering a frame in Vulkan consists of a common set of
    steps:
    - Wait for the previous frame to finish
    - Acquire an image from the swap chain
    - Record a command buffer which draws the scene onto that image
    - Submit the recorded command buffer
    - Present the swap chain image
    */

    // The vkWaitForFences function takes an array of fences and waits on the
    // host for any or all of the fences to be signaled before returning.
    // VK_TRUE indicates that we want to wait for all fences,
    // but in the case of a single one it doesn't matter.
    // This function has a timeout parameter that we set to the maximum
    // value of a 64 bit unsigned integer.
    // UINT64_MAX disables the timeout.

    // Wait until the previous frame has finished, so that the command buffer
    // and semaphores are available to use.
    vkWaitForFences(device, 1, &in_flight_fences[current_frame], VK_TRUE,
                    UINT64_MAX);

    /* Suboptimal or out-of-date swap chain
    The vkAcquireNextImageKHR and vkQueuePresentKHR functions can return the
    following special values to indicate this:
    - VK_ERROR_OUT_OF_DATE_KHR: The swap chain has become incompatible with the
    surface and can no longer be used for rendering. Usually happens after a
    window resize.
    - VK_SUBOPTIMAL_KHR: The swap chain can still be used to
    successfully present to the surface, but the surface properties are no
    longer matched exactly.
    */

    uint32_t image_index = 0;
    VkResult result =
        vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX,
                              image_available_semaphores[current_frame],
                              VK_NULL_HANDLE, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    /* Fixing a deadlock */
    // Only reset the fence if we are submitting work
    vkResetFences(device, 1, &in_flight_fences[current_frame]);

    /* Reecording the command buffer */
    // check if the command buffer is able to be recorded
    vkResetCommandBuffer(command_buffers[current_frame], 0);

    // record the commands
    RecordCommandBuffer(command_buffers[current_frame], image_index);

    /* Submitting the command buffer */
    // Configure queue submission and synchronization
    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    std::array<VkSemaphore, 1> wait_semaphores = {
        image_available_semaphores[current_frame]};
    std::array<VkPipelineStageFlags, 1> wait_stages = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    // The first three parameters specify which semaphores to wait on before
    // the execution begins and in which stage(s) of the pipeline to wait.
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.pWaitDstStageMask = wait_stages.data();

    // The two parameters specify which command buffers to actually submit for
    // execution.
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[current_frame];

    // The signalSemaphoreCount and pSignalSemaphores parameters specify which
    // semaphores to signal once the command buffer(s) have finished execution.
    std::array<VkSemaphore, 1> signal_semaphores = {
        render_finished_semaphores[current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores.data();

    // On the next frame, the CPU will wait for this command buffer to finish
    // executing before it records new commands into it.

    // Submit the command buffer to the graphics queue
    if (vkQueueSubmit(graphics_queue, 1, &submit_info,
                      in_flight_fences[current_frame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    /* Presentation */
    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Two paramets specify which semaphores to wait on before presentation can
    // happen.
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores.data();

    // Two parameters specify the swap chains to present images to and the index
    // of the image for each swap chain. This will almost always be a single
    // one.
    std::array<VkSwapchainKHR, 1> swap_chains = {swap_chain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains.data();
    present_info.pImageIndices = &image_index;

    // pResults allows you to specify an array of VkResult values to check every
    // individual swap chain if presentation was successful.
    // It is not required if you're only using a single swap chain, because you
    // can simply use the return value of the present function.
    present_info.pResults = nullptr;  // Optional

    // Submit the request to present an image to the swap chain.
    result = vkQueuePresentKHR(present_queue, &present_info);

    // Handling resizes explicitly
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        framebuffer_resized) {
        framebuffer_resized = false;
        RecreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    // Advance to the next frame every time
    current_frame = (current_frame + 1) & MAX_FRAMES_IN_FLIGHT;
}

void TriangleApplication::CreateSyncObjects() {
    /* Synchronization
    The number of events that are required to order explicitly because they
    happen on the GPU, such as:
    - Acquire an image from the swap chain
    - Execute commands that draw onto the acquired image
    - Present that image to the screen for presentation, returning it to the
    swapchain
    */

    // Resize the following vectors
    image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    render_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
    in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);

    // Describe semaphore information
    VkSemaphoreCreateInfo semaphore_info{};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // The problem is the vkWaitForFences function blocks indefinitely, waiting
    // on something which will never happen. There is a clever workaround built
    // into the API. Create the fence in the signaled state, so that the first
    // call to vkWaitForFences() returns immediately since the fence is already
    // signaled.
    // To achieve this, we add the VK_FENCE_CREATE_SIGNALED_BIT flag to the
    // VkFenceCreateInfo

    // Describe the fence information
    VkFenceCreateInfo fence_info{};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        // Create semaphores and the fence
        if (vkCreateSemaphore(device, &semaphore_info, nullptr,
                              &image_available_semaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphore_info, nullptr,
                              &render_finished_semaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]) !=
                VK_SUCCESS) {
            throw std::runtime_error(
                "failed to create synchronization objects for a frame!");
        }
    }
}

void TriangleApplication::RecreateSwapChain() {
    /* Recreating the swap chain */
    /* Handling minimization */
    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    CleanupSwapChain();

    CreateSwapChain();
    CreateImageViews();
    CreateFramebuffers();
}

void TriangleApplication::CleanupSwapChain() {
    for (size_t i = 0; i < swap_chain_framebuffers.size(); i++) {
        vkDestroyFramebuffer(device, swap_chain_framebuffers[i], nullptr);
    }

    for (size_t i = 0; i < swap_chain_image_views.size(); i++) {
        vkDestroyImageView(device, swap_chain_image_views[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swap_chain, nullptr);
}

void TriangleApplication::FramebufferResizeCallback(GLFWwindow* window,
                                                    int width, int height) {
    auto* app = reinterpret_cast<TriangleApplication*>(
        glfwGetWindowUserPointer(window));
    app->framebuffer_resized = true;
}