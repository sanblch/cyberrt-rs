use core::time;
use std::thread;

#[cxx::bridge(namespace = "apollo::cyber")]
mod ffi {
    unsafe extern "C++" {
        include!("cyberrt-rs/include/rs_cyber.h");

        fn init(module_name: &str) -> bool;
        fn deinit();
        fn is_shutdown() -> bool;

        fn register_messages(file_descriptor_set: &[u8]);

        type RsNode;
        fn new_node(node_name: &str) -> UniquePtr<RsNode>;
        fn create_reader(
            self: Pin<&mut RsNode>,
            channel: &str,
            data_type: &str,
            func: fn(&[u8]) -> i32,
        ) -> UniquePtr<RsReader>;
        fn create_writer(
            self: Pin<&mut RsNode>,
            channel: &str,
            data_type: &str,
            qos_depth: u32,
        ) -> UniquePtr<RsWriter>;

        type RsReader;

        type RsWriter;
        fn write(self: &RsWriter, data: Vec<u8>) -> i32;
    }
}

pub fn init(module_name: &str) -> bool {
    let res = ffi::init(module_name);

    let file_descriptor_set_bytes =
        include_bytes!(concat!(env!("OUT_DIR"), "/file_descriptor_set.bin"));
    ffi::register_messages(file_descriptor_set_bytes);

    res
}

pub fn deinit() {
    ffi::deinit();
}

pub fn is_shutdown() -> bool {
    ffi::is_shutdown()
}

pub struct Node {
    node: cxx::UniquePtr<ffi::RsNode>,
}

impl Node {
    pub fn new(name: &str) -> Self {
        Node {
            node: ffi::new_node(name),
        }
    }

    pub fn create_reader(
        &mut self,
        channel: &str,
        data_type: &str,
        func: fn(&[u8]) -> i32,
    ) -> Reader {
        Reader {
            _reader: self.node.pin_mut().create_reader(channel, data_type, func),
        }
    }

    pub fn create_writer(&mut self, channel: &str, data_type: &str, qos_depth: u32) -> Writer {
        Writer {
            _name: channel.to_string(),
            writer: self
                .node
                .pin_mut()
                .create_writer(channel, data_type, qos_depth),
            _data_type: data_type.to_string(),
        }
    }

    pub fn spin(&self) {
        while !is_shutdown() {
            thread::sleep(time::Duration::from_secs_f32(0.002));
        }
    }
}

pub struct Reader {
    _reader: cxx::UniquePtr<ffi::RsReader>,
}

pub struct Writer {
    _name: String,
    writer: cxx::UniquePtr<ffi::RsWriter>,
    _data_type: String,
}

impl Writer {
    pub fn write(&self, data: Vec<u8>) -> i32 {
        self.writer.as_ref().unwrap().write(data)
    }
}
