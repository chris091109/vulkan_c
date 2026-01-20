#include "platform.h"
#include "color.h"
#include <GLFW/glfw3.h>
#include <stdio.h>

bool platform_create(Platform_Context *ctx, uint32_t w, uint32_t h, const char *t) {
  if (!ctx) return false;
  if (!glfwInit()) {
    printf(RED "[ERROR] " RESET "failed to initialize glfw\n");
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  ctx->window = glfwCreateWindow(w, h, t, NULL, NULL);
  if (!ctx->window)
  {
    printf(RED "[ERROR] " RESET "failed to create window\n");
    return false;
  }
  ctx->width = w;
  ctx->height = h;
  ctx->title = t;
  printf(GREEN "[OK] " RESET "window\n");
  return true;
}

bool platform_should_close(Platform_Context *ctx) {
  if (!ctx) return false;
  return glfwWindowShouldClose(ctx->window);
}

void platform_events(void) {
  glfwPollEvents();
}

void platform_destroy(Platform_Context *ctx) {
  if (!ctx) return;
  glfwDestroyWindow(ctx->window);
  glfwTerminate();
}
