use core::time;
use cyberrt_rs::cyber;
use prost::Message;
use std::{error::Error, thread};

pub mod unittest {
    include!(concat!(env!("OUT_DIR"), "/apollo.cyber.proto.rs"));
}

fn main() -> Result<(), Box<dyn Error>> {
    cyber::init("talker");

    let mut msg = unittest::ChatterBenchmark::default();
    msg.content = Some("rs:talker:send Alex!".to_string());
    msg.stamp = Some(9999);
    msg.seq = Some(0);

    println!("{:#?}", msg);

    let mut node = cyber::Node::new("node_name1");

    let mut g_count = 1;

    let writer = node.create_writer("channel/chatter", "apollo.cyber.proto.ChatterBenchmark", 0);
    writer.write(msg.encode_to_vec());

    while !cyber::is_shutdown() {
        thread::sleep(time::Duration::from_secs(1));
        g_count = g_count + 1;
        msg.seq = Some(g_count);
        msg.content = Some("I am rust talker.".to_string());
        println!("======================================================");
        println!("write msg -> {:#?}", msg);
        writer.write(msg.encode_to_vec());
    }

    cyber::deinit();

    Ok(())
}
