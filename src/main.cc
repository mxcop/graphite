#include "graphite/gpu_adapter.hh"
#include "graphite/vram_bank.hh"

#include <cstdio>
#include <cassert>

int main() {
    GPUAdapter gpu = GPUAdapter();
    VRAMBank bank = VRAMBank();

    /* Set a custom debug logger callback */
    gpu.set_logger(color_logger, DebugLevel::Verbose);

    /* Initialize the GPU adapter */
    if (const Result r = gpu.init(true); r.is_err()) {
        printf("failed to initialize gpu adapter.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }

    /* Initialize the VRAM bank */
    if (const Result r = bank.init(gpu); r.is_err()) {
        printf("failed to initialize vram bank.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }

    /* Cleanup the VRAM bank & GPU adapter */
    bank.destroy().expect("failed to destroy vram bank.");
    gpu.destroy().expect("failed to destroy gpu adapter.");

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

    return EXIT_SUCCESS;
}
