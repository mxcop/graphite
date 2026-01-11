#include "pipeline_cache_vk.hh"

#include "graphite/vram_bank.hh"
#include "graphite/gpu_adapter.hh"
#include "graphite/nodes/compute_node.hh"
#include "graphite/nodes/raster_node.hh"

#include "shader_vk.hh"
#include "translate_vk.hh"
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
    pipeline.name = node.compute_path;
    pipeline.name.erase(pipeline.name.end() - 3, pipeline.name.end());

    /* Try to load the shader module for the new pipeline */
    const Result r_shader = shader::from_alias(gpu->logical_device, path, node.compute_path);
    if (r_shader.is_err()) return Err(r_shader.unwrap_err());
    const VkShaderModule shader = r_shader.unwrap();
    const std::string shader_name = "Compute Shader (" + std::string(node.compute_path) + ")";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE, (u64)shader, shader_name.c_str());

    /* Create the descriptor layout for the new pipeline */
    const Result r_layout = node_descriptor_layout(*gpu, node);
    if (r_layout.is_err()) return Err(r_layout.unwrap_err());
    pipeline.descriptors = r_layout.unwrap();

    /* Pipeline layout creation info */
    const VkDescriptorSetLayout desc_layouts[] {pipeline.descriptors, gpu->get_vram_bank().bindless_layout};
    VkPipelineLayoutCreateInfo layout_ci {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_ci.setLayoutCount = sizeof(desc_layouts) / sizeof(VkDescriptorSetLayout);
    layout_ci.pSetLayouts = desc_layouts;

    /* Create the pipeline layout */
    if (vkCreatePipelineLayout(gpu->logical_device, &layout_ci, nullptr, &pipeline.layout) != VK_SUCCESS) {
        return Err("failed to create pipeline layout for '%s' node.", node.label.data());
    }
    const std::string pipeline_layout_name = "Compute Pipeline Layout (" + pipeline.name + ")";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipeline.layout, pipeline_layout_name.c_str());

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
    const std::string pipeline_name = "Compute Pipeline (" + pipeline.name + ")";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_PIPELINE, (u64)pipeline.pipeline, pipeline_name.c_str());

    /* We can free the shader module after compiling the pipeline */
    vkDestroyShaderModule(gpu->logical_device, shader, nullptr);

    /* Save the new pipeline to our cache and return it */
    cache[key] = pipeline;
    return Ok(pipeline);
}

Result<Pipeline> PipelineCache::get_pipeline(const std::string_view path, const RasterNode& node) {
    if (gpu == nullptr) return Err("tried to get pipeline from cache without gpu.");

    /* Check the cache for a hit */
    const std::string key = std::string(node.label);
    if (cache.count(key) == 1u) return Ok(cache[key]);

    /* Fill in the pipeline struct */
    Pipeline pipeline {};
    pipeline.name = std::string(node.vertex_path);
    pipeline.name.erase(pipeline.name.end() - 3, pipeline.name.end());

    /* Try to load the vertex shader module for the new pipeline */
    const Result r_vert_shader = shader::from_alias(gpu->logical_device, path, node.vertex_path);
    if (r_vert_shader.is_err()) return Err(r_vert_shader.unwrap_err());
    const VkShaderModule vert_shader = r_vert_shader.unwrap();
    const std::string v_shader_name = "Vertex Shader (" + std::string(node.vertex_path) + ")";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE, (u64)vert_shader, v_shader_name.c_str());

    /* Try to load the pixel shader module for the new pipeline */
    const Result r_frag_shader = shader::from_alias(gpu->logical_device, path, node.pixel_path);
    if (r_frag_shader.is_err()) return Err(r_frag_shader.unwrap_err());
    const VkShaderModule frag_shader = r_frag_shader.unwrap();
    const std::string f_shader_name = "Fragment Shader (" + std::string(node.pixel_path) + ")";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_SHADER_MODULE, (u64)frag_shader, f_shader_name.c_str());

    /* Pipeline shader stages */
    VkPipelineShaderStageCreateInfo stages[2] {};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vert_shader;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = frag_shader;
    stages[1].pName = "main";

    /* Create the descriptor layout for the new pipeline */
    const Result r_layout = node_descriptor_layout(*gpu, node);
    if (r_layout.is_err()) return Err(r_layout.unwrap_err());
    pipeline.descriptors = r_layout.unwrap();
    const std::string pd_name = "Push Descriptor Set Layout (" + pipeline.name + " Pipeline)";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, (u64)pipeline.descriptors, pd_name.c_str());

    /* Pipeline layout creation info */
    const VkDescriptorSetLayout desc_layouts[] {pipeline.descriptors, gpu->get_vram_bank().bindless_layout};
    VkPipelineLayoutCreateInfo layout_ci {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_ci.setLayoutCount = sizeof(desc_layouts) / sizeof(VkDescriptorSetLayout);
    layout_ci.pSetLayouts = desc_layouts;

    /* Create the pipeline layout */
    if (vkCreatePipelineLayout(gpu->logical_device, &layout_ci, nullptr, &pipeline.layout) != VK_SUCCESS) {
        return Err("failed to create pipeline layout for '%s' node.", node.label.data());
    }
    const std::string pipeline_layout_name = "Raster Pipeline Layout (" + pipeline.name + ")";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_PIPELINE_LAYOUT, (u64)pipeline.layout, pipeline_layout_name.c_str());

    /* Vertex attributes */
    std::vector<VkVertexInputAttributeDescription> vertex_attributes {};
    u32 vertex_stride = 0u;
    for (const AttrFormat attr : node.attributes) {
        const u32 size = translate::vertex_attribute_size(attr);
        const VkFormat format = translate::vertex_format(attr);

        VkVertexInputAttributeDescription attr {};
        attr.location = (u32)vertex_attributes.size();
        attr.binding = 0u;
        attr.format = format;
        attr.offset = vertex_stride;
        vertex_attributes.emplace_back(attr);
        vertex_stride += size;
    }

    /* Vertex binding (always 1, since we don't support non-interleaved vertex attributes) */
    VkVertexInputBindingDescription vertex_binding {};
    vertex_binding.binding = 0u;
    vertex_binding.stride = vertex_stride;
    vertex_binding.inputRate = translate::vertex_input_rate(node.vertex_input_rate);

    /* Pipeline vertex input state */
    VkPipelineVertexInputStateCreateInfo vertex_input { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
    vertex_input.vertexBindingDescriptionCount = 1u;
    vertex_input.pVertexBindingDescriptions = &vertex_binding;
    vertex_input.vertexAttributeDescriptionCount = (u32)vertex_attributes.size();
    vertex_input.pVertexAttributeDescriptions = vertex_attributes.data();

    /* Pipeline input assembly state */
    VkPipelineInputAssemblyStateCreateInfo assembly_input { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    assembly_input.topology = translate::primitive_topology(node.prim_topology);

    /* (placeholder) Pipeline tessellation state */
    VkPipelineTessellationStateCreateInfo tessellation_state { VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };

    const VkViewport viewport {};
    const VkRect2D scissor {};

    /* Pipeline viewport state */
    VkPipelineViewportStateCreateInfo viewport_state { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewport_state.viewportCount = 1u;
    viewport_state.pViewports = &viewport;
    viewport_state.scissorCount = 1u;
    viewport_state.pScissors = &scissor;

    /* Pipeline rasterizer state */
    VkPipelineRasterizationStateCreateInfo raster_state { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    raster_state.polygonMode = VK_POLYGON_MODE_FILL;
    raster_state.cullMode = VK_CULL_MODE_NONE;
    raster_state.frontFace = VK_FRONT_FACE_CLOCKWISE;
    raster_state.lineWidth = 1.0f;

    /* Pipeline multisample state */
    VkPipelineMultisampleStateCreateInfo multisample_state { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisample_state.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    /* Get the active VRAM bank */
    VRAMBank& bank = gpu->get_vram_bank();

    /* Find all attachment resource dependencies to put in the rendering info. */
    std::vector<VkFormat> color_attachments {};
    std::vector<VkPipelineColorBlendAttachmentState> blend_attachments {};
    for (const Dependency& dep : node.dependencies) {
        /* Find attachment dependencies */
        if (dep.usage != DependencyUsage::ColorAttachment) continue;

        VkFormat format {}; /* Get the image format for render target or texture */
        if (dep.resource.get_type() == ResourceType::RenderTarget) {
            format = bank.render_targets.get(dep.resource).format;
        } else {
            const TextureSlot& texture = bank.textures.get(dep.resource);
            if (has_flag(texture.usage, TextureUsage::ColorAttachment) == false) continue;
            format = translate::texture_format(texture.format);
        }

        color_attachments.emplace_back(format);
        VkPipelineColorBlendAttachmentState blend_state {};
        blend_state.blendEnable = node.alpha_blend ? VK_TRUE : VK_FALSE /* <- Alpha blending */;
        blend_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend_state.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend_state.colorBlendOp = VK_BLEND_OP_ADD;
        blend_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend_state.alphaBlendOp = VK_BLEND_OP_ADD;
        blend_state.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | (node.alpha_blend ? 0u : VK_COLOR_COMPONENT_A_BIT);
        blend_attachments.emplace_back(blend_state);
    }

    /* Dynamic rendering info */
    VkPipelineRenderingCreateInfoKHR dynamic_rendering { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
    dynamic_rendering.colorAttachmentCount = (u32)color_attachments.size();
    dynamic_rendering.pColorAttachmentFormats = color_attachments.data();

    /* Pipeline depth stencil state */
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
    depth_stencil_state.depthTestEnable = VK_FALSE;
    depth_stencil_state.depthWriteEnable = VK_FALSE;
    depth_stencil_state.depthCompareOp = VK_COMPARE_OP_LESS;

    /* Check for a depth stencil attachment */
    if (node.depth_stencil_image.is_null() == false) {
        const TextureSlot& depth_texture = bank.textures.get(bank.images.get(node.depth_stencil_image).texture);
        depth_stencil_state.depthTestEnable = node.depth_test;
        depth_stencil_state.depthWriteEnable = node.depth_write;
        dynamic_rendering.depthAttachmentFormat = translate::texture_format(depth_texture.format);
    }

    /* Pipeline color blend state */
    VkPipelineColorBlendStateCreateInfo color_blend_state { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    color_blend_state.attachmentCount = (u32)blend_attachments.size();
    color_blend_state.pAttachments = blend_attachments.data();

    /* Pipeline dynamic state */
    const VkDynamicState dynamic_states[] { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
    dynamic_state.dynamicStateCount = sizeof(dynamic_states) / sizeof(VkDynamicState);
    dynamic_state.pDynamicStates = dynamic_states;

    /* Pipeline creation info */
    VkGraphicsPipelineCreateInfo pipeline_ci { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    pipeline_ci.pNext = &dynamic_rendering;
    pipeline_ci.stageCount = 2u;
    pipeline_ci.pStages = stages;
    pipeline_ci.pVertexInputState = &vertex_input;
    pipeline_ci.pInputAssemblyState = &assembly_input;
    pipeline_ci.pTessellationState = &tessellation_state;
    pipeline_ci.pViewportState = &viewport_state;
    pipeline_ci.pRasterizationState = &raster_state;
    pipeline_ci.pMultisampleState = &multisample_state;
    pipeline_ci.pDepthStencilState = &depth_stencil_state;
    pipeline_ci.pColorBlendState = &color_blend_state;
    pipeline_ci.pDynamicState = &dynamic_state;
    pipeline_ci.layout = pipeline.layout;
    pipeline_ci.renderPass = nullptr;
    pipeline_ci.subpass = 0u;
    pipeline_ci.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_ci.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(gpu->logical_device, nullptr, 1u, &pipeline_ci, nullptr, &pipeline.pipeline) != VK_SUCCESS) {
        return Err("failed to create pipeline for '%s' node.", node.label.data());
    }
    const std::string pipeline_name = "Raster Pipeline (" + pipeline.name + ")";
    gpu->set_object_name(VkObjectType::VK_OBJECT_TYPE_PIPELINE, (u64)pipeline.pipeline, pipeline_name.c_str());

    /* We can free the shader modules after compiling the pipeline */
    vkDestroyShaderModule(gpu->logical_device, vert_shader, nullptr);
    vkDestroyShaderModule(gpu->logical_device, frag_shader, nullptr);

    /* Save the new pipeline to our cache and return it */
    cache[key] = pipeline;
    return Ok(pipeline);
}
