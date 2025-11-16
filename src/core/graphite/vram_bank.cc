#include "vram_bank.hh"

void AgnVRAMBank::destroy(OpaqueHandle& resource) {
    remove_reference(resource);
    /* Set the handle to NULL */
    resource = OpaqueHandle();
}

void AgnVRAMBank::add_reference(OpaqueHandle resource) {
    switch (resource.get_type()) {
    case ResourceType::RenderTarget:
        render_targets.add_reference(resource);
        break;
    case ResourceType::Buffer:
        buffers.add_reference(resource);
        break;
    case ResourceType::Texture:
        textures.add_reference(resource);
        break;
    case ResourceType::Image:
        images.add_reference(resource);
        break;
    case ResourceType::Sampler:
        samplers.add_reference(resource);
        break;
    }
}

void AgnVRAMBank::remove_reference(OpaqueHandle resource) {
    switch (resource.get_type()) {
    case ResourceType::RenderTarget:
        if (render_targets.remove_reference(resource)) destroy_render_target((RenderTarget&)resource);
        break;
    case ResourceType::Buffer:
        if (buffers.remove_reference(resource)) destroy_buffer((Buffer&)resource);
        break;
    case ResourceType::Texture:
        if (textures.remove_reference(resource)) destroy_texture((Texture&)resource);
        break;
    case ResourceType::Image:
        if (images.remove_reference(resource)) destroy_image((Image&)resource);
        break;
    case ResourceType::Sampler:
        if (samplers.remove_reference(resource)) destroy_sampler((Sampler&)resource);
        break;
    }
}
