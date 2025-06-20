#include "vram_bank.hh"

#include "render_target.hh"

Result<void> VRAMBank::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    /* Initialize the Stack Pools */
    render_targets.init(8u);

    return Ok();
}

Result<RenderTarget*> VRAMBank::create_render_target(TargetDesc& target, u32 def_width, u32 def_height) {
    return Ok(&render_targets.pop().data);
}
