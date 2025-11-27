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

/**
 * Render Graph Rasterisation Node.
 * Used to build a rasterisation shader pass.
 */
class RasterNode : public Node {
    /* Can only be constructed by the RenderGraph */
    RasterNode() = default;
    RasterNode(std::string_view label, std::string_view vx_path, std::string_view fg_path);

   public:
    /* Shader file paths */
    std::string_view vertex_path {};
    std::string_view pixel_path {};

    /* Vertex shader attributes */
    std::vector<AttrFormat> attributes {};
    Topology prim_topology = Topology::Invalid;
    LoadOp pixel_load_op = LoadOp::Load;

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
    RasterNode& load_op(const LoadOp op);

    /* Add a bindable resource as an output for this node. */
    RasterNode& write(BindHandle resource, ShaderStages stages);

    /* Add a bindable resource as an input for this node. */
    RasterNode& read(BindHandle resource, ShaderStages stages);

    /* Add a rendering attachment as an output for the pixel stage */
    RasterNode& attach(BindHandle resource);

    /* Set the raster extent of the raster pass. (the extent of the attachments to rasterize into) */
    RasterNode& raster_extent(const u32 w, const u32 h, const u32 x = 0u, const u32 y = 0u);

    /* Create a draw call for this raster pass. */
    DrawCall& draw(
        const Buffer vertex_buffer, const u32 vertex_count, const u32 vertex_offset = 0u, const u32 instance_count = 1u,
        const u32 instance_offset = 0u
    );

    /* To access constructors */
    friend class AgnRenderGraph;
};

/* Render Graph raster pass draw call. */
struct DrawCall {
    RasterNode& parent_pass;

    Buffer vertex_buffer {};
    u32 vertex_count = 0u, vertex_offset = 0u;
    u32 instance_count = 0u, instance_offset = 0u;

    /* Vertex buffer draw call constructor. */
    DrawCall(
        RasterNode& parent_pass, const Buffer vertex_buffer, const u32 vertex_count, const u32 vertex_offset,
        const u32 instance_count, const u32 instance_offset
    );
};