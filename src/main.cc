#include "core/test.hh"

#include <cstdio>

int main() {
    TestAdapter t = TestAdapter();
    t.spc_function();

    /* Initialize the GPU adapter */
    // GPUAdapter gpu = GPUAdapter();
    // printf("gpu: %s\n", gpu.name());

    /* Initialize a swapchain resource for video output */
    /* This seperates the OS related video output code from the render graph */
    // Swapchain swapchain = gpu.create_swapchain(FRAMES_IN_FLIGHT, window.handle);
    // swapchain.set_resolution(window.width, window.height);

    /* Initialize 2 VRAM banks, one for the game, and one for the editor */
    /* When building the game (without editor) we don't need to editor VRAM bank */
    // VRAMBank game_bank = gpu.create_vram_bank(MAX_RESOURCES, swapchain);
    // VRAMBank editor_bank = gpu.create_vram_bank(MAX_RESOURCES);
    // Image img = game_vrm.create_image(...); /* <- it's just a VRM :) */

    /* Initialize a render graph */
    // RenderGraph rg = gpu.create_graph(MAX_NODES);

    printf("test: %i\n", t.agn_function());

    return 0;
}
