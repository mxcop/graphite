#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"

#include <unordered_map>
#include <string>

class GPUAdapter;
class ComputeNode;

/* Collection of everything that makes up a pipeline. */
struct Pipeline {
    VkDescriptorSetLayout descriptors {};
    VkPipelineLayout layout {};
    VkPipeline pipeline {};
};

/* Vulkan shader pipeline cache. */
class PipelineCache {
    GPUAdapter* gpu = nullptr;

    /* Hash table with (key: shader_alias, value: pipeline) */
    std::unordered_map<std::string, Pipeline> cache {};

public:
    PipelineCache() = default;
    PipelineCache(GPUAdapter& gpu) : gpu(&gpu) {}

    /* Get a pipeline from the cache, or load the pipeline if it's not already cached. */
    Result<Pipeline> get_pipeline(const std::string_view path, const ComputeNode& node);
};
