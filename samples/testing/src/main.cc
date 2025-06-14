#include <cstdio>

#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>

#include <graphite/gpu_adapter.hh>
#include <graphite/vram_bank.hh>
#include <graphite/render_target.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>

int main() {
    GPUAdapter gpu = GPUAdapter();
    VRAMBank bank = VRAMBank();
    RenderTarget rt = RenderTarget();
    RenderGraph rg = RenderGraph();

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

    /* Initialize the Render Graph */
    if (const Result r = rg.init(gpu); r.is_err()) {
        printf("failed to initialize render graph.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }

    /* Create a window using GLFW */
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow* win = glfwCreateWindow(1440, 810, "Graphite Test Sample", NULL, NULL);

    /* Initialize the GPU adapter */
    TargetDesc target { glfwGetWin32Window(win) };
    if (const Result r = rt.init(gpu, target); r.is_err()) {
        printf("failed to initialize render target.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }

    /* Main loop */
    for (;;) {
        /* Poll events */
        glfwPollEvents();

        rg.new_graph();

        rg.add_compute_pass("proc", "shader:proc")
            .write(rt)
            .group_size(16, 16)
            .work_size(1440, 810);

        rg.end_graph();
        rg.dispatch(gpu);

        /* Check if we are still running */
        if (glfwWindowShouldClose(win))
            break;
    }

    /* Cleanup the VRAM bank & GPU adapter */
    rt.destroy(gpu).expect("failed to destroy render target.");
    rg.destroy(gpu).expect("failed to destroy render graph.");
    bank.destroy().expect("failed to destroy vram bank.");
    gpu.destroy().expect("failed to destroy gpu adapter.");

    glfwTerminate();

    return 0;
}
