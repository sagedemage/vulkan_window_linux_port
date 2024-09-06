#ifndef TRIANGLE_APPLICATION_H
#define TRIANGLE_APPLICATION_H

/* Third party libraries */
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIUS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

/* Standard libraries */
#include <algorithm>  // Required for std::clamp
#include <array>
#include <cstdint>  // Required for uint32_t
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>  // Required for std::numeric_limits
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// Enable the standard diagnostic layers provided by the Vulkan SDK
const std::array<const char*, 1> VALIDATION_LAYERS = {
    "VK_LAYER_KHRONOS_validation"};

// Declare a list of required device extensions
const std::array<const char*, 3> DEVICE_EXTENSIONS = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
    VK_EXT_GRAPHICS_PIPELINE_LIBRARY_EXTENSION_NAME,
};

#define NDEBUG

#ifndef NDEBUG  // not debug
const bool ENABLE_VALIDATION_LAYERS = false;
#else
const bool ENABLE_VALIDATION_LAYERS = true;
#endif

const unsigned int MAX_FRAMES_IN_FLIGHT = 2;

class TriangleApplication {
   private:
    GLFWwindow* window{};
    VkInstance instance{};
    VkDebugUtilsMessengerEXT debug_messenger{};
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;
    VkDevice device{};
    VkQueue graphics_queue{};
    VkSurfaceKHR surface{};
    VkQueue present_queue{};
    VkSwapchainKHR swap_chain{};
    std::vector<VkImage> swap_chain_images;
    std::vector<VkImageView> swap_chain_image_views;
    VkFormat swap_chain_image_format{};
    VkExtent2D swap_chain_extent{};
    VkRenderPass render_pass{};
    VkPipelineLayout pipeline_layout{};
    VkPipeline graphics_pipeline{};
    std::vector<VkFramebuffer> swap_chain_framebuffers;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> command_buffers;
    std::vector<VkSemaphore> image_available_semaphores;
    std::vector<VkSemaphore> render_finished_semaphores;
    std::vector<VkFence> in_flight_fences;
    uint32_t current_frame = 0;

    bool framebuffer_resized = false;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphics_family;
        std::optional<uint32_t> present_family;

        bool IsComplete();
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> present_modes;
    };

    void InitWindow();
    void InitVulkan();
    void MainLoop();
    void CleanUp();
    static void CheckExtensionSupport();
    void CreateInstance();
    static bool CheckValidationLayerSupport();
    static std::vector<const char*> GetRequiredExtensions();
    static VKAPI_ATTR VkBool32 VKAPI_CALL
    DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                  VkDebugUtilsMessageTypeFlagsEXT message_type,
                  const VkDebugUtilsMessengerCallbackDataEXT* p_callback_data,
                  void* p_user_data);
    void SetupDebugMessenger();
    static VkResult CreateDebugUtilsMessengerEXT(
        VkInstance instance,
        const VkDebugUtilsMessengerCreateInfoEXT* p_create_info,
        const VkAllocationCallbacks* p_allocator,
        VkDebugUtilsMessengerEXT* p_debug_messenger);
    static void DestroyDebugUtilsMessengerEXT(
        VkInstance instance, VkDebugUtilsMessengerEXT debug_messenger,
        const VkAllocationCallbacks* p_allocator);
    static void PopulateDebugMessengerCreateInfo(
        VkDebugUtilsMessengerCreateInfoEXT& create_info);
    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    void CreateLogicalDevice();
    void CreateSurface();
    static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    static VkSurfaceFormatKHR ChooseSwapSurfaceFormat(
        const std::vector<VkSurfaceFormatKHR>& available_formats);
    static VkPresentModeKHR ChooseSwapPresentMode(
        const std::vector<VkPresentModeKHR>& available_present_modes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void CreateSwapChain();
    void CreateImageViews();
    void CreateGraphicsPipeline();
    static std::vector<char> ReadFile(const std::string& filename);
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void RecordCommandBuffer(VkCommandBuffer command_buffer,
                             uint32_t image_index);
    void DrawFrame();
    void CreateSyncObjects();
    void RecreateSwapChain();
    void CleanupSwapChain();
    static void FramebufferResizeCallback(GLFWwindow* window, int width,
                                          int height);

   public:
    void Run();
};

#endif  // TRIANGLE_APPLICATION_H
