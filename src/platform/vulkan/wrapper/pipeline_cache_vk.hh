#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"

#include <unordered_map>
#include <string>

class GPUAdapter;
class ComputeNode;
class RasterNode;

/* Collection of everything that makes up a pipeline. */
struct Pipeline {
    VkDescriptorSetLayout descriptors {};
    VkPipelineLayout layout {};
    VkPipeline pipeline {};

    std::string name = "";
};

/* Vulkan shader pipeline cache. */
class PipelineCache {
    GPUAdapter* gpu = nullptr;

    /* Hash table with (key: shader_alias, value: pipeline) */
    std::unordered_map<std::string, Pipeline> cache {};

public:
    PipelineCache() = default;

    /* Initialize the pipeline cache. */
    void init(GPUAdapter& gpu_adapter) { this->gpu = &gpu_adapter; };

    /* Evict any pipelines from the pipeline cache. */
    void evict();

    /* Get a pipeline from the cache, or load the pipeline if it's not already cached. */
    Result<Pipeline> get_pipeline(const std::string_view path, const ComputeNode& node);

    /* Get a pipeline from the cache, or load the pipeline if it's not already cached. */
    Result<Pipeline> get_pipeline(const std::string_view path, const RasterNode& node);
};
