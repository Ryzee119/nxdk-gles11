#include "arena.h"
#include "gles_private.h"

// GPU staging arena for client-side vertex arrays.
// The NV2A GPU reads vertex data via DMA from physical addresses, so client-side
// arrays (from malloc/stack) must be copied into contiguous GPU-accessible memory
// before drawing. VBO-backed arrays are already in contiguous memory and are skipped.

// Maximum number of enabled attribute arrays we could have at once:
// vertex + normal + color + texcoord * N + point_size
#define MAX_STAGED_RANGES (1 + 1 + 1 + GLI_MAX_TEXTURE_UNITS + 1)

// Tracks a source range that has already been copied into the arena,
// so that interleaved arrays sharing the same memory block are only copied once.
typedef struct
{
    const uint8_t *src_start;
    const uint8_t *src_end;
    uint8_t *dst_start;
} staged_range_t;

void gliStagingInit(void)
{
    gli_context_t *context = gliGetContext();

    void *pool =
        MmAllocateContiguousMemoryEx(GLI_STAGING_ARENA_SIZE, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    assert(pool != NULL);

    context->staging_arena_pool = pool;
    arena_init(pool, GLI_STAGING_ARENA_SIZE, &context->staging_arena);
}

void gliStagingDestroy(void)
{
    gli_context_t *context = gliGetContext();
    if (context->staging_arena_pool) {
        MmFreeContiguousMemory(context->staging_arena_pool);
        context->staging_arena_pool = NULL;
    }
}

// Check if this source range is fully contained within an already-staged range.
// If so, return the destination pointer adjusted for the offset, otherwise return NULL.
static void *find_staged_overlap(const staged_range_t *ranges,
                                 int count,
                                 const uint8_t *src_start,
                                 const uint8_t *src_end)
{
    for (int i = 0; i < count; i++) {
        if (src_start >= ranges[i].src_start && src_end <= ranges[i].src_end) {
            ptrdiff_t offset = src_start - ranges[i].src_start;
            return ranges[i].dst_start + offset;
        }
    }
    return NULL;
}

// Copy a source range into the staging arena. Checks for interleaving first.
// Returns the new GPU-accessible pointer, or NULL on arena overflow.
static void *stage_range(
    arena_t *arena, staged_range_t *ranges, int *range_count, const void *ptr, GLsizei stride, GLsizei vertex_count)
{
    const uint8_t *src_start = (const uint8_t *)ptr;
    const uint8_t *src_end = src_start + (vertex_count * stride);

    // Check if this range was already copied (interleaved with another attribute)
    void *existing = find_staged_overlap(ranges, *range_count, src_start, src_end);
    if (existing) {
        return existing;
    }

    uint32_t byte_size = (uint32_t)(src_end - src_start);

    // Round robin if we will go over capacity
    if (arena_available(arena) < byte_size) {
        arena_reset(arena);
    }

    void *dst = arena_alloc(arena, byte_size);
    if (!dst) {
        return NULL;
    }
    gli_memcpy(dst, src_start, byte_size);

    // Record this range for interleaving detection
    if (*range_count < MAX_STAGED_RANGES) {
        ranges[*range_count].src_start = src_start;
        ranges[*range_count].src_end = src_end;
        ranges[*range_count].dst_start = (uint8_t *)dst;
        (*range_count)++;
    }

    return dst;
}

// Compute the effective stride for an attribute array.
// OpenGL spec: if stride is 0, elements are tightly packed.
static GLsizei compute_stride(GLsizei user_stride, GLint component_count, GLenum type)
{
    if (user_stride != 0) {
        return user_stride;
    }
    return (GLsizei)(component_count * gliEnumtoByteSize(type));
}

GLboolean gliStageClientArrays(GLsizei vertex_count)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;
    arena_t *arena = &context->staging_arena;

    if (vertex_count <= 0) {
        return GL_TRUE;
    }

    staged_range_t ranges[MAX_STAGED_RANGES];
    int range_count = 0;

    // --- Vertex array ---
    if (vad->vertex_array_enabled && vad->vertex_array_buffer_binding == 0 && vad->vertex_array_ptr != NULL) {
        GLsizei stride = compute_stride(vad->vertex_array_stride, vad->vertex_array_size, vad->vertex_array_type);
        void *staged = stage_range(arena, ranges, &range_count, vad->vertex_array_ptr, stride, vertex_count);
        if (!staged) {
            goto out_of_memory;
        }
        XguVertexArrayType format = gliEnumToNvType(vad->vertex_array_type);
        xgux_set_attrib_pointer(XGU_VERTEX_ARRAY, format, vad->vertex_array_size, stride, staged);
        vad->vertex_array_dirty = GL_FALSE;
    }

    // --- Normal array ---
    if (vad->normal_array_enabled && vad->normal_array_buffer_binding == 0 && vad->normal_array_ptr != NULL) {
        GLsizei stride = compute_stride(vad->normal_array_stride, 3, vad->normal_array_type);
        void *staged = stage_range(arena, ranges, &range_count, vad->normal_array_ptr, stride, vertex_count);
        if (!staged) {
            goto out_of_memory;
        }
        XguVertexArrayType format = gliEnumToNvType(vad->normal_array_type);
        xgux_set_attrib_pointer(XGU_NORMAL_ARRAY, format, 3, stride, staged);
        vad->normal_array_dirty = GL_FALSE;
    }

    // --- Color array ---
    if (vad->color_array_enabled && vad->color_array_buffer_binding == 0 && vad->color_array_ptr != NULL) {
        GLsizei stride = compute_stride(vad->color_array_stride, vad->color_array_size, vad->color_array_type);
        void *staged = stage_range(arena, ranges, &range_count, vad->color_array_ptr, stride, vertex_count);
        if (!staged) {
            goto out_of_memory;
        }
        XguVertexArrayType format = gliEnumToNvType(vad->color_array_type);
        xgux_set_attrib_pointer(XGU_COLOR_ARRAY, format, vad->color_array_size, stride, staged);
        vad->color_array_dirty = GL_FALSE;
    }

    // --- Texture coordinate arrays ---
    for (GLuint i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        if (vad->texcoord_array_enabled[i] && vad->texcoord_array_buffer_binding[i] == 0 &&
            vad->texcoord_array_ptr[i] != NULL) {
            GLsizei stride =
                compute_stride(vad->texcoord_array_stride[i], vad->texcoord_array_size[i], vad->texcoord_array_type[i]);
            void *staged = stage_range(arena, ranges, &range_count, vad->texcoord_array_ptr[i], stride, vertex_count);
            if (!staged) {
                goto out_of_memory;
            }
            XguVertexArrayType format = gliEnumToNvType(vad->texcoord_array_type[i]);
            xgux_set_attrib_pointer(XGU_TEXCOORD0_ARRAY + i, format, vad->texcoord_array_size[i], stride, staged);
            vad->texcoord_array_dirty[i] = GL_FALSE;
        }
    }

    // --- Point size array ---
    if (vad->point_size_array_enabled && vad->point_size_array_buffer_binding == 0 &&
        vad->point_size_array_ptr != NULL) {
        GLsizei stride = compute_stride(vad->point_size_array_stride, 1, vad->point_size_array_type);
        void *staged = stage_range(arena, ranges, &range_count, vad->point_size_array_ptr, stride, vertex_count);
        if (!staged) {
            goto out_of_memory;
        }
        XguVertexArrayType format = gliEnumToNvType(vad->point_size_array_type);
        xgux_set_attrib_pointer(XGU_POINT_SIZE_ARRAY, format, 1, stride, staged);
        vad->point_size_array_dirty = GL_FALSE;
    }

    __asm__ __volatile__("sfence");
    if (range_count > 0) {
        uint32_t *pb = pb_begin();
        pb = pb_push1(pb, NV097_BREAK_VERTEX_BUFFER_CACHE, 0);
        pb_end(pb);
    }

    return GL_TRUE;

out_of_memory:
    gliDebugF("[gles] staging arena overflow (%u bytes). Increase GLI_STAGING_ARENA_SIZE.\n",
                    (unsigned)GLI_STAGING_ARENA_SIZE);
    gliSetError(GL_OUT_OF_MEMORY);
    return GL_FALSE;
}

// Scan an index buffer to find the maximum index value.
// This determines how many vertices we need to stage for glDrawElements.
GLsizei gliScanMaxIndex(GLenum type, const void *indices, GLsizei count)
{
    GLsizei max_idx = 0;
    if (type == GL_UNSIGNED_BYTE) {
        const uint8_t *idx = (const uint8_t *)indices;
        GLsizei i = 0;
        for (; i + 3 < count; i += 4) {
            if (idx[i] > max_idx) max_idx = idx[i];
            if (idx[i + 1] > max_idx) max_idx = idx[i + 1];
            if (idx[i + 2] > max_idx) max_idx = idx[i + 2];
            if (idx[i + 3] > max_idx) max_idx = idx[i + 3];
        }
        for (; i < count; i++) {
            if (idx[i] > max_idx) {
                max_idx = idx[i];
            }
        }
    } else if (type == GL_UNSIGNED_SHORT) {
        const uint16_t *idx = (const uint16_t *)indices;
        GLsizei i = 0;
        for (; i + 3 < count; i += 4) {
            if (idx[i] > max_idx) max_idx = idx[i];
            if (idx[i + 1] > max_idx) max_idx = idx[i + 1];
            if (idx[i + 2] > max_idx) max_idx = idx[i + 2];
            if (idx[i + 3] > max_idx) max_idx = idx[i + 3];
        }
        for (; i < count; i++) {
            if (idx[i] > max_idx) {
                max_idx = idx[i];
            }
        }
    }
#ifdef GL_OES_element_index_uint
    else if (type == GL_UNSIGNED_INT) {
        const uint32_t *idx = (const uint32_t *)indices;
        GLsizei i = 0;
        for (; i + 3 < count; i += 4) {
            if (idx[i] > max_idx) max_idx = (GLsizei)idx[i];
            if (idx[i + 1] > max_idx) max_idx = (GLsizei)idx[i + 1];
            if (idx[i + 2] > max_idx) max_idx = (GLsizei)idx[i + 2];
            if (idx[i + 3] > max_idx) max_idx = (GLsizei)idx[i + 3];
        }
        for (; i < count; i++) {
            if ((GLsizei)idx[i] > max_idx) {
                max_idx = (GLsizei)idx[i];
            }
        }
    }
#endif
    return max_idx;
}

GLboolean gliNeedsStaging(void)
{
    gli_context_t *context = gliGetContext();
    vertex_array_data_t *vad = &context->vertex_array_data;

    if (vad->vertex_array_enabled && vad->vertex_array_buffer_binding == 0 && vad->vertex_array_ptr != NULL) {
        return GL_TRUE;
    }
    if (vad->normal_array_enabled && vad->normal_array_buffer_binding == 0 && vad->normal_array_ptr != NULL) {
        return GL_TRUE;
    }
    if (vad->color_array_enabled && vad->color_array_buffer_binding == 0 && vad->color_array_ptr != NULL) {
        return GL_TRUE;
    }
    if (vad->point_size_array_enabled && vad->point_size_array_buffer_binding == 0 &&
        vad->point_size_array_ptr != NULL) {
        return GL_TRUE;
    }
    for (int i = 0; i < GLI_MAX_TEXTURE_UNITS; i++) {
        if (vad->texcoord_array_enabled[i] && vad->texcoord_array_buffer_binding[i] == 0 &&
            vad->texcoord_array_ptr[i] != NULL) {
            return GL_TRUE;
        }
    }

    return GL_FALSE;
}
