#include "pipeline_cache_vk.hh"

#include "graphite/gpu_adapter.hh"
#include "graphite/nodes/compute_node.hh"

#include "shader_vk.hh"
#include "descriptor_vk.hh"

void PipelineCache::evict() {
    for (const auto& [_, pipeline] : cache) {
        vkDestroyDescriptorSetLayout(gpu->logical_device, pipeline.descriptors, nullptr);
        vkDestroyPipelineLayout(gpu->logical_device, pipeline.layout, nullptr);
        vkDestroyPipeline(gpu->logical_device, pipeline.pipeline, nullptr);
    }
    cache.clear();
}

Result<Pipeline> PipelineCache::get_pipeline(const std::string_view path, const ComputeNode& node) {
    if (gpu == nullptr) return Err("tried to get pipeline from cache without gpu.");

    /* Check the cache for a hit */
    const std::string key = std::string(node.compute_path);
    if (cache.count(key) == 1u) return Ok(cache[key]);

    /* Fill in the pipeline struct */
    Pipeline pipeline {};

    /* Try to load the shader module for the new pipeline */
    const Result r_shader = shader::from_alias(gpu->logical_device, path, node.compute_path);
    if (r_shader.is_err()) return Err(r_shader.unwrap_err());
    const VkShaderModule shader = r_shader.unwrap();
    
    /* Create the descriptor layout for the new pipeline */
    const Result r_layout = node_descriptor_layout(*gpu, node);
    if (r_layout.is_err()) return Err(r_layout.unwrap_err());
    pipeline.descriptors = r_layout.unwrap();

    /* Pipeline layout creation info */
    /* TODO: Insert bindless descriptor set here later. */
    VkPipelineLayoutCreateInfo layout_ci { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    layout_ci.setLayoutCount = 1u;
    layout_ci.pSetLayouts = &pipeline.descriptors;

    /* Create the pipeline layout */
    if (vkCreatePipelineLayout(gpu->logical_device, &layout_ci, nullptr, &pipeline.layout) != VK_SUCCESS) {
        return Err("failed to create pipeline layout for '%s' node.", node.label.data());
    }

    /* Pipeline stage creation info */
    VkPipelineShaderStageCreateInfo stage_ci { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    stage_ci.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stage_ci.module = shader;
    stage_ci.pName = "main";
    
    /* Pipeline creation info */
    VkComputePipelineCreateInfo pipeline_ci { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
    pipeline_ci.stage = stage_ci;
    pipeline_ci.layout = pipeline.layout;

    /* Create compute pipeline */
    if (vkCreateComputePipelines(gpu->logical_device, VK_NULL_HANDLE, 1u, &pipeline_ci, nullptr, &pipeline.pipeline) != VK_SUCCESS) {
        return Err("failed to create pipeline for '%s' node.", node.label.data());
    }

    /* We can free the shader module after compiling the pipeline */
    vkDestroyShaderModule(gpu->logical_device, shader, nullptr);

    /* Save the new pipeline to our cache and return it */
    cache[key] = pipeline;
    return Ok(pipeline);
}
