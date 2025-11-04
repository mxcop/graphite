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
