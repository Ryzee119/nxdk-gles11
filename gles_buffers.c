#include "gles_private.h"

static buffer_object_t *find_buffer_object(GLuint name, buffer_object_t **prev)
{
    gli_context_t *context = gliGetContext();
    buffer_object_t *g_buffers_head = context->buffer_objects;
    if (prev) {
        *prev = NULL;
    }

    if (name == 0) {
        return NULL;
    }

    for (buffer_object_t *it = g_buffers_head; it != NULL; it = it->next) {
        if (it->buffer_name == name) {
            return it;
        }
        if (prev) {
            *prev = it;
        }
    }
    return NULL;
}

static GLuint *get_binding_ptr(GLenum target)
{
    gli_context_t *context = gliGetContext();
    switch (target) {
        case GL_ARRAY_BUFFER:
            return &context->vertex_array_data.array_buffer_binding;
        case GL_ELEMENT_ARRAY_BUFFER:
            return &context->vertex_array_data.element_array_buffer_binding;
        default:
            return NULL;
    }
}

buffer_object_t *gliGetBufferObject(GLuint name)
{
    return find_buffer_object(name, NULL);
}

GL_API void GL_APIENTRY glGenBuffers(GLsizei n, GLuint *buffers)
{
    if (buffers == NULL) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }
    static GLuint next = 1;
    for (GLsizei i = 0; i < n; i++) {
        // Surely we will never wrap around, but skip zero if we so
        // Fixme, wrap around could cause duplicate names
        if (next == 0) {
            next++;
        }
        buffers[i] = next++;
    }
}

GL_API void GL_APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
    gli_context_t *context = gliGetContext();

    GLuint *binding = get_binding_ptr(target);
    if (binding == NULL) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    // A buffer value of zero unbinds any buffer currently bound to the target
    if (buffer == 0) {
        *binding = 0;
        return;
    }

    // FIXME, check for valid buffer name?

    // First check if the buffer object already exists, if so just bind it
    buffer_object_t *buffer_object = find_buffer_object(buffer, NULL);
    if (buffer_object != NULL) {
        *binding = buffer;
        return;
    }

    // It's a new buffer object, so we create it
    buffer_object = GLI_MALLOC(sizeof(buffer_object_t));
    if (buffer_object == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    memset(buffer_object, 0, sizeof(buffer_object_t));
    buffer_object->buffer_name = buffer;
    buffer_object->buffer_size = 0;
    buffer_object->buffer_usage = GL_STATIC_DRAW;

    // Bind and add to the context's list
    *binding = buffer;
    buffer_object->next = context->buffer_objects;
    context->buffer_objects = buffer_object;
}

GL_API void GL_APIENTRY glDeleteBuffers(GLsizei n, const GLuint *buffers)
{
    gli_context_t *context = gliGetContext();
    if (n < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    for (GLsizei i = 0; i < n; i++) {
        GLuint name = buffers[i];
        buffer_object_t *prev = NULL; // Track previous for linked list removal
        buffer_object_t *buf = find_buffer_object(name, &prev);
        if (buf) {
            // Remove from the context's list
            if (prev) {
                prev->next = buf->next;
            } else {
                context->buffer_objects = buf->next;
            }

            // If a buffer object is deleted while it is bound, all bindings to that object in the current context are
            // reset to zero
            if (context->vertex_array_data.array_buffer_binding == name) {
                context->vertex_array_data.array_buffer_binding = 0;
            }

            if (context->vertex_array_data.element_array_buffer_binding == name) {
                context->vertex_array_data.element_array_buffer_binding = 0;
            }

            // Also unbind from any vertex attributes using this buffer
            if (context->vertex_array_data.vertex_array_buffer_binding == name) {
                context->vertex_array_data.vertex_array_buffer_binding = 0;
                context->vertex_array_data.vertex_array_ptr = NULL;
                context->vertex_array_data.vertex_array_dirty = GL_TRUE;
            }

            if (context->vertex_array_data.normal_array_buffer_binding == name) {
                context->vertex_array_data.normal_array_buffer_binding = 0;
                context->vertex_array_data.normal_array_ptr = NULL;
                context->vertex_array_data.normal_array_dirty = GL_TRUE;
            }

            if (context->vertex_array_data.color_array_buffer_binding == name) {
                context->vertex_array_data.color_array_buffer_binding = 0;
                context->vertex_array_data.color_array_ptr = NULL;
                context->vertex_array_data.color_array_dirty = GL_TRUE;
            }

            if (context->vertex_array_data.point_size_array_buffer_binding == name) {
                context->vertex_array_data.point_size_array_buffer_binding = 0;
                context->vertex_array_data.point_size_array_ptr = NULL;
                context->vertex_array_data.point_size_array_dirty = GL_TRUE;
            }

            for (int u = 0; u < GLI_MAX_TEXTURE_UNITS; ++u) {
                if (context->vertex_array_data.texcoord_array_buffer_binding[u] == name) {
                    context->vertex_array_data.texcoord_array_buffer_binding[u] = 0;
                    context->vertex_array_data.texcoord_array_ptr[u] = NULL;
                    context->vertex_array_data.texcoord_array_dirty[u] = GL_TRUE;
                }
            }

            if (buf->buffer_data) {
                MmFreeContiguousMemory(buf->buffer_data);
            }
            GLI_FREE(buf);
        }
    }
}

GL_API void GL_APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    gli_context_t *context = gliGetContext();

    GLuint *binding = get_binding_ptr(target);
    if (binding == NULL) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    buffer_object_t *buffer_object = find_buffer_object(*binding, NULL);
    if (buffer_object == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (usage != GL_STATIC_DRAW && usage != GL_DYNAMIC_DRAW) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (size < 0) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    // Any pre-existing data store is deleted
    if (buffer_object->buffer_data) {
        MmFreeContiguousMemory(buffer_object->buffer_data);
        buffer_object->buffer_data = NULL;
    }

    // Data size of zero is valid, but we dont need to allocate memory. We are done.
    if (size == 0) {
        buffer_object->buffer_size = (GLuint)size;
        buffer_object->buffer_usage = usage;
        return;
    }

    // We have shared VRAM, don't really need to care about usage hints (GL_STATIC_DRAW / GL_DYNAMIC_DRAW) for now
    void *gpu_data = MmAllocateContiguousMemoryEx(size, 0, 0xFFFFFFFF, 0x1000, PAGE_READWRITE | PAGE_WRITECOMBINE);
    if (gpu_data == NULL) {
        gliSetError(GL_OUT_OF_MEMORY);
        return;
    }

    buffer_object->buffer_size = (GLuint)size;
    buffer_object->buffer_usage = usage;
    buffer_object->buffer_data = gpu_data;

    // If data is NULL, a data store of the specified size is still created, but its contents remain uninitialized and
    // thus undefined.
    if (data) {
        memcpy(buffer_object->buffer_data, data, size);
    }
}

GL_API void GL_APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
{
    gli_context_t *context = gliGetContext();
    if (data == NULL) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    GLuint *binding = get_binding_ptr(target);
    if (binding == NULL) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    buffer_object_t *buffer_object = find_buffer_object(*binding, NULL);
    if (buffer_object == NULL || buffer_object->buffer_data == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    if (offset < 0 || size < 0 || (GLuint)offset > buffer_object->buffer_size ||
        (GLuint)((GLuint)offset + (GLuint)size) > buffer_object->buffer_size) {
        gliSetError(GL_INVALID_VALUE);
        return;
    }

    memcpy((uint8_t *)buffer_object->buffer_data + offset, data, size);
}

GL_API void GL_APIENTRY glGetBufferParameteriv(GLenum target, GLenum pname, GLint *params)
{
    gli_context_t *context = gliGetContext();

    GLuint *binding = get_binding_ptr(target);
    if (binding == NULL) {
        gliSetError(GL_INVALID_ENUM);
        return;
    }

    if (params == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    buffer_object_t *buffer_object = find_buffer_object(*binding, NULL);
    if (buffer_object == NULL) {
        gliSetError(GL_INVALID_OPERATION);
        return;
    }

    switch (pname) {
        case GL_BUFFER_SIZE:
            *params = (GLint)buffer_object->buffer_size;
            break;
        case GL_BUFFER_USAGE:
            *params = (GLint)buffer_object->buffer_usage;
            break;
        default:
            gliSetError(GL_INVALID_ENUM);
            return;
    }
}

GL_API GLboolean GL_APIENTRY glIsBuffer(GLuint buffer)
{
    gli_context_t *context = gliGetContext();
    buffer_object_t *buffer_object = find_buffer_object(buffer, NULL);
    if (buffer == 0 || buffer_object == NULL) {
        return GL_FALSE;
    }
    return GL_TRUE;
}
