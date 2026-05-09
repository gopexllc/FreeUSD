//! Minimal safe wrappers over the FreeUSD C ABI (`freeusd.h`).
//!
//! Build the C++ project first so `build/src/libfreeusd_*.a` exist, then:
//! `cargo test --manifest-path bindings/rust/Cargo.toml`

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]

use std::ffi::{c_char, c_double, c_int, CStr};
use std::ptr;

#[repr(C)]
pub struct FreeusdLayer {
    _private: [u8; 0],
}

#[repr(C)]
pub struct FreeusdStage {
    _private: [u8; 0],
}

extern "C" {
    fn freeusd_version_string() -> *const c_char;
    fn freeusd_last_error_message() -> *const c_char;

    fn freeusd_layer_new_anonymous(identifier: *const c_char) -> *mut FreeusdLayer;
    fn freeusd_layer_free(layer: *mut FreeusdLayer);
    fn freeusd_layer_load_usda_from_string(layer: *mut FreeusdLayer, text: *const c_char) -> c_int;

    fn freeusd_stage_attach_root_layer(layer: *const FreeusdLayer) -> *mut FreeusdStage;
    fn freeusd_stage_free(stage: *mut FreeusdStage);
    fn freeusd_stage_read_field_double(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out: *mut c_double,
    ) -> c_int;
}

/// Project version string (borrowed from C static storage).
pub fn version_string() -> &'static str {
    unsafe {
        let p = freeusd_version_string();
        CStr::from_ptr(p).to_str().unwrap_or("")
    }
}

/// Thread-local last error from the C API.
pub fn last_error_message() -> &'static str {
    unsafe {
        let p = freeusd_last_error_message();
        CStr::from_ptr(p).to_str().unwrap_or("")
    }
}

pub struct Layer {
    ptr: *mut FreeusdLayer,
}

impl Drop for Layer {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            unsafe { freeusd_layer_free(self.ptr) };
            self.ptr = ptr::null_mut();
        }
    }
}

impl Layer {
    pub fn new_anonymous(identifier: Option<&str>) -> Option<Self> {
        let c = identifier.map(|s| std::ffi::CString::new(s).ok()).flatten();
        let p = unsafe {
            freeusd_layer_new_anonymous(match &c {
                Some(s) => s.as_ptr(),
                None => ptr::null(),
            })
        };
        if p.is_null() {
            None
        } else {
            Some(Self { ptr: p })
        }
    }

    pub fn load_usda(&mut self, text: &str) -> i32 {
        let cs = match std::ffi::CString::new(text) {
            Ok(s) => s,
            Err(_) => return 1,
        };
        unsafe { freeusd_layer_load_usda_from_string(self.ptr, cs.as_ptr()) }
    }
}

pub struct Stage {
    ptr: *mut FreeusdStage,
}

impl Drop for Stage {
    fn drop(&mut self) {
        if !self.ptr.is_null() {
            unsafe { freeusd_stage_free(self.ptr) };
            self.ptr = ptr::null_mut();
        }
    }
}

impl Stage {
    pub fn attach_root_layer(layer: &Layer) -> Option<Self> {
        let p = unsafe { freeusd_stage_attach_root_layer(layer.ptr as *const FreeusdLayer) };
        if p.is_null() {
            None
        } else {
            Some(Self { ptr: p })
        }
    }

    pub fn read_field_double(&self, prim_path: &str, attr: &str, time: f64) -> Result<f64, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut out: c_double = 0.0;
        let rc = unsafe {
            freeusd_stage_read_field_double(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut out,
            )
        };
        if rc != 0 {
            Err(rc as i32)
        } else {
            Ok(out as f64)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn version_non_empty() {
        assert!(!version_string().is_empty());
    }

    #[test]
    fn read_double_smoke() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        double x = 3.0
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_smoke")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        let v = stage
            .read_field_double("/W/C", "x", 1.0)
            .expect(last_error_message());
        assert!((v - 3.0).abs() < 1e-9);
    }
}
