#include "pipeline_cache_vk.hh"

#include "graphite/gpu_adapter.hh"
#include "graphite/nodes/compute_node.hh"

#include "shader_vk.hh"
#include "translate_vk.hh"

/* Create a descriptor layout for a compute node. */
Result<VkDescriptorSetLayout> descriptor_layout(VkDevice device, const ComputeNode& node);

Result<Pipeline> PipelineCache::get_pipeline(const std::string_view path, const ComputeNode& node) {
    const std::string key = std::string(node.compute_path);

    /* Check the cache for a hit */
    if (cache.count(key) == 1u) return Ok(cache[key]);

    Pipeline pipeline {};

    /* Try to load the shader module for the new pipeline */
    const Result r_shader = shader::from_alias(gpu->logical_device, path, node.compute_path);
    if (r_shader.is_err()) return Err(r_shader.unwrap_err());
    const VkShaderModule shader = r_shader.unwrap();
    
    /* Create the descriptor layout for the new pipeline */
    const Result r_layout = descriptor_layout(gpu->logical_device, node);
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

/* Create a descriptor layout binding for a render target resource. */
VkDescriptorSetLayoutBinding render_target(u32 slot, const Dependency& dep) {
    VkDescriptorSetLayoutBinding binding {};
    binding.binding = slot;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    binding.descriptorCount = 1u;
    binding.stageFlags = translate::stage_flags(dep.stages);
    return binding;
}

/* Create a descriptor layout for a compute node. */
Result<VkDescriptorSetLayout> descriptor_layout(VkDevice device, const ComputeNode& node) {
    /* Descriptor bindings */
    std::vector<VkDescriptorSetLayoutBinding> bindings {};

    /* Create a binding for each dependency */
    for (const Dependency& dep : node.dependencies) {
        const ResourceType rtype = dep.resource.get_type();
        const u32 slot = (u32)bindings.size();

        switch (rtype) {
            case ResourceType::RenderTarget:
                bindings.push_back(render_target(slot, dep));
                break;
            // case ResourceType::Buffer:
            //     bindings.push_back(buffer_binding((uint32)bindings.size(), dep));
            //     break;
            // case ResourceType::Image:
            //     bindings.push_back(image_binding((uint32)bindings.size(), dep));
            //     break;
            default:
                return Err("invalid resource type used in graph.");
        }
    }

    /* Descriptor set layout creation info (using push descriptors) */
    VkDescriptorSetLayoutCreateInfo layout_ci { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
    layout_ci.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR;
    layout_ci.bindingCount = (u32)bindings.size();
    layout_ci.pBindings = bindings.data();

    /* Create the descriptor set layout */
    VkDescriptorSetLayout layout {};
    if (vkCreateDescriptorSetLayout(device, &layout_ci, nullptr, &layout) != VK_SUCCESS) {
        return Err("failed to create descriptor set layout for '%s' node.", node.label.data());
    }
    return Ok(layout);
}
