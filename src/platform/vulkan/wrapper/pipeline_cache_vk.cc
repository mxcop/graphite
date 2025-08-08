#include "pipeline_cache_vk.hh"

#include "graphite/gpu_adapter.hh"
#include "graphite/nodes/compute_node.hh"

#include "shader_vk.hh"

Result<Pipeline> PipelineCache::get_pipeline(const ComputeNode &node) {
    const std::string key = std::string(node.get_sfa());

    /* Check the cache for a hit */
    if (cache.count(key) == 1u) return Ok(cache[key]);

    /* Try to load the shader module */
    const Result r_shader = shader::from_alias(gpu->logical_device, node.get_sfa());
    if (r_shader.is_err()) return Err(r_shader.unwrap_err());
    const VkShaderModule shader = r_shader.unwrap();

    return Err("todo: create the pipeline and save it into the cache!");
}
