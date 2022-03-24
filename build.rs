use std::{env, path::PathBuf};

fn main() {
    let include_path = "/usr/local/include/apollo";
    let rtps_include_path = "/usr/local/fast-rtps/include";
    let cyber_lib_path = "/usr/local/lib/cyber";
    let lib_path = "/usr/local/lib";
    let rtps_lib_path = "/usr/local/fast-rtps/lib";

    cxx_build::bridge("src/cyber.rs")
        .include(include_path)
        .include(rtps_include_path)
        .file("src/rs_cyber.cc")
        .flag_if_supported("-std=c++17")
        .define("CMAKE_BUILD", None)
        .compile("cyberrt-rs");

    println!("cargo:rustc-link-search={}", cyber_lib_path);
    println!("cargo:rustc-link-search={}", lib_path);
    println!("cargo:rustc-link-search={}", rtps_lib_path);
    println!("cargo:rustc-link-lib=core");
    println!("cargo:rustc-link-lib=fastrtps");
    println!("cargo:rustc-link-lib=glogd");
    println!("cargo:rustc-link-lib=gflags_nothreads_debug");

    let mut config = prost_build::Config::new();
    config.file_descriptor_set_path(
        PathBuf::from(env::var("OUT_DIR").expect("OUT_DIR environment variable not set"))
            .join("file_descriptor_set.bin"),
    );
    config
        .compile_protos(&["src/unit_test.proto"], &["src/"])
        .unwrap();
}
