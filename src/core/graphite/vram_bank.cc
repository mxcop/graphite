#include "vram_bank.hh"

Result<void> VRAMBank::init(GPUAdapter& gpu) {
    this->gpu = &gpu;

    /* Initialize the Stack Pools */
    render_targets.init(8u);

    return Ok();
}
