#pragma once

#include <string_view>

#include "node.hh"

struct DrawCall;

/* Vertex attribute format. */
enum class AttrFormat : u32 {
    Invalid = 0u,  /* Invalid vertex attribute format. */
    X32_SFloat,    /* X 32 bits per channel, signed float. */
    XY32_SFloat,   /* XY 32 bits per channel, signed float. */
    XYZ32_SFloat,  /* XYZ 32 bits per channel, signed float. */
    XYZW32_SFloat, /* XYZW 32 bits per channel, signed float. */
    EnumLimit      /* Anything above or equal is invalid. */
};

/* Vertex primitive topology. */
enum class Topology : u32 {
    Invalid = 0u, /* Invalid primitive topology. */
    TriangleList, /* List of triangles. */
    LineList,     /* List of lines. */
    EnumLimit     /* Anything above or equal is invalid. */
};

/* Pixel load operation. */
enum class LoadOp : u32 {
    Load, /* Load pixel data already present. */
    Clear /* Clear pixel data. */
};

/* Vertex input rate. */
enum class VertexInputRate : u32 {
    Vertex, /* Advance one vertex with each vertex. */
    Instance, /* Advance one vertex with each instance. */
};

/* Stencil test operation. */
enum class StencilOp : u32 {
    Keep,           /* keep existing stencil value. */
    Zero,           /* set stencil value to zero. */
    Replace,        /* replace stencil value with the reference value. */
    IncrementClamp, /* increment stencil value and clamp to 255. */
    DecrementClamp, /* decrement stencil value and clamp to 0. */
    Invert,         /* inverts each bit of the stencil value. */
    IncrementWrap,  /* increment stencil value and wrap to 0 (256 -> 0). */
    DecrementWrap   /* decrement stencil value and wrap to 255 (-1 -> 255). */
};

/* Stencil comparison operation. */
enum class CompareOp : u32 {
    Never,
    Less,
    Equal,
    LessEqual,
    Greater,
    NotEqual,
    GreaterEqual,
    Always
};

/* Stencil state. */
struct StencilState {
    StencilState() = default;

    StencilOp fail_op = StencilOp::Replace;
    StencilOp pass_op = StencilOp::Replace;
    StencilOp depth_fail_op = StencilOp::Replace;

    CompareOp compare_op = CompareOp::Always;
    uint32_t compare_mask = 0x00;  /* comparison mask */
    uint32_t write_mask = 0x00;    /* write mask */
    uint32_t reference_value = 0u; /* value to write/compare against */

    bool test = false; /* perform stencil test */
    LoadOp load_op = LoadOp::Clear; /* operation to perform on load in a raster pass (load or clear) */
};

/**
 * Render Graph Rasterisation Node.
 * Used to build a rasterisation shader pass.
 */
class RasterNode : public Node {
    /* Can only be constructed by the RenderGraph */
    RasterNode() = default;
    RasterNode(std::string_view label, std::string_view vx_path, std::string_view fg_path);
    ~RasterNode() override;

   public:
    /* Shader file paths */
    std::string_view vertex_path {};
    std::string_view pixel_path {};

    /* Vertex shader attributes */
    std::vector<AttrFormat> attributes {};
    Topology prim_topology = Topology::Invalid;
    LoadOp pixel_load_op = LoadOp::Load;
    VertexInputRate vertex_input_rate = VertexInputRate::Vertex;
    bool alpha_blend = false;

    /* Depth/Stencil image */
    Image depth_stencil_image {};
    LoadOp depth_load_op = LoadOp::Load;
    bool depth_test = true;
    bool depth_write = true;
    StencilState stencil_state {};

    /* Push Constants */
    ShaderStages pc_stages {};

    /* Extents */
    u32 raster_w = 0u, raster_h = 0u, raster_x = 0u, raster_y = 0u;

    /* Draw calls */
    std::vector<DrawCall> draws {};

    /* No copies allowed */
    RasterNode(const RasterNode&) = delete;
    RasterNode& operator=(const RasterNode&) = delete;

    /* Add a vertex attribute to the pass. (vertex attributes are always interleaved in 1 buffer) */
    RasterNode& attribute(const AttrFormat format);

    /* Set the vertex primitive topology of the pass. */
    RasterNode& topology(const Topology type);

    /* Set the pixel load operation of the pass. */
    RasterNode& load_op_color(const LoadOp op);

    /* Set the depth load operation of the pass. */
    RasterNode& load_op_depth(const LoadOp op);

    /* Set the vertex input rate of the pass. */
    RasterNode& input_rate(const VertexInputRate rate);

    /* Set the alpha blending of the pass. */
    RasterNode& alpha_blending(const bool blend);

    /* Add a bindable resource as an output for this node. */
    RasterNode& write(BindHandle resource, ShaderStages stages);

    /* Add a bindable resource as an input for this node. */
    RasterNode& read(BindHandle resource, ShaderStages stages);

    /**
     * @brief Set push constants for this node.
     * @param offset Offset of the push constants in bytes.
     * @param size Size of the push constants in bytes.
     */
    RasterNode& push_constants(void* data, u32 offset, u32 size, ShaderStages stages);

    /* Add a rendering attachment as an output for the pixel stage */
    RasterNode& attach(BindHandle resource);

    /* Add a depth/stencil attachment as an input/output */
    RasterNode& depth_stencil(Image image, bool test = true, bool write = true, StencilState s_state = StencilState());

    /* Set the raster extent of the raster pass. (the extent of the attachments to rasterize into) */
    RasterNode& raster_extent(const u32 w, const u32 h, const u32 x = 0u, const u32 y = 0u);

    /* Create a draw call for this raster pass. */
    DrawCall& draw(
        const Buffer vertex_buffer, const u32 vertex_count, const u32 vertex_offset = 0u, const u32 instance_count = 1u,
        const u32 instance_offset = 0u
    );
    /* Create an indirect draw call for this raster pass. */
    DrawCall& draw_indirect(const Buffer vertex_buffer, const Buffer indirect_buffer);

    /* To access constructors */
    friend class AgnRenderGraph;
};

/* Render Graph raster pass draw call. */
struct DrawCall {
    RasterNode& parent_pass;

    Buffer vertex_buffer {};
    Buffer indirect_buffer {};
    u32 vertex_count = 0u, vertex_offset = 0u;
    u32 instance_count = 0u, instance_offset = 0u;

    /* Vertex buffer draw call constructor. */
    DrawCall(
        RasterNode& parent_pass, const Buffer vertex_buffer, const u32 vertex_count, const u32 vertex_offset,
        const u32 instance_count, const u32 instance_offset
    );
    /* Vertex buffer and indirect draw call constructor. */
    DrawCall(RasterNode& parent_pass, const Buffer vertex_buffer, const Buffer indirect_buffer);
};