#include "vram_bank.hh"

Result<void> VRAMBank::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    /* Initialize the Stack Pools */
    render_targets.init(8u);

    return Ok();
}

RenderTargetSlot& VRAMBank::get_render_target(OpaqueHandle render_target) {
    return render_targets.get(render_target);
}

const RenderTargetSlot &VRAMBank::get_render_target(OpaqueHandle render_target) const {
    return render_targets.get(render_target);
}
