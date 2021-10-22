#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "fujin.h"      /**  Logging  **/

#define VK_CHECK(call) {VkResult _result = call;assert(_result == VK_SUCCESS);}
#define assert_log(value, message) {if (!(value)) {log_message(10, message); cleanup(10);};}
#define LENGTH(vec) (sizeof(vec)/sizeof(vec[0]))
#define B2S(number) ((number)/255.0f)

uint32_t shouldClose = 0;

int32_t windowWidth = 0, windowHeight = 0;

void cleanup(int32_t severity) {
  log_format(severity, "Exiting with exit code %i", severity);

  glfwTerminate();

  exit(severity);
}

GLFWwindow *init_window();
void callback_error(int error, const char *desc);
void callback_window_resize(GLFWwindow *__restrict window, int width, int height);

VkSurfaceKHR create_surface(VkInstance *__restrict instance,
                            GLFWwindow *__restrict window) {
  VkXlibSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
  createInfo.pNext = NULL;
  createInfo.dpy = glfwGetX11Display();
  createInfo.window = glfwGetX11Window(window);

  VkSurfaceKHR surface = 0;
  VK_CHECK(vkCreateXlibSurfaceKHR(*instance, &createInfo, 0, &surface));
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
  createInfo.enabledLayerCount = LENGTH(debugLayers);
#endif

  const char *extensions[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#ifdef _DEBUG
		  VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
#endif
  };

  createInfo.ppEnabledExtensionNames = extensions;
  createInfo.enabledExtensionCount = LENGTH(extensions);

  VkInstance instance = 0;
  VK_CHECK(vkCreateInstance(&createInfo, 0, &instance));

  return instance;
}

VkDevice create_device(VkPhysicalDevice  *__restrict physicalDevice, 
                       uint32_t          *__restrict familyIndex) {
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
    createInfo.enabledExtensionCount = LENGTH(extensions);

    VkDevice device = 0;
    VK_CHECK(vkCreateDevice(*physicalDevice, &createInfo, 0, &device));
    return device;
}

VkSurfaceFormatKHR get_surface_format(VkPhysicalDevice *__restrict physicalDevice,
                                      VkSurfaceKHR     *__restrict surface) {
  VkSurfaceFormatKHR formats[16];
  uint32_t formatCount = LENGTH(formats);
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(*physicalDevice, *surface, &formatCount, formats));
  assert_log(formatCount, "Format count error, FIXME");
  return formats[0];
}

VkSwapchainKHR create_swapchain(VkDevice            *__restrict device,
                                VkSurfaceKHR        *__restrict surface,
                                VkSurfaceFormatKHR  *__restrict surfaceFormat,
                                VkPhysicalDevice    *__restrict physicalDevice,
                                uint32_t            *__restrict familyIndex,
                                uint32_t                        width,
                                uint32_t                        height) {
    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = *surface;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = familyIndex;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkSurfaceCapabilitiesKHR surfaceCapabilites;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*physicalDevice, *surface, &surfaceCapabilites);
    createInfo.minImageCount = surfaceCapabilites.minImageCount;
    createInfo.imageExtent.width = surfaceCapabilites.currentExtent.width;
    createInfo.imageExtent.width = surfaceCapabilites.currentExtent.width;
    createInfo.preTransform = surfaceCapabilites.currentTransform;
    createInfo.compositeAlpha = surfaceCapabilites.supportedCompositeAlpha;

    *surfaceFormat = get_surface_format(physicalDevice, surface);
    createInfo.imageFormat = surfaceFormat->format;
    createInfo.imageColorSpace = surfaceFormat->colorSpace;

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

VkPhysicalDevice pick_physical_device(VkPhysicalDevice  *__restrict physicalDevices,
                                      uint32_t                      physicalDeviceCount) {
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

void init_vulkan(VkInstance        *__restrict instance, 
                 VkPhysicalDevice  *__restrict physicalDevice, 
                 VkDevice          *__restrict device, 
                 uint32_t          *__restrict familyIndex) {
  *instance = create_instance();
  assert_log(*instance, "Could not create a Vulkan instance! Please check that Vulkan is enabled on your system.");

  VkPhysicalDevice physicalDevices[16];
  uint32_t physicalDeviceCount = LENGTH(physicalDevices);
  VK_CHECK(vkEnumeratePhysicalDevices(*instance, &physicalDeviceCount, physicalDevices));

  *physicalDevice = pick_physical_device(physicalDevices, physicalDeviceCount);
  assert_log(*physicalDevice, "Could not pick a physical device!");

  *familyIndex = 0;
  *device = create_device(physicalDevice, familyIndex);
  assert_log(*device, "Could not create a Vulkan device!");
}

VkCommandPool create_command_pool(VkDevice  *__restrict device, 
                                 uint32_t   *__restrict familyIndex) {
  VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  createInfo.queueFamilyIndex = *familyIndex;

  VkCommandPool commandPool = 0;
  VK_CHECK(vkCreateCommandPool(*device, &createInfo, 0, &commandPool));

  return commandPool;
}

VkRenderPass create_render_pass(VkDevice            *__restrict device,
                                VkSurfaceFormatKHR  *__restrict surfaceFormat) {
  VkAttachmentDescription attachments[1] = {};
  attachments[0].format = surfaceFormat->format;
  attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachments = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpass = {};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachments;

  VkRenderPassCreateInfo createInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
  createInfo.attachmentCount = LENGTH(attachments);
  createInfo.pAttachments = attachments;
  createInfo.subpassCount = 1;
  createInfo.pSubpasses = &subpass;

  VkRenderPass renderPass = 0;
  VK_CHECK(vkCreateRenderPass(*device, &createInfo, 0, &renderPass));

  return renderPass;
}

VkShaderModule load_shader(VkDevice   *__restrict device,
                          const char *           path) {
  int32_t fd = open(path, O_RDONLY);
  if (fd < 0) {
    log_format(10, "Could not open the file at %s. Please create it.", path);
    cleanup(10);
  }

  struct stat fileStat;
  if (fstat(fd, &fileStat) == -1)  {
    log_format(10, "Stat failed %u %s %s", fd, path, strerror(errno));
    cleanup(10);
  }

  assert_log(fileStat.st_size, "Could not get the size of the file!");

  char *buffer = malloc(sizeof(char) * fileStat.st_size);
  assert_log(buffer, "Could not create the file buffer!");
  
  ssize_t bread = read(fd, buffer, fileStat.st_size);
  if (bread == -1) {
    log_format(10, "Read failed %u %s %s %s", fd, path, strerrorname_np(errno), strerror(errno));
    cleanup(10);
  }

  VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  createInfo.codeSize = fileStat.st_size;
  createInfo.pCode = (const uint32_t *)buffer;

  VkShaderModule shaderModule = 0;
  VK_CHECK(vkCreateShaderModule(*device, &createInfo, 0, &shaderModule));
  
  free(buffer);

  return shaderModule;
}

VkPipelineLayout create_pipeline_layout(VkDevice *__restrict device) {
  VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

  VkPipelineLayout layout = 0;
  VK_CHECK(vkCreatePipelineLayout(*device, &createInfo, 0, &layout));

  return layout;
}

VkPipeline create_graphics_pipeline(VkDevice          *__restrict device, 
                                    VkPipelineCache   *__restrict pipelineCache,
                                    VkRenderPass      *__restrict renderPass,
                                    VkShaderModule    *__restrict vert,
                                    VkShaderModule    *__restrict frag,
                                    VkPipelineLayout  *__restrict layout) {
  VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

  VkPipelineShaderStageCreateInfo stages[2] = {};

  stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = *vert;
	stages[0].pName  = "main";

	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = *frag;
	stages[1].pName  = "main";

  createInfo.stageCount = LENGTH(stages);
  createInfo.pStages = stages;

  VkPipelineVertexInputStateCreateInfo vertexInput = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  createInfo.pVertexInputState = &vertexInput;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  createInfo.pInputAssemblyState = &inputAssembly;

  VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;
  createInfo.pViewportState = &viewportState;

  VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  rasterizationState.lineWidth = 1.0f;
  createInfo.pRasterizationState = &rasterizationState;

  VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; /** MSAA here? **/
  createInfo.pMultisampleState = &multisampleState;

  VkPipelineDepthStencilStateCreateInfo depthStencilState = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  createInfo.pDepthStencilState = &depthStencilState;

  VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
  colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

  VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
  colorBlendState.attachmentCount = 1;
  colorBlendState.pAttachments = &colorBlendAttachmentState;
  createInfo.pColorBlendState = &colorBlendState;

  VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };


  VkPipelineDynamicStateCreateInfo dynamicState = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
  dynamicState.dynamicStateCount = LENGTH(dynamicStates);
  dynamicState.pDynamicStates = dynamicStates;
  createInfo.pDynamicState = &dynamicState;

  createInfo.layout = *layout;
  createInfo.renderPass = *renderPass;

  VkPipeline pipeline = 0;
  VK_CHECK(vkCreateGraphicsPipelines(*device, *pipelineCache, 1, &createInfo, 0, &pipeline));

  return pipeline;
  
}

VkImageView create_image_view(VkDevice            *__restrict device,
                              VkImage             *__restrict image,
                              VkSurfaceFormatKHR  *__restrict surfaceFormat) {
  VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
  createInfo.image = *image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = surfaceFormat->format;
  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.layerCount = 1;

  VkImageView view = 0;
  VK_CHECK(vkCreateImageView(*device, &createInfo, 0, &view));

  return view;
}

VkFramebuffer create_framebuffer(VkDevice      *__restrict device,
                                 VkRenderPass  *__restrict renderPass,
                                 VkImageView   *__restrict imageView,
                                 uint32_t                  width,
                                 uint32_t                  height) {
  VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
  createInfo.renderPass = *renderPass;
  createInfo.attachmentCount = 1;
  createInfo.pAttachments = imageView;
  createInfo.width = width;
  createInfo.height = height;
  createInfo.layers = 1;

  VkFramebuffer framebuffer = 0;
  VK_CHECK(vkCreateFramebuffer(*device, &createInfo, 0, &framebuffer));

  return framebuffer;
}

void sig_handler(int signum) {
  log_format(signum, "Caught signal %u %s (%s)!", signum, sigabbrev_np(signum), sigdescr_np(signum));
  shouldClose |= 0b10;
}

__inline__ static void set_signals() {
  signal(SIGQUIT, sig_handler);
  signal(SIGINT, sig_handler);
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
  set_signals();

  GLFWwindow *__restrict window;
  assert_log(glfwInit(), "Could not initialize glfw!");

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
    assert_log(pSupported, "No WSI support on the created surface!");
  }

  VkSurfaceFormatKHR surfaceFormat = {0, 0};
  VkSwapchainKHR swapchain = create_swapchain(&device, &surface, &surfaceFormat, &physicalDevice, &familyIndex, windowWidth, windowHeight);
  assert_log(swapchain, "Could not create a swapchain!");

  VkSemaphore acquireSemaphore = create_semaphore(&device);
  assert_log(acquireSemaphore, "Could not create the acquire semaphore!");

  VkSemaphore releaseSemaphore = create_semaphore(&device);
  assert_log(releaseSemaphore, "Could not create the release semaphore!");

  VkQueue queue = 0;
  vkGetDeviceQueue(device, familyIndex, 0, &queue);

  VkRenderPass renderPass = create_render_pass(&device, &surfaceFormat);

  VkShaderModule triangleVert = load_shader(&device, "shaders/triangle.vert.spv");
  VkShaderModule triangleFrag = load_shader(&device, "shaders/triangle.frag.spv");

  VkPipelineCache pipelineCache = 0; /// TODO: Add pipeline cache

  VkPipelineLayout triangleLayout = create_pipeline_layout(&device);
  assert_log(triangleLayout, "Could not create a pipeline layout");

  VkPipeline trianglePipeline = create_graphics_pipeline(&device, &pipelineCache, &renderPass, &triangleVert, &triangleFrag, &triangleLayout);
  assert_log(trianglePipeline, "Could not create a pipeline. ~~Fuck~~");

  VkImage swapchainImages[16];
  uint32_t swapchainImageCount = LENGTH(swapchainImages);
  VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages));

  VkImageView   swapchainImageViews  [16];
  VkFramebuffer swapchainFramebuffers[16];
  {
    int32_t i;
    for(i = 0; i < swapchainImageCount; ++i) {
      swapchainImageViews[i] = create_image_view(&device, swapchainImages+i, &surfaceFormat);
      assert_log(swapchainImageViews[i], "Could not create swapchainImageViews");
    }
    for(i = 0; i < swapchainImageCount; ++i) {
      swapchainFramebuffers[i] = create_framebuffer(&device, &renderPass, swapchainImageViews+i, windowWidth, windowHeight);
      assert_log(swapchainFramebuffers[i], "Could not create swapchainFramebuffers");
    }
  }

  VkCommandPool commandPool = create_command_pool(&device, &familyIndex);
  assert_log(commandPool, "Could not create the command pool!");

  VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  allocateInfo.commandPool = commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;
  
  VkCommandBuffer commandBuffer = 0;
  VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

  while (!shouldClose) {
    glfwPollEvents();
    
    uint32_t imageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain, ~0ULL, acquireSemaphore, VK_NULL_HANDLE, &imageIndex));

    VK_CHECK(vkResetCommandPool(device, commandPool, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkClearColorValue color = { B2S(48), B2S(10), B2S(36), B2S(255) };
    VkClearValue clearColor = { color };

    VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    passBeginInfo.renderPass = renderPass;
    passBeginInfo.framebuffer = swapchainFramebuffers[imageIndex];
    passBeginInfo.renderArea.extent.width = windowWidth;
    passBeginInfo.renderArea.extent.height = windowHeight;
    passBeginInfo.clearValueCount = 1;
    passBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0, (float)windowHeight, (float)windowWidth, -(float)windowHeight, 0, 1 };
    VkRect2D scissor = { {0, 0}, {windowWidth, windowHeight} };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    
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
    shouldClose |= glfwWindowShouldClose(window);
  }

  vkDestroyInstance(instance, 0);

  cleanup(0);
  /** Unreachable **/
  return 0;
}

GLFWwindow *init_window() {
  GLFWwindow *window;
  glfwSetErrorCallback(callback_error);

  window = glfwCreateWindow(1080, 1920, "Suijin", NULL, NULL);
  glfwGetWindowSize(window, &windowWidth, &windowHeight);

  assert_log(window, "Could not create the window!");

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

