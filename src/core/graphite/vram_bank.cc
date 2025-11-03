#include "vram_bank.hh"

RenderTargetSlot& AgnVRAMBank::get_render_target(OpaqueHandle render_target) {
    return render_targets.get(render_target);
}

const RenderTargetSlot &AgnVRAMBank::get_render_target(OpaqueHandle render_target) const {
    return render_targets.get(render_target);
}
