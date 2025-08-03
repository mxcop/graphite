#include "render_graph_vk.hh"

Result<void> RenderGraph::init(GPUAdapter& gpu) {
    this->gpu = &gpu;
    return Ok();
}

Result<void> RenderGraph::dispatch() {
    return Ok();
}

Result<void> RenderGraph::destroy() {
    return Ok();
}
