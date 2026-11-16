#ifndef PD_COMPILER_ABI_H
#define PD_COMPILER_ABI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PD_COMPILER_ABI_VERSION 1

typedef struct pd_compiler_ctx pd_compiler_ctx_t;

typedef enum pd_status {
    PD_OK = 0,
    PD_INVALID_ARGUMENT = 1,
    PD_OUT_OF_MEMORY = 2,
    PD_COMPILER_ERROR = 3,
    PD_INTERNAL_ERROR = 4
} pd_status_t;

typedef struct pd_buffer {
    uint8_t* data;
    size_t length;
} pd_buffer_t;

uint32_t pd_compiler_get_abi_version(void);

pd_status_t pd_compiler_create(
    pd_compiler_ctx_t** out_context
);

pd_status_t pd_compiler_compile(
    pd_compiler_ctx_t* context,
    const uint8_t* source,
    size_t source_length,
    pd_buffer_t* out_buffer
);

void pd_compiler_free_buffer(
    pd_buffer_t* buffer
);

void pd_compiler_destroy(
    pd_compiler_ctx_t* context
);

#ifdef __cplusplus
}
#endif

#endif // PD_COMPILER_ABI_H
