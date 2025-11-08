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
    float dt = 0.0f;
    float last_frame = 0.0f;
    float total_time = 0.0f;
    
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

    /* Initialise some resources */
    Buffer test_buffer = bank.create_buffer(BufferUsage::Constant | BufferUsage::TransferDst, 4).expect("failed to create buffer.");
    Texture test_texture = bank.create_texture(TextureUsage::Storage, TextureFormat::RGBA8Unorm, { 128, 128 }).expect("failed to create texture.");
    Image test_image = bank.create_image(test_texture).unwrap();

    /* Setup the framebuffer resize callback */ 
    WindowUserData user_data { &bank, rt };
    glfwSetWindowUserPointer(win, &user_data);
    glfwSetFramebufferSizeCallback(win, win_resize_cb);

    /* Initialize the immediate mode GUI */
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(win, true);
    ImGUI imgui = ImGUI();
    if (const Result r = imgui.init(gpu, rt, IMGUI_FUNCTIONS); r.is_err()) {
        printf("failed to initialize imgui.\nreason: %s\n", r.unwrap_err().c_str());
        return EXIT_SUCCESS;
    }

    /* Add the immediate mode GUI to the render graph */
    rg.add_imgui(imgui);

    /* Main loop */
    for (;;) {
        /* Get delta time */
        float currentFrame = glfwGetTime();
        dt = currentFrame - last_frame;
        total_time += dt;
        last_frame = currentFrame;
        
        /* Poll events */
        glfwPollEvents();
        int win_w, win_h;
        glfwGetWindowSize(win, &win_w, &win_h);

        /* Update constant buffer */
        bank.upload_buffer(test_buffer, &total_time, 0, sizeof(total_time));
        //printf("dt: %f\n", dt);

        /* Start a new imgui frame */
        imgui.new_frame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        /* End the imgui frame */
        ImGui::Render();

        rg.new_graph();

        /* Test Pass */
        rg.add_compute_pass("render pass", "test")
            .write(rt)
            .read(test_buffer)
            .group_size(16, 8)
            .work_size(win_w, win_h);
            
        /* Image Test Pass */
        rg.add_compute_pass("image test pass", "image")
            .write(test_image)
            .read(test_buffer)
            .group_size(16, 8)
            .work_size(128, 128);

        rg.end_graph();
        rg.dispatch().expect("failed to dispatch render graph.");

        /* Check if we are still running */
        if (glfwWindowShouldClose(win)) {
            break;
        }
    }

    /* Cleanup resources */
    bank.destroy_render_target(rt);
    bank.destroy_buffer(test_buffer);
    bank.destroy_image(test_image);
    bank.destroy_texture(test_texture);
    imgui.destroy().expect("failed to destroy imgui.");
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    /* Cleanup the VRAM bank & GPU adapter */
    rg.destroy().expect("failed to destroy render graph.");
    bank.destroy().expect("failed to destroy vram bank.");
    gpu.destroy().expect("failed to destroy gpu adapter.");

    glfwTerminate();

    return 0;
}
