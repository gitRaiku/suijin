#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "fujin.h"      /**  Logging                **/

#define VK_CHECK(call) {VkResult _result = call;assert(_result == VK_SUCCESS);}
#define assert_log(value, message)                {if (!(value)              ) {log_message(10, message); cleanup(10);};}

int32_t windowWidth = 0, windowHeight = 0;

void cleanup(int32_t severity);

GLFWwindow *init_window();
void callback_error(int error, const char *desc);
void callback_window_resize(GLFWwindow *window, int width, int height);

VkSurfaceKHR create_surface(VkInstance *instance, GLFWwindow *window) {
  VkXlibSurfaceCreateInfoKHR create_info = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
  create_info.pNext = NULL;
  create_info.dpy = glfwGetX11Display();
  create_info.window = glfwGetX11Window(window);

  VkSurfaceKHR surface = 0;
  VK_CHECK(vkCreateXlibSurfaceKHR(*instance, &create_info, 0, &surface));
  return surface;
}

VkInstance create_instance() {
	VkApplicationInfo appInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	appInfo.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
  createInfo.pApplicationInfo = &appInfo;
  createInfo.pNext = NULL;

#ifdef _DEBUG
  const char *debugLayers[] = {
		  "VK_LAYER_KHRONOS_validation"
  };
  createInfo.ppEnabledLayerNames = debugLayers;
  createInfo.enabledLayerCount = sizeof(debugLayers) / sizeof(debugLayers[0]);
#endif

  const char *extensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
		  VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
  };

  createInfo.ppEnabledExtensionNames = extensions;
  createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);

  VkInstance instance = 0;
  VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));

  return instance;
}

VkDevice create_device(VkInstance instance, VkPhysicalDevice physicalDevice, uint32_t *__restrict familyIndex) {
    *familyIndex = 0;

    float queuePriorities[] = { 1.0f };
    
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = *familyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = queuePriorities;

    const char *extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueInfo;
    createInfo.ppEnabledExtensionNames = extensions;
    createInfo.enabledExtensionCount = sizeof(extensions) / sizeof(extensions[0]);

    VkDevice device = 0;
    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));
    return device;
}

VkSwapchainKHR create_swapchain(VkDevice *__restrict device, VkSurfaceKHR *__restrict surface, VkPhysicalDevice *__restrict physicalDevice, uint32_t *__restrict familyIndex, uint32_t width, uint32_t height) {
    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = *surface;
    createInfo.minImageCount = 2;
    createInfo.imageExtent.width = width;
    createInfo.imageExtent.height = height;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = familyIndex;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;

    VkSurfaceCapabilitiesKHR surfaceCapabilites;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physicalDevice, *surface, &surfaceCapabilites);
    createInfo.preTransform = surfaceCapabilites.currentTransform;
    createInfo.compositeAlpha = surfaceCapabilites.supportedCompositeAlpha;

    {
      VkSurfaceFormatKHR surfaceFormats[16];
      uint32_t surfaceFormatCount = sizeof(surfaceFormats) / sizeof(surfaceFormats[0]);

      VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, *surface, &surfaceFormatCount, surfaceFormats));
      createInfo.imageFormat = surfaceFormats[0].format;
      createInfo.imageColorSpace = surfaceFormats[0].colorSpace;
    }

    VkSwapchainKHR swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(*device, &createInfo, 0, &swapchain));

    return swapchain;
}

VkSemaphore create_semaphore(VkDevice *__restrict device) {
  VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

  VkSemaphore semaphore = 0;
  VK_CHECK(vkCreateSemaphore(*device, &createInfo, 0, &semaphore));

  return semaphore;
}

VkPhysicalDevice pick_physical_device(VkPhysicalDevice *__restrict physicalDevices, uint32_t physicalDeviceCount) {
  {
    int32_t i;
    VkPhysicalDeviceProperties props;
    for(i = 0; i < physicalDeviceCount; ++i) {
      vkGetPhysicalDeviceProperties(physicalDevices[i], &props);

      if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        log_format(0, "Picking discrete GPU: %s", props.deviceName);
        return physicalDevices[i];
      }
    }
  }

  if (physicalDeviceCount > 0) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevices[0], &props);

    log_format(0, "Picking fallback GPU: %s", props.deviceName);
    return physicalDevices[0];
  }

  log_message(10, "No physical devices available!");
  return 0;
}

void init_vulkan(VkInstance *__restrict instance, VkPhysicalDevice *__restrict physicalDevice, VkDevice *__restrict device, uint32_t *__restrict familyIndex) {
  *instance = create_instance();
  if (*instance == 0) {
    log_message(10, "Could not create a Vulkan instance! Please check that Vulkan is enabled on your system.");
    cleanup(10);
  }

  VkPhysicalDevice physicalDevices[16];
  uint32_t physicalDeviceCount = sizeof(physicalDevices) / sizeof(physicalDevices[0]);
  VK_CHECK(vkEnumeratePhysicalDevices(*instance, &physicalDeviceCount, physicalDevices));

  *physicalDevice = pick_physical_device(physicalDevices, physicalDeviceCount);
  if (*physicalDevice == 0) {
    log_message(10, "Could not pick a physical device!");
    cleanup(10);
  }

  *familyIndex = 0;
  *device = create_device(*instance, *physicalDevice, familyIndex);
  assert_log(*device, "Could not create a Vulkan device!");
}

VkCommandPool create_command_pool(VkDevice *__restrict device, uint32_t *__restrict familyIndex) {
  VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  createInfo.queueFamilyIndex = *familyIndex;

  VkCommandPool commandPool = 0;
  VK_CHECK(vkCreateCommandPool(*device, &createInfo, 0, &commandPool));

  return commandPool;
}

__inline__ static void set_signals() {
  signal(SIGHUP, sig_handler);
  signal(SIGINT, sig_handler);
  signal(SIGQUIT, sig_handler);
  signal(SIGILL, sig_handler);
  signal(SIGKILL, sig_handler);
  signal(SIGABRT, sig_handler);
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  set_signals();

  GLFWwindow *__restrict window;

  if (!glfwInit()) {
    log_message(10, "Could not initialize glfw\tAborting!");
    cleanup(10);
  }

  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  uint32_t familyIndex;

  init_vulkan(&instance, &physicalDevice, &device, &familyIndex);
  window = init_window();

  VkSurfaceKHR surface = create_surface(&instance, window);
  assert_log(surface, "Could not create a surface!");
  {
    uint32_t pSupported;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &pSupported));
    assert_log(surface, "No WSI support on the created surface!");
  }

  VkSwapchainKHR swapchain = create_swapchain(&device, &surface, &physicalDevice, &familyIndex, windowWidth, windowHeight);
  assert_log(swapchain, "Could not create a swapchain!");

  VkSemaphore acquireSemaphore = create_semaphore(&device);
  assert_log(acquireSemaphore, "Could not create the acquire semaphore!");

  VkSemaphore releaseSemaphore = create_semaphore(&device);
  assert_log(releaseSemaphore, "Could not create the release semaphore!");

  VkQueue queue = 0;
  vkGetDeviceQueue(device, familyIndex, 0, &queue);

  VkImage swapchainImages[16];
  uint32_t swapchainImageCount = sizeof(swapchainImages) / sizeof(swapchainImages[0]);
  VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages));

  VkCommandPool commandPool = create_command_pool(&device, &familyIndex);
  assert_log(commandPool, "Could not create the command pool!");

  VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  allocateInfo.commandPool = commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;
  
  VkCommandBuffer commandBuffer = 0;
  VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

  while (!glfwWindowShouldClose(window)) {
    log_message(0, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n\n\n\n\n");
    glfwPollEvents();
    
    uint32_t imageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, ~0ULL, acquireSemaphore, VK_NULL_HANDLE, &imageIndex));

    VK_CHECK(vkResetCommandPool(device, commandPool, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkClearColorValue color = { 1, 0, 1, 1 };
    
    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.levelCount = 1;
    range.layerCount = 1;

    vkCmdClearColorImage(commandBuffer, swapchainImages[imageIndex], VK_IMAGE_LAYOUT_GENERAL, &color, 1, &range);

    VK_CHECK(vkEndCommandBuffer(commandBuffer));

    VkPipelineStageFlags submitStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    submitInfo.pWaitDstStageMask = &submitStageMask;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &releaseSemaphore;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

    VK_CHECK(vkDeviceWaitIdle(device));
  }

  cleanup(0);
  /** Unreachable **/
  return 0;
}

GLFWwindow *init_window() {
  GLFWwindow *window;
  glfwSetErrorCallback(callback_error);

  window = glfwCreateWindow(1080, 1920, "Suijin", NULL, NULL);
  glfwGetWindowSize(window, &windowWidth, &windowHeight);

  if (window == NULL) {
    log_message(10, "Could not create the window!\tTerminating!");
    cleanup(10);
  }

  glfwMakeContextCurrent(window);

  glfwSetFramebufferSizeCallback(window, callback_window_resize);
  /* glfwSetWindowCloseCallback(window, callback_window_should_close); */

  return window;
}

void callback_error(int error, const char *desc) {
  log_format(0, "GLFW error %i:[%s]", error, desc);
}

void callback_window_resize(GLFWwindow *window, int width, int height) {
  /* glViewport(0, 0, width, height); */
  log_format(0, "Window resized to: %ix%i", width, height);
  windowWidth = width;
  windowHeight = height;
}

