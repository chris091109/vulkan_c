#ifndef RENDERING_H
#define RENDERING_H

#include "vulkan_init.h"
#include <stdbool.h>
#include <vulkan/vulkan.h>
#include "platform.h"

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct Rendering_Context Rendering_Context;
struct Rendering_Context {
  Vulkan_Context vulkan_context;
  VkSwapchainKHR swapChain;
  VkImage *swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  VkImageView *swapChainImageViews;
  uint32_t swapChainImageCount;

  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;

  VkPipelineLayout pipelineLayout;
  VkRenderPass renderPass;
  VkPipeline graphicsPipeline;

  VkFramebuffer *swapChainFramebuffers;
  VkCommandPool commandPool;
  VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

  // Synchronization objects
  VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];  // Per-frame
  VkSemaphore *renderFinishedSemaphores;  // Per-swapchain image (dynamic array)
  VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];  // Per-frame
  VkFence *imagesInFlight;  // Tracks which fence is using each image (dynamic array)

  uint32_t currentFrame;
};

bool rendering_create(Rendering_Context *ctx, Vulkan_Context *vulkan_context, Platform_Context *platform);
void rendering_draw(Rendering_Context *ctx);
void rendering_destroy(Rendering_Context *ctx);

#endif
