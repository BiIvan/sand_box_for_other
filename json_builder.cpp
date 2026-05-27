#include "json_builder.h"

#include <stdexcept>
#include <utility>

using namespace std::literals;

namespace json {
  namespace detail {

    Node& BuilderCore::AddNode(Node node) {
      if (!has_root_) {
        root_ = std::move(node);
        has_root_ = true;
        return root_;
      }

      if (nodes_stack_.empty()) {
        throw std::logic_error("JSON root has already been built"s);
      }

      auto& top = nodes_stack_.back();

      if (top.type == ContainerType::Array) {
        auto& arr = top.node->AsArray();
        arr.push_back(std::move(node));
        return arr.back();
      }

      if (!current_key_) {
        throw std::logic_error("Key is expected before inserting into dict"s);
      }

      auto& dict = top.node->AsDict();
      auto [it, inserted] = dict.emplace(std::move(*current_key_), std::move(node));
      current_key_.reset();
      return it->second;
    }

    void BuilderCore::SetKey(std::string key) {
      if (!InDict()) {
        throw std::logic_error("Key() can be called only inside dict"s);
      }
      if (current_key_) {
        throw std::logic_error("Previous key has no value yet"s);
      }
      current_key_ = std::move(key);
    }

    void BuilderCore::StartDict() {
      Node& node = AddNode(Dict{});
      nodes_stack_.push_back({ &node, ContainerType::Dict });
    }

    void BuilderCore::StartArray() {
      Node& node = AddNode(Array{});
      nodes_stack_.push_back({ &node, ContainerType::Array });
    }

    void BuilderCore::AddValue(Node value) {
      AddNode(std::move(value));
    }

    void BuilderCore::EndDict() {
      if (!InDict()) {
        throw std::logic_error("EndDict() without opened dict"s);
      }
      if (current_key_) {
        throw std::logic_error("Cannot end dict: key without value"s);
      }
      nodes_stack_.pop_back();
    }

    void BuilderCore::EndArray() {
      if (!InArray()) {
        throw std::logic_error("EndArray() without opened array"s);
      }
      nodes_stack_.pop_back();
    }

    Node BuilderCore::Build() const {
      if (!has_root_) {
        throw std::logic_error("Cannot build empty document"s);
      }
      if (!nodes_stack_.empty()) {
        throw std::logic_error("Cannot build: some containers are not closed"s);
      }
      if (current_key_) {
        throw std::logic_error("Cannot build: key without value"s);
      }
      return root_;
    }

  }  // namespace detail

  Builder::DictItemContext<FinalContext> Builder::StartDict()&& {
    core_->StartDict();
    return Builder::DictItemContext<FinalContext>(core_, FinalContext(core_));
  }

  Builder::ArrayItemContext<FinalContext> Builder::StartArray()&& {
    core_->StartArray();
    return Builder::ArrayItemContext<FinalContext>(core_, FinalContext(core_));
  }

  FinalContext Builder::Value(Node value)&& {
    core_->AddValue(std::move(value));
    return FinalContext(core_);
  }

}  // namespace json