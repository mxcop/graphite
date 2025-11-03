#pragma once

#include "vulkan/api_vk.hh" /* Vulkan API */
#include "graphite/utils/result.hh"
#include "graphite/utils/types.hh"

struct Pipeline;
struct RenderTarget;
struct GraphExecution;

class Node;
class VRAMBank;
class GPUAdapter;
class RenderGraph;

/* Create the descriptor layout for a render graph node. */
Result<VkDescriptorSetLayout> node_descriptor_layout(GPUAdapter& gpu, const Node& node);

/* Push all descriptors for a render graph node onto the command buffer. */
Result<void> node_push_descriptors(const RenderGraph& rg, const Pipeline& pipeline, const Node& node);

/* Synchronize all descriptors for a render graph wave. */
Result<void> wave_sync_descriptors(const RenderGraph& rg, u32 start, u32 end);
