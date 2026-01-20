#ifndef VULKAN_INIT_H
#define VULKAN_INIT_H

#include <stdbool.h>
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include "color.h"
#include "platform.h"

typedef struct {
  uint32_t graphicsFamily;
  uint32_t presentFamily;
} QueueFamilyIndices;

extern const char *validationLayers[];
extern const char *deviceExtensions[];

typedef struct Vulkan_Context Vulkan_Context;
struct Vulkan_Context {
  VkInstance instance;
  VkPhysicalDevice physicalDevice;
  VkDevice device;
  VkQueue queue;
  VkQueue presentQueue;
  VkSurfaceKHR surface;
  VkDebugUtilsMessengerEXT debugMessenger;
};

bool vulkan_create(Vulkan_Context *ctx, Platform_Context *platform);
void vulkan_destroy(Vulkan_Context *ctx);

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);

#endif
