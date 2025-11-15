#include "vram_bank.hh"

void AgnVRAMBank::inc_handle_refs(OpaqueHandle& handle) {
    // printf("inc refs\n");
    switch (handle.get_type()) {
    case ResourceType::RenderTarget:
        render_targets.inc_ref(handle);
        break;
    case ResourceType::Buffer:
        buffers.inc_ref(handle);
        break;
    case ResourceType::Texture:
        textures.inc_ref(handle);
        break;
    case ResourceType::Image:
        images.inc_ref(handle);
        break;
    case ResourceType::Sampler:
        samplers.inc_ref(handle);
        break;
    default:
        break;
    }
}

void AgnVRAMBank::dec_handle_refs(OpaqueHandle& handle) {
    // printf("dec refs\n");
    switch (handle.get_type()) {
    case ResourceType::RenderTarget:
        if (render_targets.dec_ref(handle)) destroy_render_target((RenderTarget&)handle);
        break;
    case ResourceType::Buffer:
        if (buffers.dec_ref(handle)) destroy_buffer((Buffer&)handle);
        break;
    case ResourceType::Texture:
        if (textures.dec_ref(handle)) destroy_texture((Texture&)handle);
        break;
    case ResourceType::Image:
        if (images.dec_ref(handle)) destroy_image((Image&)handle);
        break;
    case ResourceType::Sampler:
        if (samplers.dec_ref(handle)) destroy_sampler((Sampler&)handle);
        break;
    default:
        break;
    }
}

void AgnVRAMBank::destroy_resource(OpaqueHandle& handle) {
    switch (handle.get_type()) {
    case ResourceType::RenderTarget:
        printf("destroying render target. (%d)\n", render_targets.refs[handle.get_index() - 1]);
        destroy_render_target((RenderTarget&)handle);
        break;
    case ResourceType::Buffer:
        printf("destroying buffer. (%d)\n", buffers.refs[handle.get_index() - 1]);
        destroy_buffer((Buffer&)handle);
        break;
    case ResourceType::Texture:
        printf("destroying texture. (%d)\n", textures.refs[handle.get_index() - 1]);
        destroy_texture((Texture&)handle);
        break;
    case ResourceType::Image:
        destroy_image((Image&)handle);
        printf("destroying image.\n");
        break;
    case ResourceType::Sampler:
        destroy_sampler((Sampler&)handle);
        printf("destroying sampler.\n");
        break;
    default:
        break;
    }
}
