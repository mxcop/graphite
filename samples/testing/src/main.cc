#include <cstdio>

#include <glfw/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi")

#include <graphite/gpu_adapter.hh>
#include <graphite/vram_bank.hh>
#include <graphite/render_graph.hh>
#include <graphite/nodes/compute_node.hh>
#include <graphite/imgui.hh>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

struct WindowUserData {
    VRAMBank* bank {};
    RenderTarget rt {};
};

/* Called when the window is resized */
void win_resize_cb(GLFWwindow* win, int width, int height) {
    if (width == 0 || height == 0) return; /* Don't resize when minimized */
    WindowUserData* user_data = (WindowUserData*)glfwGetWindowUserPointer(win);
    user_data->bank->resize_render_target(user_data->rt, width, height);
}

int main() {
    /* Set a custom debug logger callback */
    GPUAdapter gpu = GPUAdapter();
    gpu.set_logger(color_logger, DebugLevel::Verbose);

    /* Initialize the GPU adapter */
    if (const Result r = gpu.init(true); r.is_err()) {
        printf("failed to initialize gpu adapter.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }

    /* Initialize the Render Graph */
    RenderGraph rg = RenderGraph();
    rg.set_shader_path("samples/testing/kernels");
    rg.set_max_graphs_in_flight(2u); /* Double buffering */
    if (const Result r = rg.init(gpu); r.is_err()) {
        printf("failed to initialize render graph.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }

    /* Get the VRAM bank */
    VRAMBank& bank = gpu.get_vram_bank();

    /* Create a window using GLFW */
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* win = glfwCreateWindow(1440, 810, "Graphite Test Sample", NULL, NULL);
    BOOL dark = TRUE;
    DwmSetWindowAttribute(glfwGetWin32Window(win), 20, &dark, sizeof(BOOL));

    /* Initialize the Render Target */
    const TargetDesc target { glfwGetWin32Window(win) };
    RenderTarget rt {};
    if (const Result r = bank.create_render_target(target); r.is_err()) {
        printf("failed to initialize render target.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    } else rt = r.unwrap();

    /* Setup the framebuffer resize callback */ 
    WindowUserData user_data { &bank, rt };
    glfwSetWindowUserPointer(win, &user_data);
    glfwSetFramebufferSizeCallback(win, win_resize_cb);
    ImGui_ImplGlfw_InitForVulkan(win, true);

    /* Initialize the immediate mode GUI */
    ImGUI imgui = ImGUI();
    ImGUIFunctions func { ImGui_ImplVulkan_Init };
    if (const Result r = imgui.init(gpu, rt, func); r.is_err()) {
        printf("failed to initialize imgui.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }
    // /* Add viewport image to ImGUI */
    // imgui.bind_image(Image);
    // imgui.unbind_image(Image);

#if 0
    const u32 key_7 = 0x7u;
    BindHandle dummy_7 = (BindHandle&)key_7;
    const u32 key_8 = 0x8u;
    BindHandle dummy_8 = (BindHandle&)key_8;
    const u32 key_9 = 0x9u;
    BindHandle dummy_9 = (BindHandle&)key_9;
    const u32 key_a = 0xAu;
    BindHandle dummy_a = (BindHandle&)key_a;
    const u32 key_b = 0xBu;
    BindHandle dummy_b = (BindHandle&)key_b;
    const u32 key_c = 0xCu;
    BindHandle dummy_c = (BindHandle&)key_c;
    const u32 key_d = 0xDu;
    BindHandle dummy_d = (BindHandle&)key_d;
    const u32 key_e = 0xEu;
    BindHandle dummy_e = (BindHandle&)key_e;
    const u32 key_f = 0xFu;
    BindHandle dummy_f = (BindHandle&)key_f;

    /* Main loop */
    for (;;) {
        /* Poll events */
        glfwPollEvents();

        rg.new_graph();

        /* Root A */
        rg.add_compute_pass("rA", "shader:dummy")
            .write(dummy_7)
            .group_size(16, 8)
            .work_size(1440, 810);
            
        /* Root B */
        rg.add_compute_pass("rB", "shader:dummy")
            .write(dummy_8)
            .group_size(16, 8)
            .work_size(1440, 810);
        rg.add_compute_pass("c0", "shader:dummy")
            .read(dummy_8)
            .write(dummy_9)
            .group_size(16, 8)
            .work_size(1440, 810);
            
        /* Root C */
        rg.add_compute_pass("rC", "shader:dummy")
            .write(dummy_a)
            .group_size(16, 8)
            .work_size(1440, 810);
        rg.add_compute_pass("c1", "shader:dummy")
            .read(dummy_a)
            .write(dummy_b)
            .group_size(16, 8)
            .work_size(1440, 810);

        /* C2 */
        rg.add_compute_pass("c2", "shader:dummy")
            .read(dummy_7)
            .read(dummy_9)
            .read(dummy_b)
            .write(dummy_c)
            .group_size(16, 8)
            .work_size(1440, 810); 
            
        /* C3 */
        rg.add_compute_pass("c3", "shader:dummy")
            .read(dummy_c)
            .write(dummy_d)
            .group_size(16, 8)
            .work_size(1440, 810); 
        /* C4 */
        rg.add_compute_pass("c4", "shader:dummy")
            .read(dummy_c)
            .write(dummy_e)
            .group_size(16, 8)
            .work_size(1440, 810); 
            
        /* C5 */
        rg.add_compute_pass("c5", "shader:dummy")
            .read(dummy_d)
            .read(dummy_e)
            .write(dummy_f)
            .group_size(16, 8)
            .work_size(1440, 810); 

        rg.end_graph();
        rg.dispatch();

        /* Check if we are still running */
        if (glfwWindowShouldClose(win))
            break;
        break; /* Exit for testing */
    }
#else
    /* Main loop */
    for (;;) {
        /* Poll events */
        glfwPollEvents();
        int win_w, win_h;
        glfwGetWindowSize(win, &win_w, &win_h);

        rg.new_graph();

        /* Test Pass */
        rg.add_compute_pass("render pass", "test")
            .write(rt)
            .group_size(16, 8)
            .work_size(win_w, win_h);

        /* ImGUI Pass */
        // rg.add_imgui_pass(imgui, rt);
            
        rg.end_graph();
        rg.dispatch().expect("failed to dispatch render graph.");

        /* Check if we are still running */
        if (glfwWindowShouldClose(win))
            break;
        // break; /* Exit for testing */
    }
#endif

    /* Cleanup resources */
    bank.destroy_render_target(rt);
    imgui.destroy().expect("failed to destroy imgui.");

    /* Cleanup the VRAM bank & GPU adapter */
    rg.destroy().expect("failed to destroy render graph.");
    bank.destroy().expect("failed to destroy vram bank.");
    gpu.destroy().expect("failed to destroy gpu adapter.");

    glfwTerminate();

    return 0;
}
