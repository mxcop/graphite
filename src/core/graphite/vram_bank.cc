#include "vram_bank.hh"

Result<void> AgnVRAMBank::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    /* Initialize the Stack Pools */
    render_targets.init(8u);

    return Ok();
}

RenderTargetSlot& AgnVRAMBank::get_render_target(OpaqueHandle render_target) {
    return render_targets.get(render_target);
}

const RenderTargetSlot &AgnVRAMBank::get_render_target(OpaqueHandle render_target) const {
    return render_targets.get(render_target);
}
