// Created by RED on 16.12.2025.
#include "file_utils.h"

#include <stdio.h>


Error read_u32le(UserIO *user_io, u32 *out) {
    if (user_io->read_func == NULL) {
        return error_fatal("UserIO read function is NULL");
    }
    u32 tmp = 0;
    const u32 read = user_io->read_func(user_io, &tmp, sizeof(u32));
    if (read != sizeof(u32)) {
        return error_fatal("Failed to read u32 from UserIO");
    }
    *out = le32_to_host(tmp);
    return NO_ERROR;
}

Error read_u32be(UserIO *user_io, u32 *out) {
    if (user_io->read_func == NULL) {
        return error_fatal("UserIO read function is NULL");
    }
    u32 tmp = 0;
    const u32 read = user_io->read_func(user_io, &tmp, sizeof(u32));
    if (read != sizeof(u32)) {
        return error_fatal("Failed to read u32 from UserIO");
    }
    *out = be32_to_host(tmp);
    return NO_ERROR;
}

Error read_u64le(UserIO *user_io, u64 *out) {
    if (user_io->read_func == NULL) {
        return error_fatal("UserIO read function is NULL");
    }
    u64 tmp = 0;
    const u32 read = user_io->read_func(user_io, &tmp, sizeof(u64));
    if (read != sizeof(u64)) {
        return error_fatal("Failed to read u64 from UserIO");
    }
    *out = le64_to_host(tmp);
    return NO_ERROR;
}

Error read_bytes(UserIO *user_io, void *out, u32 size) {
    if (user_io->read_func == NULL) {
        return error_fatal("UserIO read function is NULL");
    }
    const u32 read = user_io->read_func(user_io, out, size);
    if (read != size) {
        return error_fatal("Failed to read bytes from UserIO");
    }
    return NO_ERROR;
}

Error write_u32be(UserIO *user_io, const u32 value) {
    if (user_io->write_func == NULL) {
        return error_fatal("UserIO write function is NULL");
    }
    const u32 tmp = host_to_be32(value);
    const u32 written = user_io->write_func(user_io, &tmp, sizeof(u32));
    if (written != sizeof(u32)) {
        return error_fatal("Failed to write u32 to UserIO");
    }
    return NO_ERROR;
}

Error write_u32le(UserIO *user_io, const u32 value) {
    if (user_io->write_func == NULL) {
        return error_fatal("UserIO write function is NULL");
    }
    const u32 tmp = host_to_le32(value);
    const u32 written = user_io->write_func(user_io, &tmp, sizeof(u32));
    if (written != sizeof(u32)) {
        return error_fatal("Failed to write u32 to UserIO");
    }
    return NO_ERROR;
}

Error write_u64le(UserIO *user_io, const u64 value) {
    if (user_io->write_func == NULL) {
        return error_fatal("UserIO write function is NULL");
    }
    const u64 tmp = host_to_le64(value);
    const u32 written = user_io->write_func(user_io, &tmp, sizeof(u64));
    if (written != sizeof(u64)) {
        return error_fatal("Failed to write u64 to UserIO");
    }
    return NO_ERROR;
}

Error write_bytes(UserIO *user_io, const void *in, u32 size) {
    if (user_io->write_func == NULL) {
        return error_fatal("UserIO write function is NULL");
    }
    const u32 written = user_io->write_func(user_io, in, size);
    if (written != size) {
        return error_fatal("Failed to write bytes to UserIO");
    }
    return NO_ERROR;
}

void MemoryFile_free(MemoryFile *mem_file) {
    if (mem_file->data) {
        free(mem_file->data);
        mem_file->data = NULL;
    }
    mem_file->size = 0;
    mem_file->capacity = 0;
    mem_file->position = 0;
}

u32 memory_file_read(UserIO *io, void *out, u32 out_size) {
    MemoryFile *mem_file = (MemoryFile *) io->user_file;
    u32 to_read = out_size;
    if (mem_file->position + to_read > mem_file->size) {
        to_read = mem_file->size - mem_file->position;
    }
    memcpy(out, mem_file->data + mem_file->position, to_read);
    mem_file->position += to_read;
    return to_read;
}

u32 memory_file_write(UserIO *io, const void *in, u32 in_size) {
    MemoryFile *mem_file = (MemoryFile *) io->user_file;
    if (mem_file->position + in_size > mem_file->capacity) {
        u32 new_capacity = (mem_file->position + in_size) * 2;
        u8 *new_data = realloc(mem_file->data, new_capacity);
        if (!new_data) {
            return 0; // Failed to allocate memory
        }
        mem_file->data = new_data;
        mem_file->capacity = new_capacity;
    }
    memcpy(mem_file->data + mem_file->position, in, in_size);
    mem_file->position += in_size;
    if (mem_file->position > mem_file->size) {
        mem_file->size = mem_file->position;
    }
    return in_size;
}

u32 native_file_read(UserIO *io, void *out, u32 out_size) {
    FILE *file = (FILE *) io->user_file;
    return (u32) fread(out, 1, out_size, file);
}

u32 native_file_write(UserIO *io, const void *in, u32 in_size) {
    FILE *file = (FILE *) io->user_file;
    return (u32) fwrite(in, 1, in_size, file);
}
