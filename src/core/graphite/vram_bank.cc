#include "vram_bank.hh"

Result<void> VRAMBank::init(GPUAdapter& gpu, u32 bank_index) {
    this->gpu = &gpu;
    this->bank_index = bank_index;

    /* Initialize the Stack Pools */
    render_targets.init(bank_index, 8u);

    return Ok();
}

Result<RenderTarget> VRAMBank::create_render_target(TargetDesc& target, u32 def_width, u32 def_height) {
    return Ok(render_targets.pop().handle);
}
