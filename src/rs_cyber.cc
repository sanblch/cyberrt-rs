#include "cyberrt-rs/include/rs_cyber.h"

#include <cyber/cyber.h>
#include <cyber/init.h>
#include <cyber/message/protobuf_factory.h>
#include <cyber/node/node.h>
#include <google/protobuf/descriptor.h>

#include "cyberrt-rs/src/cyber.rs.h"

namespace apollo {
namespace cyber {

const char RAWDATATYPE[] = "RawData";

bool init(rust::Str module_name) { return Init(module_name.data()); }

void deinit() { Clear(); }

bool is_shutdown() { return IsShutdown(); }

void register_messages(rust::Slice<const uint8_t> file_descriptor_set) {
  google::protobuf::FileDescriptorSet set;
  set.ParseFromString(
      std::string(reinterpret_cast<const char*>(file_descriptor_set.data())));
  for (unsigned i = 0; i < set.file_size(); ++i) {
    message::ProtobufFactory::Instance()->RegisterMessage(set.file(i));
  }
}

RsWriter::RsWriter(const std::string& channel, const std::string& type,
                   const uint32_t qos_depth, Node* node)
    : channel_name_(channel),
      data_type_(type),
      qos_depth_(qos_depth),
      node_(node) {
  std::string proto_desc;
  message::ProtobufFactory::Instance()->GetDescriptorString(type, &proto_desc);
  if (proto_desc.empty()) {
    AWARN << "cpp can't find proto_desc msgtype->" << data_type_;
    return;
  }
  proto::RoleAttributes role_attr;
  role_attr.set_channel_name(channel_name_);
  role_attr.set_message_type(data_type_);
  role_attr.set_proto_desc(proto_desc);
  auto qos_profile = role_attr.mutable_qos_profile();
  qos_profile->set_depth(qos_depth_);
  writer_ = node_->CreateWriter<message::PyMessageWrap>(role_attr);
}

int RsWriter::write(rust::Vec<uint8_t> data) const {
  auto message = std::make_shared<message::PyMessageWrap>(
      std::string(reinterpret_cast<const char*>(data.data()), data.size()),
      data_type_);
  message->set_type_name(data_type_);
  return writer_->Write(message);
}

RsReader::RsReader(const std::string& channel, const std::string& type,
                   Node* node)
    : channel_name_(channel), data_type_(type), node_(node) {
  if (data_type_.compare(RAWDATATYPE) == 0) {
    auto f =
        [this](const std::shared_ptr<const message::PyMessageWrap>& request) {
          this->cb(request);
        };
    reader_ = node_->CreateReader<message::PyMessageWrap>(channel, f);
  } else {
    auto f = [this](const std::shared_ptr<const message::RawMessage>& request) {
      this->cb_rawmsg(request);
    };
    reader_rawmsg_ = node_->CreateReader<message::RawMessage>(channel, f);
  }
}

rust::Slice<const uint8_t> str2slice(const std::string& str) {
  rust::Slice<const uint8_t> res(reinterpret_cast<const uint8_t*>(&str[0]),
                                 str.size());
  return res;
}

void RsReader::cb(
    const std::shared_ptr<const message::PyMessageWrap>& message) {
  {
    std::lock_guard<std::mutex> lg(msg_lock_);
    cache_.push_back(message->data());
    if (function_registered_) {
      func_(str2slice(message->data()));
    }
  }
}

void RsReader::cb_rawmsg(
    const std::shared_ptr<const message::RawMessage>& message) {
  {
    std::lock_guard<std::mutex> lg(msg_lock_);
    cache_.push_back(message->message);
    if (function_registered_) {
      func_(str2slice(message->message));
    }
  }
}

RsNode::RsNode(const std::string& node_name) : node_name_(node_name) {
  node_ = CreateNode(node_name);
}

void RsNode::shutdown() {
  node_.reset();
  AINFO << "RsNode " << node_name_ << " exit.";
}

std::unique_ptr<RsWriter> RsNode::create_writer(rust::Str channel,
                                                rust::Str type,
                                                uint32_t qos_depth) {
  if (node_) {
    return std::make_unique<RsWriter>(static_cast<std::string>(channel),
                                      static_cast<std::string>(type), qos_depth,
                                      node_.get());
  }
  AINFO << "RsNode: node_ is null, new RsWriter failed!";
  return std::unique_ptr<RsWriter>();
}

std::unique_ptr<RsReader> RsNode::create_reader(
    rust::Str channel, rust::Str type,
    rust::Fn<int32_t(rust::Slice<const uint8_t>)> func) {
  if (node_) {
    auto ret =
        std::make_unique<RsReader>(static_cast<std::string>(channel),
                                   static_cast<std::string>(type), node_.get());
    ret->register_func(func);
    return ret;
  }
  AINFO << "RsNode: node_ is null, new RsReader failed!";
  return std::unique_ptr<RsReader>();
}

std::unique_ptr<RsNode> new_node(rust::Str node_name) {
  return std::make_unique<RsNode>(static_cast<std::string>(node_name));
}

}  // namespace cyber
}  // namespace apollo
