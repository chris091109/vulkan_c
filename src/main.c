#include "platform.h"
#include "rendering.h"
#include "vulkan_init.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "color.h"

struct Global {
  Platform_Context platform;
  Vulkan_Context vulkan;
  Rendering_Context rendering;

  bool msaa_enabled;
  uint32_t msaa_sample;
};
struct Global global;

int main(void) {
  platform_create(&global.platform , 600, 500, "vulkan");
  vulkan_create(&global.vulkan, &global.platform);
  rendering_create(&global.rendering, &global.vulkan, &global.platform);

  while (!platform_should_close(&global.platform)) {
    platform_events();
    rendering_draw(&global.rendering);
  }
  rendering_destroy(&global.rendering);
  vulkan_destroy(&global.vulkan);
  platform_destroy(&global.platform);
}
