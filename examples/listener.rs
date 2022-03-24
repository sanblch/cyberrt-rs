use cyberrt_rs::cyber;
use prost::Message;
use std::error::Error;

pub mod unittest {
    include!(concat!(env!("OUT_DIR"), "/apollo.cyber.proto.rs"));
}

fn callback(data: &[u8]) -> i32 {
    println!("================================================================================");
    println!("py:reader callback msg->:");
    println!("{:#?}", unittest::ChatterBenchmark::decode(data).unwrap());
    println!("================================================================================");
    0
}

fn main() -> Result<(), Box<dyn Error>> {
    cyber::init("listener");

    let mut node = cyber::Node::new("listener");
    let mut _reader = node.create_reader(
        "channel/chatter",
        "apollo.cyber.proto.ChatterBenchmark",
        callback,
    );

    node.spin();

    cyber::deinit();

    Ok(())
}
