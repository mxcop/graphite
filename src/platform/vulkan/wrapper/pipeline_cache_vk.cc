#include "pipeline_cache_vk.hh"

#include "graphite/nodes/compute_node.hh"

Result<Pipeline> PipelineCache::get_pipeline(const ComputeNode &node) {
    const std::string key = std::string(node.get_sfa());

    /* Check the cache for a hit */
    if (cache.count(key) == 1u) return Ok(cache[key]);

    return Err("todo: create the pipeline and save it into the cache!");
}
