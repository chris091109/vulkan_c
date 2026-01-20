#include "vulkan_init.h"
#include "color.h"
#include "platform.h"
#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vulkan.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

const char *validationLayers[] = {
  "VK_LAYER_KHRONOS_validation"
};

const char *deviceExtensions[] = {
  VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

bool checkValidationLayerSupport(void) {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, NULL);

  VkLayerProperties *availableLayers = malloc(layerCount * sizeof(VkLayerProperties));
  if (!availableLayers) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for layers\n");
    return false;
  }
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
  bool layerFound = false;
  for (uint32_t i = 0; i < layerCount; i++) {
    for (size_t j = 0; j < sizeof(validationLayers) / sizeof(validationLayers[0]); j++) {
      if (strcmp(validationLayers[j], availableLayers[i].layerName) == 0) {
        layerFound = true;
        break;
      }
    }
  }

  free(availableLayers);
  return layerFound;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface) {
  QueueFamilyIndices indices = {.graphicsFamily = UINT32_MAX, .presentFamily = UINT32_MAX};
  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, NULL);

  VkQueueFamilyProperties *queueFamilies = malloc(queueFamilyCount * sizeof(VkQueueFamilyProperties));
  if (!queueFamilies) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for queueFamilies\n");
    return indices;
  }
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies);
  
  for (uint32_t i = 0; i < queueFamilyCount; i++) {
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }
    
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }
    
    if (indices.graphicsFamily != UINT32_MAX && indices.presentFamily != UINT32_MAX) {
      break;
    }
  }
  
  free(queueFamilies);
  return indices;
}

bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface) {
  VkPhysicalDeviceProperties deviceProperties;
  VkPhysicalDeviceFeatures deviceFeatures;
  vkGetPhysicalDeviceProperties(device, &deviceProperties);
  vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

  QueueFamilyIndices indices = findQueueFamilies(device, surface);

  return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
    deviceFeatures.geometryShader &&
    indices.graphicsFamily != UINT32_MAX &&
    indices.presentFamily != UINT32_MAX;
}

bool vulkan_create(Vulkan_Context *ctx, Platform_Context *platform) {
  // Vulkan instance 
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    printf(RED "[ERROR] " RESET "validation layers requested, but not available!\n");
    return false;
  }
  
  VkApplicationInfo appInfo = {0};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "App";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  
  // Get GLFW required extensions
  uint32_t glfwExtensionCount = 0;
  const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  
  // GLFW extensions already include VK_KHR_surface and platform-specific surface extensions
  createInfo.enabledExtensionCount = glfwExtensionCount;
  createInfo.ppEnabledExtensionNames = glfwExtensions;
  
  if (enableValidationLayers) {
    createInfo.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
    createInfo.ppEnabledLayerNames = validationLayers;
  } else {
    createInfo.enabledLayerCount = 0;
  }
  
  if (vkCreateInstance(&createInfo, NULL, &ctx->instance) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create instance!\n");
    return false;
  }
  
  uint32_t extensionCount = 0;
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
  VkExtensionProperties *extensions = malloc(extensionCount * sizeof(VkExtensionProperties));
  if (!extensions) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for extensions\n");
    return false;
  }
  vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
  printf("available extensions:\n");

  for (uint32_t i = 0; i < extensionCount; i++) {
    printf("\t%s\n", extensions[i].extensionName);
  }
  free(extensions);
  printf(GREEN "[OK] " RESET "Instance\n");
  
  // Surface
  if (platform == NULL) {
    printf(RED "[ERROR] " RESET "platform context is NULL\n");
    return false;
  }
  
  if (glfwCreateWindowSurface(ctx->instance, platform->window, NULL, &ctx->surface) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create window surface\n");
    return false;
  }

  printf(GREEN "[OK] " RESET "Surface\n");

  // Vulkan physical device 
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, NULL);
  if (deviceCount == 0) {
    printf(RED "[ERROR] " RESET "failed to find GPUs with Vulkan support\n");
    return false;
  }

  VkPhysicalDevice *devices = malloc(deviceCount * sizeof(VkPhysicalDevice));
  if (!devices) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for devices\n");
    return false;
  }

  vkEnumeratePhysicalDevices(ctx->instance, &deviceCount, devices);

  // Select a suitable physical device
  for (uint32_t i = 0; i < deviceCount; i++) {
    if (isDeviceSuitable(devices[i], ctx->surface)) {
      physicalDevice = devices[i];
      break;
    }
  }

  free(devices);

  if (physicalDevice == VK_NULL_HANDLE) {
    printf(RED "[ERROR] " RESET "failed to find a suitable GPU\n");
    return false;
  }

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
  printf("Selected GPU: %s\n", deviceProperties.deviceName);
  ctx->physicalDevice = physicalDevice;
  printf(GREEN "[OK] " RESET "Device\n");

  // Vulkan logical device and queues
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice, ctx->surface);

  // Create queue create infos for unique queue families
  VkDeviceQueueCreateInfo queueCreateInfos[2];
  uint32_t queueCreateInfoCount = 0;
  uint32_t uniqueQueueFamilies[2];
  uint32_t uniqueCount = 0;
  
  uniqueQueueFamilies[uniqueCount++] = indices.graphicsFamily;
  if (indices.presentFamily != indices.graphicsFamily) {
    uniqueQueueFamilies[uniqueCount++] = indices.presentFamily;
  }
  
  float queuePriority = 1.0f;
  for (uint32_t i = 0; i < uniqueCount; i++) {
    VkDeviceQueueCreateInfo queueCreateInfo = {0};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = uniqueQueueFamilies[i];
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos[queueCreateInfoCount++] = queueCreateInfo;
  }

  VkPhysicalDeviceFeatures requestedFeatures = {0};
  VkDeviceCreateInfo createInfo2 = {0};
  createInfo2.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo2.pQueueCreateInfos = queueCreateInfos;
  createInfo2.queueCreateInfoCount = queueCreateInfoCount;

  createInfo2.pEnabledFeatures = &requestedFeatures;
  createInfo2.enabledExtensionCount = sizeof(deviceExtensions) / sizeof(deviceExtensions[0]);
  createInfo2.ppEnabledExtensionNames = deviceExtensions;

  if (enableValidationLayers) {
    createInfo2.enabledLayerCount = sizeof(validationLayers) / sizeof(validationLayers[0]);
    createInfo2.ppEnabledLayerNames = validationLayers;
  } else {
    createInfo2.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo2, NULL, &ctx->device) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create logical device\n");
    return false;
  }

  vkGetDeviceQueue(ctx->device, indices.graphicsFamily, 0, &ctx->queue);
  vkGetDeviceQueue(ctx->device, indices.presentFamily, 0, &ctx->presentQueue);
  
  printf(GREEN "[OK] " RESET "Queue\n");
  printf(GREEN "[OK] " RESET "Vulkan Init\n");
  return true;
}

void vulkan_destroy(Vulkan_Context *ctx) {
  if (!ctx) return;
  vkDestroyDevice(ctx->device, NULL);
  vkDestroySurfaceKHR(ctx->instance, ctx->surface, NULL);
  vkDestroyInstance(ctx->instance, NULL);
}
