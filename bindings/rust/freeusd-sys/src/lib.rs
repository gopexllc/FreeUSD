//! Minimal safe wrappers over the FreeUSD C ABI (`freeusd.h`).
//!
//! Build the C++ project first so `build/src/libfreeusd_*.a` exist, then:
//! `cargo test --manifest-path bindings/rust/Cargo.toml`

#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]

use std::ffi::{c_char, c_double, c_int, CString, CStr};
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
    fn freeusd_stage_open_from_root_file_utf8(path: *const c_char, sublayer_policy: c_int) -> *mut FreeusdStage;
    fn freeusd_stage_free(stage: *mut FreeusdStage);
    fn freeusd_stage_prim_is_valid(stage: *const FreeusdStage, prim_path: *const c_char) -> c_int;
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
    fn freeusd_string_free(s: *mut c_char);
    fn freeusd_path_list_free(paths: *mut *mut c_char, count: usize);
    fn freeusd_stage_read_field_double(
        stage: *const FreeusdStage,
        prim_path: *const c_char,
        attr: *const c_char,
        time: c_double,
        out: *mut c_double,
    ) -> c_int;
}

/// C API `FREEUSD_ERR_NOT_FOUND` (e.g. unmapped relocate / prefix substitution / customLayerData string read).
pub const ERR_NOT_FOUND: i32 = 3;

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

    #[test]
    fn version_non_empty() {
        assert!(!version_string().is_empty());
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
}
