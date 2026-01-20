#include "rendering.h"
#include "platform.h"
#include "vulkan_init.h"
#include "color.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <vulkan/vulkan_core.h>

static const char *vert_path = "external/shaders/shader.vert.spv";
static const char *frag_path = "external/shaders/shader.frag.spv";

typedef struct {
  VkSurfaceCapabilitiesKHR capabilities;
  VkSurfaceFormatKHR *formats;
  uint32_t formatCount;
  VkPresentModeKHR *presentModes;
  uint32_t presentModeCount;
} SwapChainSupportDetails;

static void recordCommandBuffer(Rendering_Context *ctx, VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // BEGIN COMMAND BUFFER (THIS WAS MISSING!)
    VkCommandBufferBeginInfo beginInfo = {0};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = NULL;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        printf(RED "[ERROR] " RESET "failed to begin recording command buffer!\n");
        return;
    }

    VkRenderPassBeginInfo renderPassInfo = {0};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = ctx->renderPass;
    renderPassInfo.framebuffer = ctx->swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset.x = 0;
    renderPassInfo.renderArea.offset.y = 0;
    renderPassInfo.renderArea.extent = ctx->swapChainExtent;
    
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->graphicsPipeline);
    
    VkViewport viewport = {0};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)ctx->swapChainExtent.width;
    viewport.height = (float)ctx->swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {0};
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent = ctx->swapChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        printf(RED "[ERROR] " RESET "failed to record command buffer!\n");
    }
}

char* readFile(const char *path, size_t *outSize)
{
  FILE *file = fopen(path, "rb");
  if (!file) {
    printf(RED "[ERROR] " RESET "failed to open file: %s\n", path);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if (fileSize < 0) {
    printf(RED "[ERROR] " RESET "failed to determine file size: %s\n", path);
    fclose(file);
    return NULL;
  }

  char *buffer = malloc(fileSize);
  if (!buffer) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for file: %s\n", path);
    fclose(file);
    return NULL;
  }

  size_t bytesRead = fread(buffer, 1, fileSize, file);
  if (bytesRead != (size_t)fileSize) {
    printf(RED "[ERROR] " RESET "failed to read file: %s\n", path);
    free(buffer);
    fclose(file);
    return NULL;
  }

  fclose(file);

  if (outSize) {
    *outSize = fileSize;
  }

  return buffer;
}

void freeSwapChainSupportDetails(SwapChainSupportDetails *details) {
  if (details->formats) {
    free(details->formats);
    details->formats = NULL;
  }
  if (details->presentModes) {
    free(details->presentModes);
    details->presentModes = NULL;
  }
}

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface) {
  SwapChainSupportDetails details = {0};

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, NULL);
  if (details.formatCount != 0) {
    details.formats = malloc(details.formatCount * sizeof(VkSurfaceFormatKHR));
    if (details.formats) {
      vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &details.formatCount, details.formats);
    }
  }

  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, NULL);
  if (details.presentModeCount != 0) {
    details.presentModes = malloc(details.presentModeCount * sizeof(VkPresentModeKHR));
    if (details.presentModes) {
      vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &details.presentModeCount, details.presentModes);
    }
  }

  return details;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t formatCount) {
  for (uint32_t i = 0; i < formatCount; i++) {
    if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormats[i];
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR *availablePresentModes, uint32_t presentModeCount) {
  for (uint32_t i = 0; i < presentModeCount; i++) {
    if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR *capabilities, uint32_t width, uint32_t height) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  } else {
    VkExtent2D actualExtent = {width, height};

    if (actualExtent.width < capabilities->minImageExtent.width) {
      actualExtent.width = capabilities->minImageExtent.width;
    } else if (actualExtent.width > capabilities->maxImageExtent.width) {
      actualExtent.width = capabilities->maxImageExtent.width;
    }

    if (actualExtent.height < capabilities->minImageExtent.height) {
      actualExtent.height = capabilities->minImageExtent.height;
    } else if (actualExtent.height > capabilities->maxImageExtent.height) {
      actualExtent.height = capabilities->maxImageExtent.height;
    }

    return actualExtent;
  }
}

VkShaderModule createShaderModule(const char *code, size_t codeSize, Vulkan_Context *vk_ctx) {
  VkShaderModuleCreateInfo createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = codeSize;
  createInfo.pCode = (const uint32_t*)code;
  VkShaderModule shaderModule;
  if (vkCreateShaderModule(vk_ctx->device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create shader module\n");
  }
  return shaderModule;
}

bool rendering_create(Rendering_Context *ctx, Vulkan_Context *vulkan_context, Platform_Context *platform) {
  if (!ctx || !vulkan_context) return false;

  ctx->vulkan_context = *vulkan_context;
  ctx->currentFrame = 0;  // INITIALIZE currentFrame
  
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(ctx->vulkan_context.physicalDevice, ctx->vulkan_context.surface);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(
      swapChainSupport.formats, 
      swapChainSupport.formatCount
      );
  VkPresentModeKHR presentMode = chooseSwapPresentMode(
      swapChainSupport.presentModes, 
      swapChainSupport.presentModeCount
      );
  VkExtent2D extent = chooseSwapExtent(&swapChainSupport.capabilities, platform->width, platform->height);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }
  
  VkSwapchainCreateInfoKHR createInfo = {0};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = ctx->vulkan_context.surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(ctx->vulkan_context.physicalDevice, ctx->vulkan_context.surface);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices = NULL;
  }
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(ctx->vulkan_context.device, &createInfo, NULL, &ctx->swapChain) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create swap chain!\n");
    freeSwapChainSupportDetails(&swapChainSupport);
    return false;
  }

  vkGetSwapchainImagesKHR(ctx->vulkan_context.device, ctx->swapChain, &imageCount, NULL);
  ctx->swapChainImageCount = imageCount;
  ctx->swapChainImages = malloc(imageCount * sizeof(VkImage));
  if (!ctx->swapChainImages) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for swapChainImages\n");
    freeSwapChainSupportDetails(&swapChainSupport);
    return false;
  }
  vkGetSwapchainImagesKHR(ctx->vulkan_context.device, ctx->swapChain, &imageCount, ctx->swapChainImages);

  ctx->swapChainImageFormat = surfaceFormat.format;
  ctx->swapChainExtent = extent;

  // FREE THE SWAP CHAIN SUPPORT DETAILS (MEMORY LEAK FIX)
  freeSwapChainSupportDetails(&swapChainSupport);

  printf(GREEN "[OK] " RESET "Swapchain\n");

  ctx->swapChainImageViews = malloc(ctx->swapChainImageCount * sizeof(VkImageView));
  if (ctx->swapChainImageViews == NULL) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for swapChainImageViews\n");
    return false;
  }

  for (size_t i = 0; i < ctx->swapChainImageCount; i++) {
    VkImageViewCreateInfo viewCreateInfo = {0};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = ctx->swapChainImages[i];
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewCreateInfo.format = ctx->swapChainImageFormat;
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewCreateInfo.subresourceRange.baseMipLevel = 0;
    viewCreateInfo.subresourceRange.levelCount = 1;
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;
    viewCreateInfo.subresourceRange.layerCount = 1;
    if (vkCreateImageView(ctx->vulkan_context.device, &viewCreateInfo, NULL, &ctx->swapChainImageViews[i]) != VK_SUCCESS) {
      printf(RED "[ERROR] " RESET "failed to create image views\n");
    }
  }
  printf(GREEN "[OK] " RESET "Image Views\n");

  printf("fetching shaders ...\n");

  size_t vertSize, fragSize;
  char *vertCode = readFile(vert_path, &vertSize);
  char *fragCode = readFile(frag_path, &fragSize);

  if (!vertCode || !fragCode) {
    if (vertCode) free(vertCode);
    if (fragCode) free(fragCode);
    return false;
  }

  ctx->vertShaderModule = createShaderModule(vertCode, vertSize, &ctx->vulkan_context);
  ctx->fragShaderModule = createShaderModule(fragCode, fragSize, &ctx->vulkan_context);

  free(vertCode);
  free(fragCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {0};
  vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module = ctx->vertShaderModule;
  vertShaderStageInfo.pName = "main";
  
  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {0};
  fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module = ctx->fragShaderModule;
  fragShaderStageInfo.pName = "main";
  
  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
  printf(GREEN "[OK] " RESET "Shaders\n");

  VkDynamicState dynamicStates[] = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {0};
  vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 0;
  vertexInputInfo.pVertexBindingDescriptions = NULL;
  vertexInputInfo.vertexAttributeDescriptionCount = 0;
  vertexInputInfo.pVertexAttributeDescriptions = NULL;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {0};
  inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkViewport viewport = {0};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = (float) ctx->swapChainExtent.width;
  viewport.height = (float) ctx->swapChainExtent.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {0};
  VkOffset2D offset = {0, 0};
  scissor.offset = offset;
  scissor.extent = ctx->swapChainExtent; 

  VkPipelineDynamicStateCreateInfo dynamicState = {0};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = 2; 
  dynamicState.pDynamicStates = dynamicStates;

  VkPipelineViewportStateCreateInfo viewportState = {0};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.pViewports = &viewport;
  viewportState.scissorCount = 1;
  viewportState.pScissors = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {0};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;
  rasterizer.depthBiasConstantFactor = 0.0f;
  rasterizer.depthBiasClamp = 0.0f;
  rasterizer.depthBiasSlopeFactor = 0.0f;

  VkPipelineMultisampleStateCreateInfo multisampling = {0};
  multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampling.minSampleShading = 1.0f;
  multisampling.pSampleMask = NULL;
  multisampling.alphaToCoverageEnable = VK_FALSE;
  multisampling.alphaToOneEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {0};
  colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_TRUE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlending = {0};
  colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.logicOp = VK_LOGIC_OP_COPY;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;
  colorBlending.blendConstants[0] = 0.0f;
  colorBlending.blendConstants[1] = 0.0f;
  colorBlending.blendConstants[2] = 0.0f;
  colorBlending.blendConstants[3] = 0.0f;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = NULL;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = NULL;

  if (vkCreatePipelineLayout(ctx->vulkan_context.device, &pipelineLayoutInfo, NULL, &ctx->pipelineLayout) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create pipeline layout!\n");
    return false;
  }
  printf(GREEN "[OK] " RESET "Pipeline Layout\n");

  VkAttachmentDescription colorAttachment = {0};
  colorAttachment.format = ctx->swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {0};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass = {0};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency = {0};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo = {0};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(ctx->vulkan_context.device, &renderPassInfo, NULL, &ctx->renderPass) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create render pass\n");
    return false;
  }
  printf(GREEN "[OK] " RESET "Render Pass\n");

  VkGraphicsPipelineCreateInfo pipelineInfo = {0};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = NULL;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = ctx->pipelineLayout;
  pipelineInfo.renderPass = ctx->renderPass;
  pipelineInfo.subpass = 0;
  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  if (vkCreateGraphicsPipelines(ctx->vulkan_context.device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &ctx->graphicsPipeline) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create graphics pipeline\n");
    return false;
  }
  printf(GREEN "[OK] " RESET "Graphics Pipeline\n");

  ctx->swapChainFramebuffers = malloc(ctx->swapChainImageCount * sizeof(VkFramebuffer));
  if (!ctx->swapChainFramebuffers) {
    printf(RED "[ERROR] " RESET "failed to allocate memory for swapChainFramebuffers\n");
    return false;
  }

  for (uint32_t i = 0; i < ctx->swapChainImageCount; i++) {
    VkImageView attachments[] = {ctx->swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {0};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = ctx->renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
    framebufferInfo.width = ctx->swapChainExtent.width;
    framebufferInfo.height = ctx->swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(ctx->vulkan_context.device, &framebufferInfo, NULL, &ctx->swapChainFramebuffers[i]) != VK_SUCCESS) {
      printf(RED "[ERROR] " RESET "failed to create framebuffer\n");
      return false;
    }
  }
  printf(GREEN "[OK] " RESET "Framebuffers\n");

  // ========== CREATE COMMAND POOL ==========
  QueueFamilyIndices cmdPoolIndices = findQueueFamilies(ctx->vulkan_context.physicalDevice, ctx->vulkan_context.surface);
  
  VkCommandPoolCreateInfo poolInfo = {0};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = cmdPoolIndices.graphicsFamily;

  if (vkCreateCommandPool(ctx->vulkan_context.device, &poolInfo, NULL, &ctx->commandPool) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to create command pool!\n");
    return false;
  }
  printf(GREEN "[OK] " RESET "Command Pool\n");

  // ========== ALLOCATE COMMAND BUFFERS ==========
  VkCommandBufferAllocateInfo allocInfo = {0};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = ctx->commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

  if (vkAllocateCommandBuffers(ctx->vulkan_context.device, &allocInfo, ctx->commandBuffers) != VK_SUCCESS) {
    printf(RED "[ERROR] " RESET "failed to allocate command buffers!\n");
    return false;
  }
  printf(GREEN "[OK] " RESET "Command Buffers\n");

  // ========== CREATE SYNCHRONIZATION OBJECTS ==========
  VkSemaphoreCreateInfo semaphoreInfo = {0};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {0};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

  // Allocate per-image semaphores and fence tracking
  ctx->renderFinishedSemaphores = malloc(ctx->swapChainImageCount * sizeof(VkSemaphore));
  ctx->imagesInFlight = malloc(ctx->swapChainImageCount * sizeof(VkFence));
  
  if (!ctx->renderFinishedSemaphores || !ctx->imagesInFlight) {
    printf(RED "[ERROR] " RESET "failed to allocate per-image sync objects!\n");
    free(ctx->renderFinishedSemaphores);
    free(ctx->imagesInFlight);
    return false;
  }
  
  // Initialize imagesInFlight to VK_NULL_HANDLE
  for (uint32_t i = 0; i < ctx->swapChainImageCount; i++) {
    ctx->imagesInFlight[i] = VK_NULL_HANDLE;
  }

  // Create per-frame semaphores and fences
  for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(ctx->vulkan_context.device, &semaphoreInfo, NULL, &ctx->imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(ctx->vulkan_context.device, &fenceInfo, NULL, &ctx->inFlightFences[i]) != VK_SUCCESS) {
      printf(RED "[ERROR] " RESET "failed to create per-frame synchronization objects!\n");
      return false;
    }
  }
  
  // Create per-image semaphores
  for (uint32_t i = 0; i < ctx->swapChainImageCount; i++) {
    if (vkCreateSemaphore(ctx->vulkan_context.device, &semaphoreInfo, NULL, &ctx->renderFinishedSemaphores[i]) != VK_SUCCESS) {
      printf(RED "[ERROR] " RESET "failed to create per-image semaphore!\n");
      return false;
    }
  }

  printf(GREEN "[OK] " RESET "Synchronization Objects (%d frames, %d images)\n", 
         MAX_FRAMES_IN_FLIGHT, ctx->swapChainImageCount);

  printf(GREEN "[OK] " RESET "Rendering Init Complete\n");
  return true;
}

// ========== RENDERING DRAW FUNCTION (FIXED SEMAPHORE USAGE!) ==========
void rendering_draw(Rendering_Context *ctx) {
    if (!ctx) return;

    uint32_t currentFrame = ctx->currentFrame;

    // Wait for the previous frame to finish
    vkWaitForFences(ctx->vulkan_context.device, 1, &ctx->inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swap chain
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(
        ctx->vulkan_context.device,
        ctx->swapChain,
        UINT64_MAX,
        ctx->imageAvailableSemaphores[currentFrame],  // Per-frame semaphore
        VK_NULL_HANDLE,
        &imageIndex
    );

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        printf(YELLOW "[WARNING] " RESET "Swap chain out of date\n");
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        printf(RED "[ERROR] " RESET "failed to acquire swap chain image!\n");
        return;
    }

    // Check if a previous frame is using this image (wait for it)
    if (ctx->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(ctx->vulkan_context.device, 1, &ctx->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    // Mark the image as now being in use by this frame
    ctx->imagesInFlight[imageIndex] = ctx->inFlightFences[currentFrame];

    // Reset the fence only after we're sure we're submitting work
    vkResetFences(ctx->vulkan_context.device, 1, &ctx->inFlightFences[currentFrame]);

    // Reset and record command buffer
    vkResetCommandBuffer(ctx->commandBuffers[currentFrame], 0);
    recordCommandBuffer(ctx, ctx->commandBuffers[currentFrame], imageIndex);

    // Submit command buffer
    VkSubmitInfo submitInfo = {0};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {ctx->imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &ctx->commandBuffers[currentFrame];

    // Signal per-image semaphore (indexed by imageIndex, not currentFrame!)
    VkSemaphore signalSemaphores[] = {ctx->renderFinishedSemaphores[imageIndex]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(ctx->vulkan_context.queue, 1, &submitInfo, ctx->inFlightFences[currentFrame]) != VK_SUCCESS) {
        printf(RED "[ERROR] " RESET "failed to submit draw command buffer!\n");
        return;
    }

    // Present the image
    VkPresentInfoKHR presentInfo = {0};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {ctx->swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = NULL;

    result = vkQueuePresentKHR(ctx->vulkan_context.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        printf(YELLOW "[WARNING] " RESET "Swap chain suboptimal or out of date\n");
    } else if (result != VK_SUCCESS) {
        printf(RED "[ERROR] " RESET "failed to present swap chain image!\n");
    }

    // Move to next frame
    ctx->currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void rendering_destroy(Rendering_Context *ctx) {
    if (!ctx) return;
    
    vkDeviceWaitIdle(ctx->vulkan_context.device);

    // Destroy per-frame synchronization objects
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (ctx->imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(ctx->vulkan_context.device, ctx->imageAvailableSemaphores[i], NULL);
        }
        if (ctx->inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(ctx->vulkan_context.device, ctx->inFlightFences[i], NULL);
        }
    }
    
    // Destroy per-image semaphores
    if (ctx->renderFinishedSemaphores) {
        for (uint32_t i = 0; i < ctx->swapChainImageCount; i++) {
            if (ctx->renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(ctx->vulkan_context.device, ctx->renderFinishedSemaphores[i], NULL);
            }
        }
        free(ctx->renderFinishedSemaphores);
        ctx->renderFinishedSemaphores = NULL;
    }
    
    if (ctx->imagesInFlight) {
        free(ctx->imagesInFlight);
        ctx->imagesInFlight = NULL;
    }

    if (ctx->commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(ctx->vulkan_context.device, ctx->commandPool, NULL);
    }
    
    if (ctx->swapChainFramebuffers) {
        for (uint32_t i = 0; i < ctx->swapChainImageCount; i++) {
            if (ctx->swapChainFramebuffers[i] != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(ctx->vulkan_context.device, ctx->swapChainFramebuffers[i], NULL);
            }
        }
        free(ctx->swapChainFramebuffers);
        ctx->swapChainFramebuffers = NULL;
    }
    
    if (ctx->graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(ctx->vulkan_context.device, ctx->graphicsPipeline, NULL);
        ctx->graphicsPipeline = VK_NULL_HANDLE;
    }
    
    if (ctx->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(ctx->vulkan_context.device, ctx->pipelineLayout, NULL);
        ctx->pipelineLayout = VK_NULL_HANDLE;
    }
    
    if (ctx->renderPass != VK_NULL_HANDLE) {
        vkDestroyRenderPass(ctx->vulkan_context.device, ctx->renderPass, NULL);
        ctx->renderPass = VK_NULL_HANDLE;
    }
    
    if (ctx->fragShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(ctx->vulkan_context.device, ctx->fragShaderModule, NULL);
        ctx->fragShaderModule = VK_NULL_HANDLE;
    }
    if (ctx->vertShaderModule != VK_NULL_HANDLE) {
        vkDestroyShaderModule(ctx->vulkan_context.device, ctx->vertShaderModule, NULL);
        ctx->vertShaderModule = VK_NULL_HANDLE;
    }
    
    if (ctx->swapChainImageViews) {
        for (uint32_t i = 0; i < ctx->swapChainImageCount; i++) {
            if (ctx->swapChainImageViews[i] != VK_NULL_HANDLE) {
                vkDestroyImageView(ctx->vulkan_context.device, ctx->swapChainImageViews[i], NULL);
            }
        }
        free(ctx->swapChainImageViews);
        ctx->swapChainImageViews = NULL;
    }
    
    if (ctx->swapChainImages) {
        free(ctx->swapChainImages);
        ctx->swapChainImages = NULL;
    }
    
    if (ctx->swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(ctx->vulkan_context.device, ctx->swapChain, NULL);
        ctx->swapChain = VK_NULL_HANDLE;
    }
}
