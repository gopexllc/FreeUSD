//! Minimal safe wrappers over the FreeUSD C ABI (`freeusd.h`).
//!
//! Build the C++ project first so `build/src/libfreeusd_*.a` exist, then:
//! `cargo test --manifest-path bindings/rust/Cargo.toml`

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]

use std::ffi::{c_char, c_double, c_float, c_int, CString, CStr};
use std::ptr;

#[repr(C)]
pub struct FreeusdUsdcTocSection {
    pub name: [c_char; 16],
    pub start_byte_offset: i64,
    pub size_bytes: i64,
}

const _: () = assert!(std::mem::size_of::<FreeusdUsdcTocSection>() == 32);

#[repr(C)]
pub struct FreeusdUsdcBootstrap {
    pub file_version_major: u8,
    pub file_version_minor: u8,
    pub file_version_patch: u8,
    pub reserved: [u8; 5],
    pub toc_byte_offset: i64,
}

const _: () = assert!(std::mem::size_of::<FreeusdUsdcBootstrap>() == 16);
const _: () = assert!(std::mem::offset_of!(FreeusdUsdcBootstrap, toc_byte_offset) == 8);

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
    fn freeusd_usdc_crate_identifier_utf8() -> *const c_char;
    fn freeusd_detect_usd_file_kind_from_path_utf8(
        path: *const c_char,
        out_kind: *mut c_int,
        out_detail: *mut *mut c_char,
    ) -> c_int;
    fn freeusd_read_usdc_bootstrap_from_path_utf8(path: *const c_char, out: *mut FreeusdUsdcBootstrap) -> c_int;
    fn freeusd_read_usdc_toc_from_path_utf8(
        path: *const c_char,
        max_sections: u64,
        out_total: *mut u64,
        out_sections: *mut *mut FreeusdUsdcTocSection,
        out_returned: *mut u64,
    ) -> c_int;
    fn freeusd_usdc_toc_sections_free(sections: *mut FreeusdUsdcTocSection);
    fn freeusd_read_usdc_section_bytes_from_path_utf8(
        path: *const c_char,
        section_name: *const c_char,
        max_bytes: u64,
        out_bytes: *mut *mut u8,
        out_size: *mut u64,
    ) -> c_int;
    fn freeusd_bytes_free(bytes: *mut std::ffi::c_void);
    fn freeusd_last_error_message() -> *const c_char;

    fn freeusd_layer_new_anonymous(identifier: *const c_char) -> *mut FreeusdLayer;
    fn freeusd_layer_free(layer: *mut FreeusdLayer);
    fn freeusd_layer_load_usda_from_string(layer: *mut FreeusdLayer, text: *const c_char) -> c_int;

    fn freeusd_stage_attach_root_layer(layer: *const FreeusdLayer) -> *mut FreeusdStage;
    fn freeusd_stage_open_from_root_file_utf8(path: *const c_char, sublayer_policy: c_int) -> *mut FreeusdStage;
    fn freeusd_stage_free(stage: *mut FreeusdStage);
    fn freeusd_stage_prim_is_valid(stage: *const FreeusdStage, prim_path: *const c_char) -> c_int;
    fn freeusd_stage_resolve_prim_specifier_kind(stage: *const FreeusdStage, prim_path: *const c_char) -> c_int;
    fn freeusd_stage_relocate_source_in_any_layer(stage: *const FreeusdStage, from_prim: *const c_char) -> c_int;
    fn freeusd_stage_get_composed_relocate_target_utf8(
        stage: *const FreeusdStage,
        from_prim: *const c_char,
        out_target: *mut *mut c_char,
    ) -> c_int;
    fn freeusd_stage_list_composed_relocate_pairs_utf8(
        stage: *const FreeusdStage,
        out_strings: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
    fn freeusd_stage_prefix_substitution_key_in_any_layer(stage: *const FreeusdStage, from_prefix: *const c_char)
        -> c_int;
    fn freeusd_stage_get_composed_prefix_substitution_utf8(
        stage: *const FreeusdStage,
        from_prefix: *const c_char,
        out_to: *mut *mut c_char,
    ) -> c_int;
    fn freeusd_stage_list_composed_prefix_substitution_pairs_utf8(
        stage: *const FreeusdStage,
        out_strings: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
    fn freeusd_stage_get_composed_custom_layer_data_utf8(
        stage: *const FreeusdStage,
        key_utf8: *const c_char,
        out_value: *mut *mut c_char,
    ) -> c_int;
    fn freeusd_stage_custom_layer_data_key_in_any_layer(stage: *const FreeusdStage, key_utf8: *const c_char) -> c_int;
    fn freeusd_stage_list_composed_custom_layer_data_keys(
        stage: *const FreeusdStage,
        out_keys: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
    fn freeusd_stage_get_composed_prim_variant_selection_utf8(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        variant_set: *const c_char,
        out_selected: *mut *mut c_char,
    ) -> c_int;
    fn freeusd_stage_prim_variant_selection_set_in_any_layer(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        variant_set: *const c_char,
    ) -> c_int;
    fn freeusd_stage_list_composed_prim_variant_selection_sets_utf8(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        out_strings: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
    fn freeusd_stage_list_composed_prim_variant_set_names_utf8(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        out_strings: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
    fn freeusd_stage_prim_variant_set_declared_in_any_layer(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        variant_set_name: *const c_char,
    ) -> c_int;
    fn freeusd_stage_list_composed_prim_variant_names_utf8(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        variant_set_name: *const c_char,
        out_strings: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
    fn freeusd_stage_get_start_time_code(stage: *const FreeusdStage, out_value: *mut c_double, out_has: *mut c_int)
        -> c_int;
    fn freeusd_stage_get_end_time_code(stage: *const FreeusdStage, out_value: *mut c_double, out_has: *mut c_int) -> c_int;
    fn freeusd_stage_get_time_codes_per_second(
        stage: *const FreeusdStage,
        out_value: *mut c_double,
        out_has: *mut c_int,
    ) -> c_int;
    fn freeusd_stage_get_frames_per_second(stage: *const FreeusdStage, out_value: *mut c_double, out_has: *mut c_int)
        -> c_int;
    fn freeusd_stage_get_frame_precision(stage: *const FreeusdStage, out_value: *mut i64, out_has: *mut c_int) -> c_int;
    fn freeusd_stage_get_meters_per_unit(stage: *const FreeusdStage, out_value: *mut c_double, out_has: *mut c_int)
        -> c_int;
    fn freeusd_stage_get_up_axis_utf8(stage: *const FreeusdStage, out_axis: *mut *mut c_char) -> c_int;
    fn freeusd_stage_list_prim_order_paths_utf8(
        stage: *const FreeusdStage,
        out_paths: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
    fn freeusd_string_free(s: *mut c_char);
    fn freeusd_path_list_free(paths: *mut *mut c_char, count: usize);
    fn freeusd_stage_read_field_double(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out: *mut c_double,
    ) -> c_int;
    fn freeusd_stage_read_field_float(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out: *mut c_float,
    ) -> c_int;
    fn freeusd_stage_read_field_bool(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out: *mut c_int,
    ) -> c_int;
    fn freeusd_stage_read_field_int64(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out: *mut i64,
    ) -> c_int;
    fn freeusd_stage_read_field_string(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_string: *mut *mut c_char,
    ) -> c_int;
    fn freeusd_stage_read_field_vec3d(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_x: *mut c_double,
        out_y: *mut c_double,
        out_z: *mut c_double,
    ) -> c_int;
    fn freeusd_stage_read_field_vec3f(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_x: *mut c_float,
        out_y: *mut c_float,
        out_z: *mut c_float,
    ) -> c_int;
    fn freeusd_stage_read_field_matrix4d(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_row_major: *mut c_double,
    ) -> c_int;
    fn freeusd_stage_read_field_quatd(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_real: *mut c_double,
        out_i: *mut c_double,
        out_j: *mut c_double,
        out_k: *mut c_double,
    ) -> c_int;
    fn freeusd_stage_read_field_quatf(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_real: *mut c_float,
        out_i: *mut c_float,
        out_j: *mut c_float,
        out_k: *mut c_float,
    ) -> c_int;
    fn freeusd_stage_read_field_token(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_token: *mut *mut c_char,
    ) -> c_int;
    fn freeusd_stage_read_field_token_array(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out_strings: *mut *mut *mut c_char,
        out_count: *mut usize,
    ) -> c_int;
}

/// C API `FREEUSD_ERR_NOT_FOUND` (e.g. unmapped relocate / prefix substitution / customLayerData string read).
pub const ERR_NOT_FOUND: i32 = 3;

/// Matches C `FreeusdPrimSpecifierKind` / `freeusd::sdf::Layer::PrimSpecifierKind` order.
#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PrimSpecifierKind {
    Default = 0,
    Def = 1,
    Class = 2,
    Over = 3,
}

/// Project version string (borrowed from C static storage).
pub fn version_string() -> &'static str {
    unsafe {
        let p = freeusd_version_string();
        CStr::from_ptr(p).to_str().unwrap_or("")
    }
}

/// USDC crate file magic prefix (`PXR-USDC`; borrowed from C static storage).
pub fn usdc_crate_identifier() -> &'static str {
    unsafe {
        let p = freeusd_usdc_crate_identifier_utf8();
        CStr::from_ptr(p).to_str().unwrap_or("")
    }
}

/// Sniff-only USD container kind (matches C `FreeusdUsdFileKind`).
#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum UsdFileKind {
    IoOrEmpty = 0,
    UsdaAscii = 1,
    UsdcCrate = 2,
    Unknown = 3,
}

/// Reads the first bytes of `path` (no full USDC decode). Returns `(kind, optional_detail)` or a non-zero C API code.
pub fn detect_usd_file_kind_from_path(path: &str) -> Result<(UsdFileKind, Option<String>), i32> {
    let c = CString::new(path).map_err(|_| 1i32)?;
    let mut kind: c_int = 0;
    let mut detail: *mut c_char = ptr::null_mut();
    let rc = unsafe {
        freeusd_detect_usd_file_kind_from_path_utf8(c.as_ptr(), &mut kind, &mut detail)
    };
    if rc != 0 {
        return Err(rc);
    }
    let k = match kind {
        0 => UsdFileKind::IoOrEmpty,
        1 => UsdFileKind::UsdaAscii,
        2 => UsdFileKind::UsdcCrate,
        _ => UsdFileKind::Unknown,
    };
    let detail_opt = if detail.is_null() {
        None
    } else {
        let s = unsafe { CStr::from_ptr(detail) }
            .to_string_lossy()
            .into_owned();
        unsafe { freeusd_string_free(detail) };
        Some(s)
    };
    Ok((k, detail_opt))
}

/// Decoded USDC bootstrap (matches C `FreeusdUsdcBootstrap` layout).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct UsdcBootstrap {
    pub file_version_major: u8,
    pub file_version_minor: u8,
    pub file_version_patch: u8,
    pub toc_byte_offset: i64,
}

/// Reads the 88-byte USDC bootstrap from `path`.
pub fn read_usdc_bootstrap_from_path(path: &str) -> Result<UsdcBootstrap, i32> {
    let c = CString::new(path).map_err(|_| 1i32)?;
    let mut raw = FreeusdUsdcBootstrap {
        file_version_major: 0,
        file_version_minor: 0,
        file_version_patch: 0,
        reserved: [0; 5],
        toc_byte_offset: 0,
    };
    let rc = unsafe { freeusd_read_usdc_bootstrap_from_path_utf8(c.as_ptr(), &mut raw) };
    if rc != 0 {
        return Err(rc);
    }
    Ok(UsdcBootstrap {
        file_version_major: raw.file_version_major,
        file_version_minor: raw.file_version_minor,
        file_version_patch: raw.file_version_patch,
        toc_byte_offset: raw.toc_byte_offset,
    })
}

/// One TOC section (name + byte range in the crate file).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct UsdcTocSection {
    pub name: String,
    pub start_byte_offset: i64,
    pub size_bytes: i64,
}

fn toc_section_from_raw(raw: &FreeusdUsdcTocSection) -> UsdcTocSection {
    let bytes: &[u8] =
        unsafe { std::slice::from_raw_parts(raw.name.as_ptr() as *const u8, raw.name.len()) };
    let end = bytes.iter().position(|&b| b == 0).unwrap_or(bytes.len());
    let name = String::from_utf8_lossy(&bytes[..end]).into_owned();
    UsdcTocSection {
        name,
        start_byte_offset: raw.start_byte_offset,
        size_bytes: raw.size_bytes,
    }
}

/// Reads the USDC table of contents (section names and file ranges). `max_sections` caps how many
/// sections the file may declare (pass at least the expected count).
pub fn read_usdc_toc_from_path(path: &str, max_sections: u64) -> Result<(u64, Vec<UsdcTocSection>), i32> {
    let c = CString::new(path).map_err(|_| 1i32)?;
    let mut total: u64 = 0;
    let mut returned: u64 = 0;
    let mut raw_ptr: *mut FreeusdUsdcTocSection = ptr::null_mut();
    let rc = unsafe {
        freeusd_read_usdc_toc_from_path_utf8(c.as_ptr(), max_sections, &mut total, &mut raw_ptr, &mut returned)
    };
    if rc != 0 {
        return Err(rc);
    }
    let mut out = Vec::new();
    if !raw_ptr.is_null() && returned > 0 {
        for i in 0..returned {
            let raw = unsafe { &*raw_ptr.add(i as usize) };
            out.push(toc_section_from_raw(raw));
        }
        unsafe { freeusd_usdc_toc_sections_free(raw_ptr) };
    }
    Ok((total, out))
}

/// Reads one raw USDC section payload by name. The returned bytes are copied into Rust-owned memory.
pub fn read_usdc_section_bytes_from_path(path: &str, section_name: &str, max_bytes: u64) -> Result<Vec<u8>, i32> {
    let c_path = CString::new(path).map_err(|_| 1i32)?;
    let c_name = CString::new(section_name).map_err(|_| 1i32)?;
    let mut raw_ptr: *mut u8 = ptr::null_mut();
    let mut size: u64 = 0;
    let rc = unsafe {
        freeusd_read_usdc_section_bytes_from_path_utf8(
            c_path.as_ptr(),
            c_name.as_ptr(),
            max_bytes,
            &mut raw_ptr,
            &mut size,
        )
    };
    if rc != 0 {
        return Err(rc);
    }
    if raw_ptr.is_null() || size == 0 {
        return Ok(Vec::new());
    }
    let out = unsafe { std::slice::from_raw_parts(raw_ptr as *const u8, size as usize) }.to_vec();
    unsafe { freeusd_bytes_free(raw_ptr.cast()) };
    Ok(out)
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

/// Policy for `Stage::open_from_root_file` (matches C ABI integers and C++ `RootLayerSublayersPolicy`).
pub mod root_sublayers {
    pub const NONE: i32 = 0;
    pub const SHALLOW: i32 = 1;
    pub const DEPTH_FIRST: i32 = 2;
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

    /// Open a USDA root file from disk (`sublayer_policy`: see [`root_sublayers`]).
    pub fn open_from_root_file(path: &str, sublayer_policy: i32) -> Option<Self> {
        let cs = std::ffi::CString::new(path).ok()?;
        let p = unsafe { freeusd_stage_open_from_root_file_utf8(cs.as_ptr(), sublayer_policy as c_int) };
        if p.is_null() {
            None
        } else {
            Some(Self { ptr: p })
        }
    }

    fn optional_layer_double(
        &self,
        f: unsafe extern "C" fn(*const FreeusdStage, *mut c_double, *mut c_int) -> c_int,
    ) -> Result<Option<f64>, i32> {
        let mut v: c_double = 0.0;
        let mut has: c_int = 0;
        let rc = unsafe { f(self.ptr as *const FreeusdStage, &mut v, &mut has) };
        if rc != 0 {
            return Err(rc as i32);
        }
        Ok(if has != 0 { Some(v as f64) } else { None })
    }

    pub fn start_time_code(&self) -> Result<Option<f64>, i32> {
        self.optional_layer_double(freeusd_stage_get_start_time_code)
    }

    pub fn end_time_code(&self) -> Result<Option<f64>, i32> {
        self.optional_layer_double(freeusd_stage_get_end_time_code)
    }

    pub fn time_codes_per_second(&self) -> Result<Option<f64>, i32> {
        self.optional_layer_double(freeusd_stage_get_time_codes_per_second)
    }

    pub fn frames_per_second(&self) -> Result<Option<f64>, i32> {
        self.optional_layer_double(freeusd_stage_get_frames_per_second)
    }

    pub fn meters_per_unit(&self) -> Result<Option<f64>, i32> {
        self.optional_layer_double(freeusd_stage_get_meters_per_unit)
    }

    pub fn frame_precision(&self) -> Result<Option<i64>, i32> {
        let mut v: i64 = 0;
        let mut has: c_int = 0;
        let rc = unsafe {
            freeusd_stage_get_frame_precision(self.ptr as *const FreeusdStage, &mut v, &mut has)
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        Ok(if has != 0 { Some(v) } else { None })
    }

    pub fn up_axis(&self) -> Result<Option<String>, i32> {
        let mut out: *mut c_char = ptr::null_mut();
        let rc = unsafe { freeusd_stage_get_up_axis_utf8(self.ptr as *const FreeusdStage, &mut out) };
        if rc as i32 == ERR_NOT_FOUND {
            return Ok(None);
        }
        if rc != 0 {
            return Err(rc as i32);
        }
        if out.is_null() {
            return Ok(None);
        }
        let s = unsafe { CStr::from_ptr(out).to_string_lossy().into_owned() };
        unsafe { freeusd_string_free(out) };
        Ok(Some(s))
    }

    pub fn prim_order_paths(&self) -> Result<Vec<String>, i32> {
        let mut rows: *mut *mut c_char = ptr::null_mut();
        let mut count: usize = 0;
        let rc = unsafe {
            freeusd_stage_list_prim_order_paths_utf8(self.ptr as *const FreeusdStage, &mut rows, &mut count)
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        if count == 0 || rows.is_null() {
            return Ok(Vec::new());
        }
        let mut v = Vec::with_capacity(count);
        for i in 0..count {
            let p = unsafe { *rows.add(i) };
            if p.is_null() {
                continue;
            }
            let s = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
            v.push(s);
        }
        unsafe { freeusd_path_list_free(rows, count) };
        Ok(v)
    }

    /// Composed USDA `def` / `class` / `over` specifier for `prim_path`.
    pub fn resolve_prim_specifier_kind(&self, prim_path: &str) -> Result<PrimSpecifierKind, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let rc = unsafe {
            freeusd_stage_resolve_prim_specifier_kind(self.ptr as *const FreeusdStage, pp.as_ptr())
        };
        if rc < 0 {
            return Err(-rc);
        }
        match rc {
            0 => Ok(PrimSpecifierKind::Default),
            1 => Ok(PrimSpecifierKind::Def),
            2 => Ok(PrimSpecifierKind::Class),
            3 => Ok(PrimSpecifierKind::Over),
            _ => Err(rc as i32),
        }
    }

    /// True if `prim_path` exists in the composed stage (maps to `freeusd_stage_prim_is_valid`).
    pub fn prim_path_in_use(&self, prim_path: &str) -> Result<bool, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let rc = unsafe {
            freeusd_stage_prim_is_valid(self.ptr as *const FreeusdStage, pp.as_ptr())
        };
        if rc < 0 {
            Err(rc as i32)
        } else {
            Ok(rc == 1)
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

    /// Composed scalar as `f32` (`float` or coerced from double / int / bool).
    pub fn read_field_float(&self, prim_path: &str, attr: &str, time: f64) -> Result<f32, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut out: c_float = 0.0;
        let rc = unsafe {
            freeusd_stage_read_field_float(
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
            Ok(out as f32)
        }
    }

    /// Composed `bool` at `time` (strict; bool payload only).
    pub fn read_field_bool(&self, prim_path: &str, attr: &str, time: f64) -> Result<bool, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut out: c_int = 0;
        let rc = unsafe {
            freeusd_stage_read_field_bool(
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
            Ok(out != 0)
        }
    }

    /// Composed integer (`int64` / coerced scalars per C ABI) at `time`.
    pub fn read_field_int64(&self, prim_path: &str, attr: &str, time: f64) -> Result<i64, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut out: i64 = 0;
        let rc = unsafe {
            freeusd_stage_read_field_int64(
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
            Ok(out)
        }
    }

    /// Composed `string` or token-as-string at `time` (matches C `read_field_string`).
    pub fn read_field_string(&self, prim_path: &str, attr: &str, time: f64) -> Result<String, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut p: *mut c_char = ptr::null_mut();
        let rc = unsafe {
            freeusd_stage_read_field_string(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut p,
            )
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        if p.is_null() {
            return Ok(String::new());
        }
        let s = unsafe {
            let out = CStr::from_ptr(p).to_string_lossy().into_owned();
            freeusd_string_free(p);
            out
        };
        Ok(s)
    }

    /// Composed `double3` / `Vec3d` at `time` (strict; no `Vec3f` promotion).
    pub fn read_field_vec3d(&self, prim_path: &str, attr: &str, time: f64) -> Result<(f64, f64, f64), i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut x: c_double = 0.0;
        let mut y: c_double = 0.0;
        let mut z: c_double = 0.0;
        let rc = unsafe {
            freeusd_stage_read_field_vec3d(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut x,
                &mut y,
                &mut z,
            )
        };
        if rc != 0 {
            Err(rc as i32)
        } else {
            Ok((x as f64, y as f64, z as f64))
        }
    }

    /// Composed `float3` / `Vec3f` (or `Vec3d` narrowed to `f32`) at `time`.
    pub fn read_field_vec3f(&self, prim_path: &str, attr: &str, time: f64) -> Result<(f32, f32, f32), i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut x: c_float = 0.0;
        let mut y: c_float = 0.0;
        let mut z: c_float = 0.0;
        let rc = unsafe {
            freeusd_stage_read_field_vec3f(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut x,
                &mut y,
                &mut z,
            )
        };
        if rc != 0 {
            Err(rc as i32)
        } else {
            Ok((x as f32, y as f32, z as f32))
        }
    }

    /// Composed `matrix4d` at `time` as 16 `f64` values, row-major `m[row * 4 + col]`.
    pub fn read_field_matrix4d(&self, prim_path: &str, attr: &str, time: f64) -> Result<[f64; 16], i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut m = [0.0_f64; 16];
        let rc = unsafe {
            freeusd_stage_read_field_matrix4d(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                m.as_mut_ptr(),
            )
        };
        if rc != 0 {
            Err(rc as i32)
        } else {
            Ok(m)
        }
    }

    /// Composed `quatd` at `time` as `(real, i, j, k)`.
    pub fn read_field_quatd(&self, prim_path: &str, attr: &str, time: f64) -> Result<(f64, f64, f64, f64), i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut r: c_double = 0.0;
        let mut i: c_double = 0.0;
        let mut j: c_double = 0.0;
        let mut k: c_double = 0.0;
        let rc = unsafe {
            freeusd_stage_read_field_quatd(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut r,
                &mut i,
                &mut j,
                &mut k,
            )
        };
        if rc != 0 {
            Err(rc as i32)
        } else {
            Ok((r as f64, i as f64, j as f64, k as f64))
        }
    }

    /// Composed `quatf` (or `quatd` narrowed) at `time`.
    pub fn read_field_quatf(&self, prim_path: &str, attr: &str, time: f64) -> Result<(f32, f32, f32, f32), i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut r: c_float = 0.0;
        let mut i: c_float = 0.0;
        let mut j: c_float = 0.0;
        let mut k: c_float = 0.0;
        let rc = unsafe {
            freeusd_stage_read_field_quatf(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut r,
                &mut i,
                &mut j,
                &mut k,
            )
        };
        if rc != 0 {
            Err(rc as i32)
        } else {
            Ok((r as f32, i as f32, j as f32, k as f32))
        }
    }

    /// Composed `token` (not a plain string) at `time`.
    pub fn read_field_token(&self, prim_path: &str, attr: &str, time: f64) -> Result<String, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut p: *mut c_char = std::ptr::null_mut();
        let rc = unsafe {
            freeusd_stage_read_field_token(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut p,
            )
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        if p.is_null() {
            return Ok(String::new());
        }
        let s = unsafe {
            let out = std::ffi::CStr::from_ptr(p).to_string_lossy().into_owned();
            freeusd_string_free(p);
            out
        };
        Ok(s)
    }

    /// Composed `token[]` at `time` (empty vec is ok).
    pub fn read_field_token_array(&self, prim_path: &str, attr: &str, time: f64) -> Result<Vec<String>, i32> {
        let pp = std::ffi::CString::new(prim_path).map_err(|_| 1)?;
        let an = std::ffi::CString::new(attr).map_err(|_| 1)?;
        let mut arr: *mut *mut c_char = std::ptr::null_mut();
        let mut n: usize = 0;
        let rc = unsafe {
            freeusd_stage_read_field_token_array(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                an.as_ptr(),
                time as c_double,
                &mut arr,
                &mut n,
            )
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        if n == 0 || arr.is_null() {
            return Ok(Vec::new());
        }
        let slice = unsafe { std::slice::from_raw_parts(arr, n) };
        let mut out = Vec::with_capacity(n);
        for &p in slice {
            if !p.is_null() {
                out.push(unsafe { std::ffi::CStr::from_ptr(p).to_string_lossy().into_owned() });
            }
        }
        unsafe {
            freeusd_path_list_free(arr, n);
        }
        Ok(out)
    }

    /// True if any composed layer authors `relocates` with this source prim path.
    pub fn relocate_source_in_any_layer(&self, from_prim: &str) -> Result<bool, i32> {
        let pp = CString::new(from_prim).map_err(|_| 1)?;
        let rc = unsafe {
            freeusd_stage_relocate_source_in_any_layer(self.ptr as *const FreeusdStage, pp.as_ptr())
        };
        if rc < 0 {
            Err(rc as i32)
        } else {
            Ok(rc == 1)
        }
    }

    /// Composed relocate target for `from_prim`, or `None` if unmapped (`ERR_NOT_FOUND`).
    pub fn composed_relocate_target(&self, from_prim: &str) -> Result<Option<String>, i32> {
        let pp = CString::new(from_prim).map_err(|_| 1)?;
        let mut out: *mut c_char = ptr::null_mut();
        let rc = unsafe {
            freeusd_stage_get_composed_relocate_target_utf8(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                &mut out,
            )
        };
        if rc as i32 == ERR_NOT_FOUND {
            return Ok(None);
        }
        if rc != 0 {
            return Err(rc as i32);
        }
        if out.is_null() {
            return Ok(None);
        }
        let s = unsafe { CStr::from_ptr(out).to_string_lossy().into_owned() };
        unsafe { freeusd_string_free(out) };
        Ok(Some(s))
    }

    /// Sorted composed relocate pairs `(from, to)` (layer stack / USD rules).
    pub fn list_composed_relocate_pairs(&self) -> Result<Vec<(String, String)>, i32> {
        let mut rows: *mut *mut c_char = ptr::null_mut();
        let mut count: usize = 0;
        let rc = unsafe {
            freeusd_stage_list_composed_relocate_pairs_utf8(
                self.ptr as *const FreeusdStage,
                &mut rows,
                &mut count,
            )
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        if count == 0 || rows.is_null() {
            return Ok(Vec::new());
        }
        let mut v = Vec::with_capacity(count);
        for i in 0..count {
            let p = unsafe { *rows.add(i) };
            if p.is_null() {
                continue;
            }
            let s = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
            if let Some((a, b)) = s.split_once('\x1f') {
                v.push((a.to_string(), b.to_string()));
            }
        }
        unsafe { freeusd_path_list_free(rows, count) };
        Ok(v)
    }

    pub fn prefix_substitution_key_in_any_layer(&self, from_prefix: &str) -> Result<bool, i32> {
        let pp = CString::new(from_prefix).map_err(|_| 1)?;
        let rc = unsafe {
            freeusd_stage_prefix_substitution_key_in_any_layer(self.ptr as *const FreeusdStage, pp.as_ptr())
        };
        if rc < 0 {
            Err(rc as i32)
        } else {
            Ok(rc == 1)
        }
    }

    pub fn composed_prefix_substitution(&self, from_prefix: &str) -> Result<Option<String>, i32> {
        let pp = CString::new(from_prefix).map_err(|_| 1)?;
        let mut out: *mut c_char = ptr::null_mut();
        let rc = unsafe {
            freeusd_stage_get_composed_prefix_substitution_utf8(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                &mut out,
            )
        };
        if rc as i32 == ERR_NOT_FOUND {
            return Ok(None);
        }
        if rc != 0 {
            return Err(rc as i32);
        }
        if out.is_null() {
            return Ok(None);
        }
        let s = unsafe { CStr::from_ptr(out).to_string_lossy().into_owned() };
        unsafe { freeusd_string_free(out) };
        Ok(Some(s))
    }

    pub fn list_composed_prefix_substitution_pairs(&self) -> Result<Vec<(String, String)>, i32> {
        let mut rows: *mut *mut c_char = ptr::null_mut();
        let mut count: usize = 0;
        let rc = unsafe {
            freeusd_stage_list_composed_prefix_substitution_pairs_utf8(
                self.ptr as *const FreeusdStage,
                &mut rows,
                &mut count,
            )
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        if count == 0 || rows.is_null() {
            return Ok(Vec::new());
        }
        let mut v = Vec::with_capacity(count);
        for i in 0..count {
            let p = unsafe { *rows.add(i) };
            if p.is_null() {
                continue;
            }
            let s = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
            if let Some((a, b)) = s.split_once('\x1f') {
                v.push((a.to_string(), b.to_string()));
            }
        }
        unsafe { freeusd_path_list_free(rows, count) };
        Ok(v)
    }

    pub fn custom_layer_data_key_in_any_layer(&self, key: &str) -> Result<bool, i32> {
        let k = CString::new(key).map_err(|_| 1)?;
        let rc = unsafe {
            freeusd_stage_custom_layer_data_key_in_any_layer(self.ptr as *const FreeusdStage, k.as_ptr())
        };
        if rc < 0 {
            Err(rc as i32)
        } else {
            Ok(rc == 1)
        }
    }

    pub fn composed_custom_layer_data(&self, key: &str) -> Result<Option<String>, i32> {
        let k = CString::new(key).map_err(|_| 1)?;
        let mut out: *mut c_char = ptr::null_mut();
        let rc = unsafe {
            freeusd_stage_get_composed_custom_layer_data_utf8(
                self.ptr as *const FreeusdStage,
                k.as_ptr(),
                &mut out,
            )
        };
        if rc as i32 == ERR_NOT_FOUND {
            return Ok(None);
        }
        if rc != 0 {
            return Err(rc as i32);
        }
        if out.is_null() {
            return Ok(None);
        }
        let s = unsafe { CStr::from_ptr(out).to_string_lossy().into_owned() };
        unsafe { freeusd_string_free(out) };
        Ok(Some(s))
    }

    pub fn list_composed_custom_layer_data_keys(&self) -> Result<Vec<String>, i32> {
        let mut rows: *mut *mut c_char = ptr::null_mut();
        let mut count: usize = 0;
        let rc = unsafe {
            freeusd_stage_list_composed_custom_layer_data_keys(
                self.ptr as *const FreeusdStage,
                &mut rows,
                &mut count,
            )
        };
        if rc != 0 {
            return Err(rc as i32);
        }
        if count == 0 || rows.is_null() {
            return Ok(Vec::new());
        }
        let mut v = Vec::with_capacity(count);
        for i in 0..count {
            let p = unsafe { *rows.add(i) };
            if p.is_null() {
                continue;
            }
            let s = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
            v.push(s);
        }
        unsafe { freeusd_path_list_free(rows, count) };
        Ok(v)
    }

    pub fn composed_prim_variant_selection(&self, prim_path: &str, variant_set: &str) -> Result<Option<String>, i32> {
        let pp = CString::new(prim_path).map_err(|_| 1)?;
        let vs = CString::new(variant_set).map_err(|_| 1)?;
        let mut out: *mut c_char = ptr::null_mut();
        let rc = unsafe {
            freeusd_stage_get_composed_prim_variant_selection_utf8(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                vs.as_ptr(),
                &mut out,
            )
        };
        if rc as i32 == ERR_NOT_FOUND {
            return Ok(None);
        }
        if rc != 0 {
            return Err(rc as i32);
        }
        if out.is_null() {
            return Ok(None);
        }
        let s = unsafe { CStr::from_ptr(out).to_string_lossy().into_owned() };
        unsafe { freeusd_string_free(out) };
        Ok(Some(s))
    }

    pub fn prim_variant_selection_set_in_any_layer(&self, prim_path: &str, variant_set: &str) -> Result<bool, i32> {
        let pp = CString::new(prim_path).map_err(|_| 1)?;
        let vs = CString::new(variant_set).map_err(|_| 1)?;
        let rc = unsafe {
            freeusd_stage_prim_variant_selection_set_in_any_layer(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                vs.as_ptr(),
            )
        };
        if rc < 0 {
            Err(rc as i32)
        } else {
            Ok(rc == 1)
        }
    }

    fn string_list_from_stage_path(
        &self,
        f: unsafe extern "C" fn(*const FreeusdStage, *const c_char, *mut *mut *mut c_char, *mut usize) -> c_int,
        prim_path: &str,
    ) -> Result<Vec<String>, i32> {
        let pp = CString::new(prim_path).map_err(|_| 1)?;
        let mut rows: *mut *mut c_char = ptr::null_mut();
        let mut count: usize = 0;
        let rc = unsafe { f(self.ptr as *const FreeusdStage, pp.as_ptr(), &mut rows, &mut count) };
        if rc != 0 {
            return Err(rc as i32);
        }
        if count == 0 || rows.is_null() {
            return Ok(Vec::new());
        }
        let mut v = Vec::with_capacity(count);
        for i in 0..count {
            let p = unsafe { *rows.add(i) };
            if p.is_null() {
                continue;
            }
            let s = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
            v.push(s);
        }
        unsafe { freeusd_path_list_free(rows, count) };
        Ok(v)
    }

    pub fn list_composed_prim_variant_selection_sets(&self, prim_path: &str) -> Result<Vec<String>, i32> {
        self.string_list_from_stage_path(freeusd_stage_list_composed_prim_variant_selection_sets_utf8, prim_path)
    }

    pub fn list_composed_prim_variant_set_names(&self, prim_path: &str) -> Result<Vec<String>, i32> {
        self.string_list_from_stage_path(freeusd_stage_list_composed_prim_variant_set_names_utf8, prim_path)
    }

    pub fn prim_variant_set_declared_in_any_layer(&self, prim_path: &str, variant_set_name: &str) -> Result<bool, i32> {
        let pp = CString::new(prim_path).map_err(|_| 1)?;
        let vn = CString::new(variant_set_name).map_err(|_| 1)?;
        let rc = unsafe {
            freeusd_stage_prim_variant_set_declared_in_any_layer(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                vn.as_ptr(),
            )
        };
        if rc < 0 {
            Err(rc as i32)
        } else {
            Ok(rc == 1)
        }
    }

    pub fn list_composed_prim_variant_names(&self, prim_path: &str, variant_set_name: &str) -> Result<Vec<String>, i32> {
        let pp = CString::new(prim_path).map_err(|_| 1)?;
        let vn = CString::new(variant_set_name).map_err(|_| 1)?;
        let mut rows: *mut *mut c_char = ptr::null_mut();
        let mut count: usize = 0;
        let rc = unsafe {
            freeusd_stage_list_composed_prim_variant_names_utf8(
                self.ptr as *const FreeusdStage,
                pp.as_ptr(),
                vn.as_ptr(),
                &mut rows,
                &mut count,
            )
        };
        if rc as i32 == ERR_NOT_FOUND {
            return Err(ERR_NOT_FOUND);
        }
        if rc != 0 {
            return Err(rc as i32);
        }
        if count == 0 || rows.is_null() {
            return Ok(Vec::new());
        }
        let mut v = Vec::with_capacity(count);
        for i in 0..count {
            let p = unsafe { *rows.add(i) };
            if p.is_null() {
                continue;
            }
            let s = unsafe { CStr::from_ptr(p).to_string_lossy().into_owned() };
            v.push(s);
        }
        unsafe { freeusd_path_list_free(rows, count) };
        Ok(v)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::path::PathBuf;

    fn fixture_path(name: &str) -> PathBuf {
        PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("../../../tests/fixtures")
            .join(name)
    }

    #[test]
    fn version_non_empty() {
        assert!(!version_string().is_empty());
    }

    #[test]
    fn usdc_crate_id_is_pxr_magic() {
        assert_eq!(usdc_crate_identifier(), "PXR-USDC");
    }

    #[test]
    fn read_usdc_bootstrap_roundtrip() {
        let dir = std::env::temp_dir();
        let p = dir.join(format!("freeusd_rust_boot_{}.usdc", std::process::id()));
        let mut buf = vec![0u8; 128];
        buf[0..8].copy_from_slice(usdc_crate_identifier().as_bytes());
        buf[8] = 0;
        buf[9] = 8;
        buf[10] = 0;
        buf[16..24].copy_from_slice(&88i64.to_le_bytes());
        std::fs::write(&p, &buf).expect("write bootstrap");
        let b = read_usdc_bootstrap_from_path(&p.to_string_lossy()).expect("read bootstrap");
        let _ = std::fs::remove_file(&p);
        assert_eq!(b.file_version_major, 0);
        assert_eq!(b.file_version_minor, 8);
        assert_eq!(b.file_version_patch, 0);
        assert_eq!(b.toc_byte_offset, 88);
    }

    #[test]
    fn read_usdc_toc_roundtrip() {
        let dir = std::env::temp_dir();
        let p = dir.join(format!("freeusd_rust_toc_{}.usdc", std::process::id()));
        let mut buf = vec![0u8; 160];
        buf[0..8].copy_from_slice(usdc_crate_identifier().as_bytes());
        buf[8] = 0;
        buf[9] = 8;
        buf[10] = 0;
        buf[16..24].copy_from_slice(&88i64.to_le_bytes());
        buf[88..96].copy_from_slice(&2u64.to_le_bytes());
        buf[96..103].copy_from_slice(b"TOKENS\x00");
        buf[128..134].copy_from_slice(b"PATHS\x00");
        buf[128 + 16..128 + 24].copy_from_slice(&120i64.to_le_bytes());
        buf[128 + 24..128 + 32].copy_from_slice(&40i64.to_le_bytes());
        std::fs::write(&p, &buf).expect("write toc");
        let (n, secs) = read_usdc_toc_from_path(&p.to_string_lossy(), 16).expect("read toc");
        let _ = std::fs::remove_file(&p);
        assert_eq!(n, 2);
        assert_eq!(secs.len(), 2);
        assert_eq!(secs[0].name, "TOKENS");
        assert_eq!(secs[1].name, "PATHS");
        assert_eq!(secs[1].start_byte_offset, 120);
        assert_eq!(secs[1].size_bytes, 40);
    }

    #[test]
    fn read_usdc_section_bytes_roundtrip() {
        let dir = std::env::temp_dir();
        let p = dir.join(format!("freeusd_rust_section_{}.usdc", std::process::id()));
        let mut buf = vec![0u8; 160];
        buf[0..8].copy_from_slice(usdc_crate_identifier().as_bytes());
        buf[8] = 0;
        buf[9] = 8;
        buf[10] = 0;
        buf[16..24].copy_from_slice(&88i64.to_le_bytes());
        buf[88..96].copy_from_slice(&1u64.to_le_bytes());
        buf[96..103].copy_from_slice(b"TOKENS\x00");
        buf[112..120].copy_from_slice(&128i64.to_le_bytes());
        buf[120..128].copy_from_slice(&5i64.to_le_bytes());
        buf[128..133].copy_from_slice(b"alpha");
        std::fs::write(&p, &buf).expect("write section");
        let payload =
            read_usdc_section_bytes_from_path(&p.to_string_lossy(), "TOKENS", 64).expect("read section");
        assert_eq!(payload, b"alpha");
        assert!(read_usdc_section_bytes_from_path(&p.to_string_lossy(), "MISSING", 64).is_err());
        let _ = std::fs::remove_file(&p);
    }

    #[test]
    fn detect_usd_file_kind_fixture_and_crate_prefix() {
        let fixture = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../tests/fixtures/stage_open_root.usda");
        let p = fixture.to_string_lossy();
        let (k, d) = detect_usd_file_kind_from_path(&p).expect("fixture usda");
        assert_eq!(k, UsdFileKind::UsdaAscii);
        assert!(d.is_none());

        let dir = std::env::temp_dir();
        let tmp = dir.join(format!("freeusd_rust_kind_{}.bin", std::process::id()));
        let mut payload = usdc_crate_identifier().as_bytes().to_vec();
        payload.push(0);
        std::fs::write(&tmp, &payload).expect("write temp crate prefix");
        let (k2, d2) = detect_usd_file_kind_from_path(&tmp.to_string_lossy()).expect("crate sniff");
        let _ = std::fs::remove_file(&tmp);
        assert_eq!(k2, UsdFileKind::UsdcCrate);
        assert!(d2.is_none());
    }

    #[test]
    fn open_from_root_fixture_stacks_sublayers() {
        let root = PathBuf::from(env!("CARGO_MANIFEST_DIR"))
            .join("../../../tests/fixtures/stage_open_root.usda");
        let root = root.to_string_lossy();
        let st_none = Stage::open_from_root_file(&root, root_sublayers::NONE).expect("stage none");
        assert!(
            !st_none
                .prim_path_in_use("/FromSub")
                .expect("prim_path_in_use"),
            "sub prim should be absent when sublayers disabled"
        );
        drop(st_none);

        let st = Stage::open_from_root_file(&root, root_sublayers::DEPTH_FIRST).expect("stage df");
        assert!(
            st.prim_path_in_use("/FromSub").expect("prim_path_in_use"),
            "sub prim should exist when depth-first sublayers load"
        );
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

    #[test]
    fn cross_language_field_read_contract() {
        let fixture = fixture_path("usd_cross_language.usda");
        let usda = std::fs::read_to_string(&fixture).expect("read fixture");
        let mut layer = Layer::new_anonymous(Some("rust_cross_lang")).expect("layer");
        assert_eq!(layer.load_usda(&usda), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");

        assert!(stage.prim_path_in_use("/Scene/Child").expect("child valid"));
        assert!(!stage.prim_path_in_use("/Scene/Missing").expect("missing invalid"));

        assert!((stage.read_field_double("/Scene/Child", "mass", 1.0).expect("mass") - 2.5).abs() < 1e-9);
        assert!((stage.read_field_double("/Scene/Child", "mass", 2.0).expect("mass samples") - 4.0).abs() < 1e-9);
        assert!((stage.read_field_float("/Scene/Child", "density", 1.0).expect("density") - 1.25).abs() < 1e-5);
        assert!((stage.read_field_float("/Scene/Child", "mass", 1.0).expect("mass float") - 2.5).abs() < 1e-5);
        assert!(stage.read_field_bool("/Scene/Child", "enabled", 1.0).expect("enabled"));
        assert_eq!(stage.read_field_int64("/Scene/Child", "count", 1.0).expect("count"), -7);
        assert_eq!(stage.read_field_int64("/Scene/Child", "mass", 1.0).expect("mass int"), 2);
        assert_eq!(stage.read_field_string("/Scene/Child", "label", 1.0).expect("label"), "hello");
        assert_eq!(stage.read_field_string("/Scene/Child", "kind", 1.0).expect("kind string"), "component");

        let (x, y, z) = stage.read_field_vec3d("/Scene/Child", "extent", 1.0).expect("extent");
        assert!((x - 1.0).abs() < 1e-9 && (y - 2.0).abs() < 1e-9 && (z - 3.0).abs() < 1e-9);

        let (x, y, z) = stage
            .read_field_vec3f("/Scene/Child", "displayColor", 1.0)
            .expect("displayColor");
        assert!((x - 0.25).abs() < 1e-5 && (y - 0.5).abs() < 1e-5 && (z - 0.75).abs() < 1e-5);

        let (x, y, z) = stage.read_field_vec3f("/Scene/Child", "extent", 1.0).expect("extent vec3f");
        assert!((x - 1.0).abs() < 1e-5 && (y - 2.0).abs() < 1e-5 && (z - 3.0).abs() < 1e-5);

        let m = stage.read_field_matrix4d("/Scene/Child", "xf", 1.0).expect("xf");
        assert_eq!(m[0], 1.0);
        assert_eq!(m[5], 1.0);
        assert_eq!(m[10], 1.0);
        assert_eq!(m[15], 1.0);

        let (r, i, j, k) = stage.read_field_quatd("/Scene/Child", "qd", 1.0).expect("qd");
        assert!((r - 1.0).abs() < 1e-9 && i.abs() < 1e-9 && j.abs() < 1e-9 && k.abs() < 1e-9);

        let (r, i, j, k) = stage.read_field_quatf("/Scene/Child", "qf", 1.0).expect("qf");
        assert!((r - 0.70710677).abs() < 1e-5 && i.abs() < 1e-5 && j.abs() < 1e-5 && (k - 0.70710677).abs() < 1e-5);

        let (r, i, j, k) = stage.read_field_quatf("/Scene/Child", "qd", 1.0).expect("qd narrowed");
        assert!((r - 1.0).abs() < 1e-5 && i.abs() < 1e-5 && j.abs() < 1e-5 && k.abs() < 1e-5);

        assert_eq!(stage.read_field_token("/Scene/Child", "kind", 1.0).expect("kind token"), "component");
        assert_eq!(
            stage
                .read_field_token_array("/Scene/Child", "tags", 1.0)
                .expect("tags"),
            vec!["a".to_string(), "b".to_string()]
        );

        assert!(stage.read_field_double("/Scene/Child", "missingAttr", 1.0).is_err());
        assert!(stage.read_field_double("/Scene/Missing", "mass", 1.0).is_err());
        assert!(stage.read_field_quatd("/Scene/Child", "qf", 1.0).is_err());
        assert!(stage.read_field_token("/Scene/Child", "label", 1.0).is_err());
        assert!(stage.read_field_token_array("/Scene/Child", "kind", 1.0).is_err());
    }

    #[test]
    fn read_field_missing_attribute_returns_err() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        double x = 1.0
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_miss")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert!(stage.read_field_double("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_float("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_vec3d("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_vec3f("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_quatd("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_quatf("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_int64("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_string("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_bool("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_matrix4d("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_token("/W/C", "no_such", 1.0).is_err());
        assert!(stage.read_field_token_array("/W/C", "no_such", 1.0).is_err());
    }

    #[test]
    fn read_bool_int64_string_smoke() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        bool flag = true
        int n = 42
        string label = "hi"
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_bis")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert!(stage
            .read_field_bool("/W/C", "flag", 1.0)
            .expect(last_error_message()));
        assert_eq!(
            stage
                .read_field_int64("/W/C", "n", 1.0)
                .expect(last_error_message()),
            42
        );
        assert_eq!(
            stage
                .read_field_string("/W/C", "label", 1.0)
                .expect(last_error_message()),
            "hi"
        );
    }

    #[test]
    fn read_float_smoke() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        float r = 2.5
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_float")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        let f = stage
            .read_field_float("/W/C", "r", 1.0)
            .expect(last_error_message());
        assert!((f - 2.5f32).abs() < 1e-5);
    }

    #[test]
    fn read_vec3d_smoke() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        double3 extent = (1.5, 2.5, 3.5)
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_vec3d")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        let (x, y, z) = stage
            .read_field_vec3d("/W/C", "extent", 1.0)
            .expect(last_error_message());
        assert!((x - 1.5).abs() < 1e-9);
        assert!((y - 2.5).abs() < 1e-9);
        assert!((z - 3.5).abs() < 1e-9);
    }

    #[test]
    fn read_vec3f_smoke() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "W"
{
    def "C"
    {
        normal3f n = (0.25, 0.5, 0.75)
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_vec3f")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        let (x, y, z) = stage
            .read_field_vec3f("/W/C", "n", 1.0)
            .expect(last_error_message());
        assert!((x - 0.25f32).abs() < 1e-5);
        assert!((y - 0.5f32).abs() < 1e-5);
        assert!((z - 0.75f32).abs() < 1e-5);
    }

    #[test]
    fn read_matrix4d_quat_token_smoke() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "W"
{
    def "P"
    {
        matrix4d m = ((1,0,0,0), (0,1,0,0), (0,0,1,0), (0,0,0,1))
        quatd qd = (1, 0, 0, 0)
        quatf qf = (0.70710677, 0, 0, 0.70710677)
        token kind = component
        token[] tags = [@a@, @b@]
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_gfreads")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        let m = stage
            .read_field_matrix4d("/W/P", "m", 1.0)
            .expect(last_error_message());
        for i in 0..16 {
            let want = if i == 0 || i == 5 || i == 10 || i == 15 {
                1.0
            } else {
                0.0
            };
            assert!((m[i] - want).abs() < 1e-9, "m[{i}]={}", m[i]);
        }
        let (qr, qi, qj, qk) = stage
            .read_field_quatd("/W/P", "qd", 1.0)
            .expect(last_error_message());
        assert!((qr - 1.0).abs() < 1e-9 && qi.abs() < 1e-9 && qj.abs() < 1e-9 && qk.abs() < 1e-9);
        let (fr, fi, fj, fk) = stage
            .read_field_quatf("/W/P", "qf", 1.0)
            .expect(last_error_message());
        assert!((fr - 0.70710677f32).abs() < 1e-5);
        assert!(fi.abs() < 1e-5 && fj.abs() < 1e-5);
        assert!((fk - 0.70710677f32).abs() < 1e-5);
        let tok = stage
            .read_field_token("/W/P", "kind", 1.0)
            .expect(last_error_message());
        assert_eq!(tok, "component");
        let tags = stage
            .read_field_token_array("/W/P", "tags", 1.0)
            .expect(last_error_message());
        assert_eq!(tags, vec!["a".to_string(), "b".to_string()]);
    }

    #[test]
    fn composed_relocates() {
        const USDA: &str = r#"#usda 1.0
(
    relocates = {
        </Src>: </Dst>,
    }
)
def Xform "Src"
(
)
{
}
def Xform "Dst"
(
)
{
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_reloc")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert!(stage.relocate_source_in_any_layer("/Src").unwrap());
        assert_eq!(
            stage.composed_relocate_target("/Src").unwrap(),
            Some("/Dst".to_string())
        );
        let pairs = stage.list_composed_relocate_pairs().unwrap();
        assert_eq!(pairs, vec![("/Src".to_string(), "/Dst".to_string())]);
        assert!(!stage.relocate_source_in_any_layer("/Nope").unwrap());
    }

    #[test]
    fn composed_prefix_substitutions() {
        const USDA: &str = r#"#usda 1.0
(
    prefixSubstitutions = {
        "/Models": "/ModelsV2",
    }
)
def Xform "Root"
(
)
{
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_psub")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert!(stage.prefix_substitution_key_in_any_layer("/Models").unwrap());
        assert_eq!(
            stage.composed_prefix_substitution("/Models").unwrap(),
            Some("/ModelsV2".to_string())
        );
        let pairs = stage.list_composed_prefix_substitution_pairs().unwrap();
        assert_eq!(pairs, vec![("/Models".to_string(), "/ModelsV2".to_string())]);
        assert!(!stage.prefix_substitution_key_in_any_layer("/Nope").unwrap());
        assert_eq!(stage.composed_prefix_substitution("/Nope").unwrap(), None);
    }

    #[test]
    fn composed_custom_layer_data() {
        const USDA: &str = r#"#usda 1.0
(
    customLayerData = {
        string layerTag = "hello",
        string layerBuild = "2025",
    }
)
def "R"
(
)
{
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_cld")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert!(stage.custom_layer_data_key_in_any_layer("layerTag").unwrap());
        assert_eq!(
            stage.composed_custom_layer_data("layerTag").unwrap(),
            Some("hello".to_string())
        );
        let keys = stage.list_composed_custom_layer_data_keys().unwrap();
        assert_eq!(keys, vec!["layerBuild".to_string(), "layerTag".to_string()]);
        assert!(!stage.custom_layer_data_key_in_any_layer("nope").unwrap());
        assert_eq!(stage.composed_custom_layer_data("nope").unwrap(), None);
    }

    #[test]
    fn composed_prim_variant() {
        const USDA: &str = r#"#usda 1.0
(
)
def Xform "Root"
(
    variantSelection = {
        string modelVariant = "HiPoly",
    }
    variantSets = {
        modelVariant = {
            "HiPoly" = {},
            LoPoly = {},
        }
    }
)
{
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_var")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert!(stage
            .prim_variant_selection_set_in_any_layer("/Root", "modelVariant")
            .unwrap());
        assert_eq!(
            stage
                .composed_prim_variant_selection("/Root", "modelVariant")
                .unwrap(),
            Some("HiPoly".to_string())
        );
        assert_eq!(
            stage.list_composed_prim_variant_selection_sets("/Root").unwrap(),
            vec!["modelVariant".to_string()]
        );
        assert!(stage
            .prim_variant_set_declared_in_any_layer("/Root", "modelVariant")
            .unwrap());
        assert_eq!(
            stage.list_composed_prim_variant_set_names("/Root").unwrap(),
            vec!["modelVariant".to_string()]
        );
        assert_eq!(
            stage
                .list_composed_prim_variant_names("/Root", "modelVariant")
                .unwrap(),
            vec!["HiPoly".to_string(), "LoPoly".to_string()]
        );
        assert_eq!(stage.composed_prim_variant_selection("/Root", "missing").unwrap(), None);
        assert_eq!(
            stage.list_composed_prim_variant_names("/Root", "missing"),
            Err(ERR_NOT_FOUND)
        );
    }

    #[test]
    fn resolve_prim_specifier_kind_smoke() {
        const USDA: &str = r#"#usda 1.0
(
)
class Xform "P"
(
)
{
}
over Xform "O"
(
)
{
}
def Xform "Q"
(
)
{
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_spec")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert_eq!(
            stage.resolve_prim_specifier_kind("/P").unwrap(),
            PrimSpecifierKind::Class
        );
        assert_eq!(
            stage.resolve_prim_specifier_kind("/O").unwrap(),
            PrimSpecifierKind::Over
        );
        assert_eq!(
            stage.resolve_prim_specifier_kind("/Q").unwrap(),
            PrimSpecifierKind::Default
        );
        assert!(stage.resolve_prim_specifier_kind("not_a_path").is_err());
    }

    #[test]
    fn layer_hints_smoke() {
        const USDA: &str = r#"#usda 1.0
(
    startTimeCode = 0
    endTimeCode = 100
    timeCodesPerSecond = 30
    framesPerSecond = 30
    framePrecision = 2
    metersPerUnit = 0.01
    upAxis = "Z"
    primOrder = [</Root/A>, </Root>]
)
def Xform "Root"
{
    def "A"
    {
    }
}
"#;
        let mut layer = Layer::new_anonymous(Some("rust_hints")).expect("layer");
        assert_eq!(layer.load_usda(USDA), 0, "{}", last_error_message());
        let stage = Stage::attach_root_layer(&layer).expect("stage");
        assert_eq!(stage.start_time_code().unwrap(), Some(0.0));
        assert_eq!(stage.end_time_code().unwrap(), Some(100.0));
        assert_eq!(stage.time_codes_per_second().unwrap(), Some(30.0));
        assert_eq!(stage.frames_per_second().unwrap(), Some(30.0));
        assert_eq!(stage.frame_precision().unwrap(), Some(2));
        assert_eq!(stage.meters_per_unit().unwrap(), Some(0.01));
        assert_eq!(stage.up_axis().unwrap(), Some("Z".to_string()));
        assert_eq!(
            stage.prim_order_paths().unwrap(),
            vec!["/Root/A".to_string(), "/Root".to_string()]
        );
    }
}
