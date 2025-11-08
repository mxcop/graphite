#include "vram_bank.hh"

RenderTargetSlot& AgnVRAMBank::get_render_target(BindHandle render_target) {
    return render_targets.get(render_target);
}

const RenderTargetSlot &AgnVRAMBank::get_render_target(BindHandle render_target) const {
    return render_targets.get(render_target);
}

BufferSlot& AgnVRAMBank::get_buffer(BindHandle buffer) {
    return buffers.get(buffer);
}

const BufferSlot& AgnVRAMBank::get_buffer(BindHandle buffer) const {
    return buffers.get(buffer);
}

TextureSlot& AgnVRAMBank::get_texture(OpaqueHandle texture) {
    return textures.get(texture);
}

const TextureSlot& AgnVRAMBank::get_texture(OpaqueHandle texture) const {
    return textures.get(texture);
}

ImageSlot& AgnVRAMBank::get_image(BindHandle image) {
    return images.get(image);
}

const ImageSlot& AgnVRAMBank::get_image(BindHandle image) const {
    return images.get(image);
}
