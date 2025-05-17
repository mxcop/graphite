#include "graphite/gpu_adapter.hh"

#include <cstdio>
#include <cassert>

int main() {
    GPUAdapter gpu = GPUAdapter();

    gpu.init().unwrap();
    
    printf("test: %i\n", gpu.agn_function());

    gpu.destroy().unwrap();

    /* TODO: try out 'volk.h' */
    /* TODO: 'volkLoadDeviceTable' can be used per GPUAdapter for better performance! */
    /* Each GPUAdapter can hold its own 'VolkDeviceTable' instance */

    /* Initialize the GPU adapter */
    // GPUAdapter gpu = GPUAdapter();
    // printf("gpu: %s\n", gpu.name());

    /* Initialize a swapchain resource for video output */
    /* This seperates the OS related video output code from the render graph */
    // Swapchain swapchain = gpu.create_swapchain(FRAMES_IN_FLIGHT, window.handle);
    // swapchain.set_resolution(window.width, window.height);

    /* Initialize 2 VRAM banks, one for the game, and one for the editor */
    /* When building the game (without editor) we don't need to editor VRAM bank */
    // VRAMBank game_bank = gpu.create_vram_bank(MAX_RESOURCES);
    // game_bank.attach_target(swapchain); /* <- attach the swapchain for target sized resources */
    // VRAMBank editor_bank = gpu.create_vram_bank(MAX_RESOURCES);
    // Image img = game_bank.create_image(...); /* <- it's just a VRM :) */

    /* Initialize a render graph */
    // RenderGraph rg = gpu.create_graph(MAX_NODES);

    return 0;
}
