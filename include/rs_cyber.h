#pragma once

#include <condition_variable>
#include <deque>
#include <memory>

#include <cyber/message/py_message.h>
#include <cyber/message/raw_message.h>
#include <rust/cxx.h>

namespace apollo {
namespace cyber {

bool init(rust::Str module_name);
void deinit();
bool is_shutdown();

void register_messages(rust::Slice<const uint8_t> file_descriptor_set);

class Node;
template <typename T>
class Reader;
template <typename T>
class Writer;

class RsWriter {
 public:
  RsWriter(const std::string& channel, const std::string& type,
           const uint32_t qos_depth, Node* node);

  int write(rust::Vec<uint8_t> data) const;

 private:
  std::string channel_name_;
  std::string data_type_;
  uint32_t qos_depth_;
  Node* node_ = nullptr;
  std::shared_ptr<Writer<message::PyMessageWrap>> writer_;
};

class RsReader {
 public:
  RsReader(const std::string& channel, const std::string& type, Node* node);

  void register_func(rust::Fn<int32_t(rust::Slice<const uint8_t>)> func) {
    function_registered_ = true;
    func_ = func;
  }

 private:
  void cb(const std::shared_ptr<const message::PyMessageWrap>& message);
  void cb_rawmsg(const std::shared_ptr<const message::RawMessage>& message);

  bool function_registered_ = false;
  std::string channel_name_;
  std::string data_type_;
  Node* node_ = nullptr;
  rust::Fn<int32_t(rust::Slice<const uint8_t>)> func_;
  std::shared_ptr<Reader<message::PyMessageWrap>> reader_ = nullptr;
  std::deque<std::string> cache_;
  std::mutex msg_lock_;

  std::shared_ptr<Reader<message::RawMessage>> reader_rawmsg_ = nullptr;
};

class RsNode {
 public:
  explicit RsNode(const std::string& node_name);

  void shutdown();

  std::unique_ptr<RsWriter> create_writer(rust::Str channel, rust::Str type,
                                          uint32_t qos_depth);

  std::unique_ptr<RsReader> create_reader(rust::Str channel, rust::Str type,
                                          rust::Fn<int32_t(rust::Slice<const uint8_t>)> func);

  std::shared_ptr<Node> get_node() { return node_; }

 private:
  std::string node_name_;
  std::shared_ptr<Node> node_ = nullptr;
};

std::unique_ptr<RsNode> new_node(rust::Str node_name);

}  // namespace cyber
}  // namespace apollo
