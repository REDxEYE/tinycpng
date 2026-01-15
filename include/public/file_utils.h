// Created by RED on 09.12.2025.
#pragma once

#include <stdlib.h>
#include <string.h>

#include "mytypes.h"
#include "error_utils.h"


typedef void *UserFile;
typedef struct UserIO UserIO;

// Read/write function typedefs
typedef u32 (*ReadFunc)(UserIO *user_io, void *out, u32 size);

typedef u32 (*WriteFunc)(UserIO *user_io, const void *in, u32 size);

struct UserIO {
    ReadFunc read_func;
    WriteFunc write_func;
    UserFile user_file;
};


Error read_u32le(UserIO *user_io, u32 *out);

Error read_u32be(UserIO *user_io, u32 *out);

Error read_u64le(UserIO *user_io, u64 *out);

Error read_bytes(UserIO *user_io, void *out, u32 size);

Error write_u32be(UserIO *user_io, u32 value);

Error write_u32le(UserIO *user_io, u32 value);

Error write_u64le(UserIO *user_io, u64 value);

Error write_bytes(UserIO *user_io, const void *in, u32 size);


// Example memory file implementation
typedef struct {
    u8 *data;
    size_t size;
    size_t capacity;
    size_t position;
} MemoryFile;

void MemoryFile_free(MemoryFile *mem_file);

u32 memory_file_read(UserIO *io, void *out, u32 out_size);

u32 memory_file_write(UserIO *io, const void *in, u32 in_size);


// Example native file implementation
// Use FILE* as user_file value

u32 native_file_read(UserIO *io, void *out, u32 out_size);

u32 native_file_write(UserIO *io, const void *in, u32 in_size);
