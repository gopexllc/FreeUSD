//! Link against the same static libraries as the C smoke test (see `CMakeFiles/freeusd_c_smoke_test.dir/link.txt`).
//! Expect `freeusd` built at `<repo>/build` from `<repo>` (this crate lives in `bindings/rust/freeusd-sys`).

use std::env;
use std::path::PathBuf;
use std::process::Command;

fn add_gcc_libstdcxx_search_path() {
    let cxx = env::var("CXX").unwrap_or_else(|_| "g++".to_string());
    let output = Command::new(cxx)
        .arg("-print-file-name=libstdc++.so")
        .output();
    let Ok(output) = output else {
        return;
    };
    if !output.status.success() {
        return;
    }
    let path_text = String::from_utf8_lossy(&output.stdout);
    let path = PathBuf::from(path_text.trim());
    if path.is_absolute() {
        if let Some(parent) = path.parent() {
            println!("cargo:rustc-link-search=native={}", parent.display());
        }
    }
}

fn main() {
    let manifest_dir = PathBuf::from(env::var("CARGO_MANIFEST_DIR").expect("CARGO_MANIFEST_DIR"));
    // freeusd-sys lives at `<repo>/bindings/rust/freeusd-sys` unless FREEUSD_REPO_ROOT is set.
    let repo_root = if let Ok(r) = env::var("FREEUSD_REPO_ROOT") {
        PathBuf::from(r).canonicalize().expect("FREEUSD_REPO_ROOT")
    } else {
        manifest_dir
            .join("../../..")
            .canonicalize()
            .expect("repo root")
    };
    let _include = repo_root.join("include");
    let lib_dir = repo_root.join("build/src");

    let lib_c = lib_dir.join("libfreeusd_c.a");
    if !lib_c.exists() {
        println!(
            "cargo:warning=freeusd static libs not found at {} - build the C++ project first (e.g. cmake -B build && cmake --build build)",
            lib_dir.display()
        );
    } else {
        println!("cargo:rerun-if-changed={}", lib_c.display());
    }

    println!(
        "cargo:rerun-if-changed={}",
        _include.join("freeusd/c/freeusd.h").display()
    );
    println!("cargo:rustc-link-search=native={}", lib_dir.display());

    // Same order as `freeusd_c_smoke_test` link line (GNU ld).
    for lib in [
        "freeusd_c",
        "freeusd_base",
        "freeusd_usdUtils",
        "freeusd_usdSkel",
        "freeusd_usdShade",
        "freeusd_usdLux",
        "freeusd_usdPhysics",
        "freeusd_usdVol",
        "freeusd_usdGeom",
        "freeusd_usd",
        "freeusd_ar",
        "freeusd_io",
        "freeusd_pcp",
        "freeusd_sdf",
        "freeusd_vt",
        "freeusd_tf",
        "freeusd_gf",
        "freeusd_plug",
    ] {
        println!("cargo:rustc-link-lib=static={}", lib);
    }

    if std::env::var("TARGET")
        .unwrap_or_default()
        .contains("apple-darwin")
    {
        println!("cargo:rustc-link-lib=c++");
    } else {
        add_gcc_libstdcxx_search_path();
        println!("cargo:rustc-link-lib=stdc++");
    }
    // USDC fixture compression in freeusd_usd (zlib + LZ4).
    println!("cargo:rustc-link-lib=z");
    println!("cargo:rustc-link-lib=lz4");
}
