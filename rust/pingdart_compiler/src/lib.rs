// rust/pingdart_compiler/src/lib.rs
use std::panic::{catch_unwind, AssertUnwindSafe};
use std::ptr;
use std::slice;

pub const PD_COMPILER_ABI_VERSION: u32 = 1;

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Debug, Copy, Clone, PartialEq, Eq)]
pub enum pd_status {
    PD_OK = 0,
    PD_INVALID_ARGUMENT = 1,
    PD_OUT_OF_MEMORY = 2,
    PD_COMPILER_ERROR = 3,
    PD_INTERNAL_ERROR = 4,
}

#[allow(non_camel_case_types)]
#[repr(C)]
pub struct pd_buffer {
    pub data: *mut u8,
    pub length: usize,
}

#[allow(non_camel_case_types)]
pub struct pd_compiler_ctx {
    pub compilation_count: usize,
}

impl pd_compiler_ctx {
    fn new() -> Self {
        Self { compilation_count: 0 }
    }
}

#[no_mangle]
pub extern "C" fn pd_compiler_get_abi_version() -> u32 {
    PD_COMPILER_ABI_VERSION
}

#[no_mangle]
pub unsafe extern "C" fn pd_compiler_create(out_context: *mut *mut pd_compiler_ctx) -> pd_status {
    if out_context.is_null() {
        return pd_status::PD_INVALID_ARGUMENT;
    }

    let result = catch_unwind(AssertUnwindSafe(|| {
        let ctx = Box::new(pd_compiler_ctx::new());
        *out_context = Box::into_raw(ctx);
        pd_status::PD_OK
    }));

    match result {
        Ok(status) => status,
        Err(_) => {
            *out_context = ptr::null_mut();
            pd_status::PD_INTERNAL_ERROR
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn pd_compiler_compile(
    context: *mut pd_compiler_ctx,
    source: *const u8,
    source_length: usize,
    out_buffer: *mut pd_buffer,
) -> pd_status {
    if context.is_null() || out_buffer.is_null() {
        return pd_status::PD_INVALID_ARGUMENT;
    }

    if source.is_null() && source_length > 0 {
        return pd_status::PD_INVALID_ARGUMENT;
    }

    // Initialize out_buffer to empty
    (*out_buffer).data = ptr::null_mut();
    (*out_buffer).length = 0;

    let result = catch_unwind(AssertUnwindSafe(|| {
        let src_slice = if source_length == 0 {
            &[]
        } else {
            slice::from_raw_parts(source, source_length)
        };

        // Check for intentional test panic
        if src_slice.starts_with(b"trigger_panic") {
            panic!("Intentional FFI panic containment test");
        }

        // Empty source or invalid syntax
        if src_slice.is_empty() || src_slice.starts_with(b"syntax_error") {
            return pd_status::PD_COMPILER_ERROR;
        }

        // Generate compiled bytecode header + payload
        let mut output = Vec::with_capacity(8 + source_length);
        output.extend_from_slice(b"PDC1\x00\x00\x00\x01"); // Magic + version
        output.extend_from_slice(src_slice);

        let mut boxed_slice = output.into_boxed_slice();
        (*out_buffer).data = boxed_slice.as_mut_ptr();
        (*out_buffer).length = boxed_slice.len();
        std::mem::forget(boxed_slice);

        (*context).compilation_count += 1;
        pd_status::PD_OK
    }));

    match result {
        Ok(status) => status,
        Err(_) => {
            (*out_buffer).data = ptr::null_mut();
            (*out_buffer).length = 0;
            pd_status::PD_INTERNAL_ERROR
        }
    }
}

#[no_mangle]
pub unsafe extern "C" fn pd_compiler_free_buffer(buffer: *mut pd_buffer) {
    if buffer.is_null() {
        return;
    }

    let _ = catch_unwind(AssertUnwindSafe(|| {
        if !(*buffer).data.is_null() && (*buffer).length > 0 {
            let slice_ptr = ptr::slice_from_raw_parts_mut((*buffer).data, (*buffer).length);
            let _ = Box::from_raw(slice_ptr);
            (*buffer).data = ptr::null_mut();
            (*buffer).length = 0;
        }
    }));
}

#[no_mangle]
pub unsafe extern "C" fn pd_compiler_destroy(context: *mut pd_compiler_ctx) {
    if context.is_null() {
        return;
    }

    let _ = catch_unwind(AssertUnwindSafe(|| {
        let _ = Box::from_raw(context);
    }));
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_abi_version() {
        assert_eq!(pd_compiler_get_abi_version(), 1);
    }

    #[test]
    fn test_create_and_destroy() {
        let mut ctx: *mut pd_compiler_ctx = ptr::null_mut();
        unsafe {
            let status = pd_compiler_create(&mut ctx);
            assert_eq!(status, pd_status::PD_OK);
            assert!(!ctx.is_null());
            pd_compiler_destroy(ctx);
        }
    }

    #[test]
    fn test_compile_and_free() {
        let mut ctx: *mut pd_compiler_ctx = ptr::null_mut();
        unsafe {
            assert_eq!(pd_compiler_create(&mut ctx), pd_status::PD_OK);
            let source = b"widget Main() { return Container(); }";
            let mut buf = pd_buffer { data: ptr::null_mut(), length: 0 };
            let status = pd_compiler_compile(ctx, source.as_ptr(), source.len(), &mut buf);
            assert_eq!(status, pd_status::PD_OK);
            assert!(!buf.data.is_null());
            assert!(buf.length > 0);

            pd_compiler_free_buffer(&mut buf);
            assert!(buf.data.is_null());
            assert_eq!(buf.length, 0);

            pd_compiler_destroy(ctx);
        }
    }

    #[test]
    fn test_panic_containment() {
        let mut ctx: *mut pd_compiler_ctx = ptr::null_mut();
        unsafe {
            assert_eq!(pd_compiler_create(&mut ctx), pd_status::PD_OK);
            let source = b"trigger_panic";
            let mut buf = pd_buffer { data: ptr::null_mut(), length: 0 };
            let status = pd_compiler_compile(ctx, source.as_ptr(), source.len(), &mut buf);
            assert_eq!(status, pd_status::PD_INTERNAL_ERROR);
            assert!(buf.data.is_null());
            pd_compiler_destroy(ctx);
        }
    }
}
