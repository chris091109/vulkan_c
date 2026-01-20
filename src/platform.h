#ifndef PLATFORM_H
#define PLATFORM_H

#define GLFW_INCLUDE_VULKAN

#include <stdbool.h>
#include <stdint.h>
#include <GLFW/glfw3.h>
#include "color.h"

typedef struct Platform_Context Platform_Context;
struct Platform_Context {
  uint32_t width;
  uint32_t height;
  const char *title;

  GLFWwindow *window;
};

bool platform_create(Platform_Context *ctx, uint32_t w, uint32_t h, const char * title);
bool platform_should_close(Platform_Context *ctx);
void platform_events(void);
void platform_destroy(Platform_Context *ctx);

#endif
