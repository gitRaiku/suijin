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

struct Swapchain {
  VkSwapchainKHR swapchain;

  uint32_t imageCount;
  VkImage       *__restrict images;
  VkImageView   *__restrict imageViews;
  VkFramebuffer *__restrict framebuffers;

  uint32_t width;
  uint32_t height;
};

void cleanup(int32_t severity) {
  log_format(severity, "Exiting with exit code %i", severity);

  glfwTerminate();

  exit(severity);
}

GLFWwindow *init_window();
void callback_error(int error, const char *desc);
void callback_window_resize(GLFWwindow *__restrict window, int width, int height);

VkSurfaceKHR create_surface(VkInstance instance,
                            GLFWwindow *__restrict window) {
  VkXlibSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR };
  createInfo.pNext = NULL;
  createInfo.dpy = glfwGetX11Display();
  createInfo.window = glfwGetX11Window(window);

  VkSurfaceKHR surface = 0;
  VK_CHECK(vkCreateXlibSurfaceKHR(instance, &createInfo, 0, &surface));
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

VkDevice create_device(VkPhysicalDevice physicalDevice, 
                       uint32_t         familyIndex) {
    float queuePriorities[] = { 1.0f };
    
    VkDeviceQueueCreateInfo queueInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO };
    queueInfo.queueFamilyIndex = familyIndex;
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
    VK_CHECK(vkCreateDevice(physicalDevice, &createInfo, 0, &device));
    return device;
}

VkImageView create_image_view(VkDevice            device,
                              VkImage             image,
                              VkSurfaceFormatKHR  surfaceFormat) {
  VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
  createInfo.image = image;
  createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  createInfo.format = surfaceFormat.format;
  createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  createInfo.subresourceRange.levelCount = 1;
  createInfo.subresourceRange.layerCount = 1;

  VkImageView view = 0;
  VK_CHECK(vkCreateImageView(device, &createInfo, 0, &view));

  return view;
}

VkFramebuffer create_framebuffer(VkDevice      device,
                                 VkRenderPass  renderPass,
                                 VkImageView   imageView,
                                 uint32_t      width,
                                 uint32_t      height) {
  VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
  createInfo.renderPass = renderPass;
  createInfo.attachmentCount = 1;
  createInfo.pAttachments = &imageView;
  createInfo.width = width;
  createInfo.height = height;
  createInfo.layers = 1;

  VkFramebuffer framebuffer = 0;
  VK_CHECK(vkCreateFramebuffer(device, &createInfo, 0, &framebuffer));

  return framebuffer;
}

VkSurfaceFormatKHR get_surface_format(VkPhysicalDevice physicalDevice,
                                      VkSurfaceKHR     surface) {
  uint32_t formatCount;
  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL));
  assert_log(formatCount > 0, "Could not get format count");

  VkSurfaceFormatKHR *__restrict formats = calloc(formatCount, sizeof(VkSurfaceFormatKHR));
  VkSurfaceFormatKHR res;

  VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats));

  if (formatCount == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
    res.format = VK_FORMAT_R8G8B8A8_UNORM;
  } else {
    int32_t i;
    for(i = 0; i < formatCount; ++i) {
      if (formats[i].format == VK_FORMAT_R8G8B8A8_UNORM || 
          formats[i].format == VK_FORMAT_B8G8R8A8_UNORM) {
        res = formats[i];
        break;
      }
    }
  }

  free(formats);
  return res;
}

VkSwapchainKHR create_swapchain(VkDevice                        device,
                                VkSurfaceKHR                    surface,
                                VkSurfaceFormatKHR  *__restrict surfaceFormat,
                                VkPhysicalDevice                physicalDevice,
                                uint32_t                        familyIndex,
                                uint32_t                        width,
                                uint32_t                        height,
                                VkSwapchainKHR                  oldSwapchain) {
    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices = &familyIndex;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkSurfaceCapabilitiesKHR surfaceCapabilites;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilites);
    createInfo.minImageCount = surfaceCapabilites.minImageCount;
    createInfo.imageExtent.width = surfaceCapabilites.currentExtent.width;
    createInfo.imageExtent.height = surfaceCapabilites.currentExtent.height;
    createInfo.preTransform = surfaceCapabilites.currentTransform;
    createInfo.compositeAlpha = surfaceCapabilites.supportedCompositeAlpha;
    createInfo.oldSwapchain = oldSwapchain;

    *surfaceFormat = get_surface_format(physicalDevice, surface);
    createInfo.imageFormat = surfaceFormat->format;
    createInfo.imageColorSpace = surfaceFormat->colorSpace;

    VkSwapchainKHR swapchain = 0;
    VK_CHECK(vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain));

    return swapchain;
}

void init_swapchain(struct Swapchain *__restrict resSwapchain,
                   VkDevice                      device,
                   VkSurfaceKHR                  surface,
                   VkPhysicalDevice              physicalDevice,
                   uint32_t                      familyIndex,
                   uint32_t                      width,
                   uint32_t                      height,
                   VkRenderPass                  renderPass,
                   VkSwapchainKHR                oldSwapchain) {
  VkSurfaceCapabilitiesKHR surfaceCapabilities;
  VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

  VkSurfaceFormatKHR surfaceFormat;
  VkSwapchainKHR swapchain = create_swapchain(device, surface, &surfaceFormat, physicalDevice, familyIndex, width, height, oldSwapchain);

  uint32_t imageCount = 0;
  VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL));
  assert_log(imageCount > 0, "Could not get swapchain images");

  VkImage *__restrict images = calloc(imageCount, sizeof(VkImage));
  VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, images));

  VkImageView   *__restrict imageViews = calloc(imageCount, sizeof(VkImageView));
  VkFramebuffer *__restrict framebuffers = calloc(imageCount, sizeof(VkFramebuffer));

  {
    int32_t i;
    for(i = 0; i < imageCount; ++i) {
      imageViews[i] = create_image_view(device, images[i], surfaceFormat);
      assert_log(imageViews[i], "Could not create imageViews");
    }
    for(i = 0; i < imageCount; ++i) {
      framebuffers[i] = create_framebuffer(device, renderPass, imageViews[i], width, height);
      assert_log(framebuffers[i], "Could not create swapchainFramebuffers");
    }
  }

  resSwapchain->swapchain = swapchain;

  resSwapchain->images = images;
  resSwapchain->imageViews = imageViews;
  resSwapchain->framebuffers = framebuffers;

  resSwapchain->width = width;
  resSwapchain->height = height;
  resSwapchain->imageCount = imageCount;
}

void destroy_swapchain(struct Swapchain *__restrict swapchain,
                      VkDevice device) {
  {
    int32_t i;
    for(i = 0; i < swapchain->imageCount; ++i) {
      vkDestroyFramebuffer(device, swapchain->framebuffers[i], 0);
      vkDestroyImageView(device, swapchain->imageViews[i], 0);
    }
  }

  free(swapchain->images);
  free(swapchain->imageViews);
  free(swapchain->framebuffers);

  vkDestroySwapchainKHR(device, swapchain->swapchain, 0);
}
VkSemaphore create_semaphore(VkDevice device) {
  VkSemaphoreCreateInfo createInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

  VkSemaphore semaphore = 0;
  VK_CHECK(vkCreateSemaphore(device, &createInfo, 0, &semaphore));

  return semaphore;
}

uint8_t supports_presentation(VkPhysicalDevice physicalDevice,
                              uint32_t familyIndex) {
  return 1;
  /** TODO: Doesn't work on windows, also just look at it... **/
}

uint32_t get_graphics_family_index(VkPhysicalDevice physicalDevice) {
  uint32_t queueCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
  assert_log(queueCount > 0, "Could not get family queue count");

  VkQueueFamilyProperties *__restrict queues = calloc(queueCount, sizeof(VkQueueFamilyProperties));
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queues);
  
  int32_t i;
  for(i = 0; i < queueCount; ++i) {
    if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      return i;
    }
  }
  return VK_QUEUE_FAMILY_IGNORED;
}

VkPhysicalDevice pick_physical_device(VkPhysicalDevice  *__restrict physicalDevices,
                                      uint32_t                      physicalDeviceCount) {
  VkPhysicalDevice discrete = 0;
  VkPhysicalDevice fallback = 0;
  VkPhysicalDeviceProperties props;
  {
    int32_t i;
    uint32_t familyIndex;
    for(i = 0; i < physicalDeviceCount; ++i) {
      vkGetPhysicalDeviceProperties(physicalDevices[i], &props);
      log_format(0, "GPU%u: %s", i, props.deviceName);

      familyIndex = get_graphics_family_index(physicalDevices[i]);
      if (familyIndex == VK_QUEUE_FAMILY_IGNORED) {
        continue;
      }

      if (!supports_presentation(physicalDevices[i], familyIndex)) {
        continue;
      }

      if (!discrete && props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        discrete = physicalDevices[i];
      }

      if (!fallback) {
        fallback = physicalDevices[i];
      }
    }
  }

  VkPhysicalDevice res = discrete ? discrete : fallback;
  if (res == 0) {
    log_message(10, "No physical devices available");
    return 0;
  }

  vkGetPhysicalDeviceProperties(res, &props);
  log_format(0, "Picking GPU %s", props.deviceName);

  return res;
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
  assert_log(*physicalDevice, "Could not pick a physical device");

  *familyIndex = 0;
  *device = create_device(*physicalDevice, *familyIndex);
  assert_log(*device, "Could not create a Vulkan device");
}

VkCommandPool create_command_pool(VkDevice  device, 
                                 uint32_t   familyIndex) {
  VkCommandPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
  createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
  createInfo.queueFamilyIndex = familyIndex;

  VkCommandPool commandPool = 0;
  VK_CHECK(vkCreateCommandPool(device, &createInfo, 0, &commandPool));

  return commandPool;
}

VkRenderPass create_render_pass(VkDevice            device,
                                VkSurfaceFormatKHR  surfaceFormat) {
  VkAttachmentDescription attachments[1] = {};
  attachments[0].format = surfaceFormat.format;
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
  VK_CHECK(vkCreateRenderPass(device, &createInfo, 0, &renderPass));

  return renderPass;
}

VkShaderModule load_shader(VkDevice   device,
                          const char *path) {
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

  assert_log(fileStat.st_size, "Could not get the size of the file");

  char *buffer = malloc(sizeof(char) * fileStat.st_size);
  assert_log(buffer, "Could not create the file buffer");
  
  ssize_t bread = read(fd, buffer, fileStat.st_size);
  if (bread == -1) {
    log_format(10, "Read failed %u %s %s %s", fd, path, strerrorname_np(errno), strerror(errno));
    cleanup(10);
  }

  VkShaderModuleCreateInfo createInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
  createInfo.codeSize = fileStat.st_size;
  createInfo.pCode = (const uint32_t *)buffer;

  VkShaderModule shaderModule = 0;
  VK_CHECK(vkCreateShaderModule(device, &createInfo, 0, &shaderModule));
  
  free(buffer);

  return shaderModule;
}

VkPipelineLayout create_pipeline_layout(VkDevice device) {
  VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

  VkPipelineLayout layout = 0;
  VK_CHECK(vkCreatePipelineLayout(device, &createInfo, 0, &layout));

  return layout;
}

VkPipeline create_graphics_pipeline(VkDevice          device, 
                                    VkPipelineCache   pipelineCache,
                                    VkRenderPass      renderPass,
                                    VkShaderModule    vert,
                                    VkShaderModule    frag,
                                    VkPipelineLayout  layout) {
  VkGraphicsPipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };

  VkPipelineShaderStageCreateInfo stages[2] = {};

  stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
	stages[0].module = vert;
	stages[0].pName  = "main";

	stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
	stages[1].module = frag;
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

  createInfo.layout = layout;
  createInfo.renderPass = renderPass;

  VkPipeline pipeline = 0;
  VK_CHECK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &createInfo, 0, &pipeline));

  return pipeline;
  
}

VkImageMemoryBarrier image_barrier(VkImage       image,
                                   VkAccessFlags srcAccessMask,
                                   VkAccessFlags dstAccessMask,
                                   VkImageLayout oldLayout,
                                   VkImageLayout newLayout) {
  VkImageMemoryBarrier res = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };

  res.srcAccessMask = srcAccessMask;
  res.dstAccessMask = dstAccessMask;
  res.oldLayout = oldLayout;
  res.newLayout = newLayout;
  res.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  res.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  res.image = image;
  res.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  res.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
  res.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

  return res;
}

VkBool32 debug_report_callback(VkDebugReportFlagsEXT      flags,
                               VkDebugReportObjectTypeEXT objectType,
                               uint64_t                   object,
                               size_t                     location,
                               int32_t                    messageCode,
                               const char*                pLayerPrefix,
                               const char*                pMessage,
                               void*                      pUserData) {
  const char* type = (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ? "ERROR"
    : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)) ? "WARNING"
      : "INFO";
  uint32_t severity = (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) ? 10
    : (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)) ? 4
      : 0;

  log_format(severity, "%s: %s", type, pMessage);

  assert_log(severity != 10, "Validation error encountered");

  return VK_FALSE;
}

VkDebugReportCallbackEXT register_debug_callback(VkInstance instance) {
  VkDebugReportCallbackCreateInfoEXT createInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT };

  createInfo.flags = VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
  createInfo.pfnCallback = debug_report_callback;
  
  PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
  
  VkDebugReportCallbackEXT callback = 0;
  VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &createInfo, 0, &callback));
  
  return callback;
}

void sig_handler(int signum) {
  log_format(signum, "Caught signal %u %s (%s)", signum, sigabbrev_np(signum), sigdescr_np(signum));
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
  assert_log(glfwInit(), "Could not initialize glfw");
  /* glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); */
  /* This apparently fixes a bug that may or may not affect anyone and may have been fixed */
  /* I've 0 idea what that does but it creates an error so i'm not including it */

  VkInstance instance;
  VkDevice device;
  VkPhysicalDevice physicalDevice;
  uint32_t familyIndex;

  init_vulkan(&instance, &physicalDevice, &device, &familyIndex);
  window = init_window();

  VkDebugReportCallbackEXT debugCallback = register_debug_callback(instance);

  VkSurfaceKHR surface = create_surface(instance, window);
  assert_log(surface, "Could not create a surface");

  {
    uint32_t pSupported;
    VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &pSupported));
    assert_log(pSupported, "No WSI support on the created surface");
  }

  VkSemaphore acquireSemaphore = create_semaphore(device);
  assert_log(acquireSemaphore, "Could not create the acquire semaphore");

  VkSemaphore releaseSemaphore = create_semaphore(device);
  assert_log(releaseSemaphore, "Could not create the release semaphore");

  VkQueue queue = 0;
  vkGetDeviceQueue(device, familyIndex, 0, &queue);

  VkSurfaceFormatKHR surfaceFormat = get_surface_format(physicalDevice, surface);
  VkRenderPass renderPass = create_render_pass(device, surfaceFormat);

  struct Swapchain swapchain;
  {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));
    init_swapchain(&swapchain, device, surface, physicalDevice, familyIndex, surfaceCapabilities.currentExtent.width, surfaceCapabilities.currentExtent.height, renderPass, 0);
    assert_log(swapchain.swapchain, "Could not create a swapchain");
  }

  VkShaderModule triangleVert = load_shader(device, "shaders/triangle.vert.spv");
  VkShaderModule triangleFrag = load_shader(device, "shaders/triangle.frag.spv");

  VkPipelineCache pipelineCache = 0; /// TODO: Add pipeline cache

  VkPipelineLayout triangleLayout = create_pipeline_layout(device);
  assert_log(triangleLayout, "Could not create a pipeline layout");

  VkPipeline trianglePipeline = create_graphics_pipeline(device, pipelineCache, renderPass, triangleVert, triangleFrag, triangleLayout);
  assert_log(trianglePipeline, "Could not create a pipeline. ~~Fuck~~");

  VkCommandPool commandPool = create_command_pool(device, familyIndex);
  assert_log(commandPool, "Could not create the command pool");

  VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
  allocateInfo.commandPool = commandPool;
  allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocateInfo.commandBufferCount = 1;
  
  VkCommandBuffer commandBuffer = 0;
  VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer));

  while (!shouldClose) {
    glfwPollEvents();

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCapabilities));

    uint32_t newWidth = surfaceCapabilities.currentExtent.width;
    uint32_t newHeight = surfaceCapabilities.currentExtent.height;

    if (swapchain.height != newHeight || swapchain.width != newWidth) {
      struct Swapchain oldSwapchain = swapchain;
      init_swapchain(&swapchain, device, surface, physicalDevice, familyIndex, newWidth, newHeight, renderPass, oldSwapchain.swapchain);

      VK_CHECK(vkDeviceWaitIdle(device));

      destroy_swapchain(&oldSwapchain, device);
    }
    
    uint32_t imageIndex = 0;
    VK_CHECK(vkAcquireNextImageKHR(device, swapchain.swapchain, ~0ULL, acquireSemaphore, VK_NULL_HANDLE, &imageIndex));

    VK_CHECK(vkResetCommandPool(device, commandPool, 0));

    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkImageMemoryBarrier renderBeginBarrier = image_barrier(swapchain.images[imageIndex], 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderBeginBarrier);

    VkClearColorValue color = { B2S(48), B2S(10), B2S(36), B2S(255) };
    VkClearValue clearColor = { color };

    VkRenderPassBeginInfo passBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    passBeginInfo.renderPass = renderPass;
    passBeginInfo.framebuffer = swapchain.framebuffers[imageIndex];
    passBeginInfo.renderArea.extent.width = swapchain.width;
    passBeginInfo.renderArea.extent.height = swapchain.height;
    passBeginInfo.clearValueCount = 1;
    passBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = { 0, (float)swapchain.height, (float)swapchain.width, -(float)swapchain.height, 0, 1 };
    VkRect2D scissor = { {0, 0}, {swapchain.width, swapchain.height} };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, trianglePipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    
    VkImageMemoryBarrier renderEndBarrier = image_barrier(swapchain.images[imageIndex], VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, 0, 0, 0, 1, &renderEndBarrier);

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

    VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));

    VK_CHECK(vkDeviceWaitIdle(device));
    shouldClose |= glfwWindowShouldClose(window);

    glfwWaitEvents();
  }

  vkDestroyCommandPool(device, commandPool, 0);

  vkDestroyPipeline(device, trianglePipeline, 0);
  vkDestroyPipelineLayout(device, triangleLayout, 0);

  vkDestroyShaderModule(device, triangleFrag, 0);
  vkDestroyShaderModule(device, triangleVert, 0);

  vkDestroyRenderPass(device, renderPass, 0);

  vkDestroySemaphore(device, releaseSemaphore, 0);
  vkDestroySemaphore(device, acquireSemaphore, 0);
  
  destroy_swapchain(&swapchain, device);
  vkDestroySurfaceKHR(instance, surface, 0);

  glfwDestroyWindow(window);

  vkDestroyDevice(device, 0);

  PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
  vkDestroyDebugReportCallbackEXT(instance, debugCallback, 0);

  vkDestroyInstance(instance, 0);

  cleanup(0);
  /** Unreachable **/
  return 0;
}

GLFWwindow *init_window() {
  GLFWwindow *window;
  glfwSetErrorCallback(callback_error);

  window = glfwCreateWindow(1080, 1920, "Suijin", NULL, NULL);

  assert_log(window, "Could not create the window");

  glfwMakeContextCurrent(window);

  /* glfwSetFramebufferSizeCallback(window, callback_window_resize); */
  /* glfwSetWindowCloseCallback(window, callback_window_should_close); */

  return window;
}

void callback_error(int error, const char *desc) {
  log_format(0, "GLFW error %i:[%s]", error, desc);
}

