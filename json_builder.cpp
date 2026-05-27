//2026.05.27_23:20
#include "json_builder.h"

#include <stdexcept>
#include <utility>

using namespace std::literals;

namespace json {
  namespace detail {

    bool BuilderCore::InDict() const {
      return !nodes_stack_.empty() && nodes_stack_.back().type == ContainerType::Dict;
    }

    bool BuilderCore::InArray() const {
      return !nodes_stack_.empty() && nodes_stack_.back().type == ContainerType::Array;
    }

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
      (void)inserted;
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

    template <typename T>
    Context ParentModel<T>::Return(std::shared_ptr<BuilderCore>) {
      return std::move(value_);
    }

    template Context ParentModel<Context>::Return(std::shared_ptr<BuilderCore>);

  }  // namespace detail

  Context Context::Key(std::string key) {
    core_->SetKey(std::move(key));
    return Context(core_);
  }

  Context Context::Value(Node value) {
    core_->AddValue(std::move(value));
    return Context(core_);
  }

  Context Context::StartDict() {
    core_->StartDict();
    return Context(core_);
  }

  Context Context::StartArray() {
    core_->StartArray();
    return Context(core_);
  }

  Context Context::EndDict() {
    core_->EndDict();
    return Context(core_);
  }

  Context Context::EndArray() {
    core_->EndArray();
    return Context(core_);
  }

  Node Context::Build() {
    return core_->Build();
  }

  KeyItemContext DictItemContext::Key(std::string key) {
    core_->SetKey(std::move(key));
    return KeyItemContext(core_, std::move(parent_));
  }

  Context DictItemContext::EndDict() {
    core_->EndDict();
    return std::move(*parent_).Return(core_);
  }

  DictItemContext KeyItemContext::Value(Node value) {
    core_->AddValue(std::move(value));
    return DictItemContext(core_, std::move(parent_));
  }

  DictItemContext KeyItemContext::StartDict() {
    auto parent = std::make_unique<detail::ParentModel<Context>>(Context(core_));
    core_->StartDict();
    return DictItemContext(core_, std::move(parent));
  }

  ArrayItemContext KeyItemContext::StartArray() {
    auto parent = std::make_unique<detail::ParentModel<Context>>(Context(core_));
    core_->StartArray();
    return ArrayItemContext(core_, std::move(parent));
  }

  ArrayItemContext ArrayItemContext::Value(Node value) {
    core_->AddValue(std::move(value));
    return ArrayItemContext(core_, std::move(parent_));
  }

  DictItemContext ArrayItemContext::StartDict() {
    auto parent = std::make_unique<detail::ParentModel<Context>>(Context(core_));
    core_->StartDict();
    return DictItemContext(core_, std::move(parent));
  }

  ArrayItemContext ArrayItemContext::StartArray() {
    auto parent = std::make_unique<detail::ParentModel<Context>>(Context(core_));
    core_->StartArray();
    return ArrayItemContext(core_, std::move(parent));
  }

  Context ArrayItemContext::EndArray() {
    core_->EndArray();
    return std::move(*parent_).Return(core_);
  }

  DictItemContext Builder::StartDict() {
    core_->StartDict();
    return DictItemContext(
      core_,
      std::make_unique<detail::ParentModel<Context>>(Context(core_))
    );
  }

  ArrayItemContext Builder::StartArray() {
    core_->StartArray();
    return ArrayItemContext(
      core_,
      std::make_unique<detail::ParentModel<Context>>(Context(core_))
    );
  }

  Context Builder::Value(Node value) {
    core_->AddValue(std::move(value));
    return Context(core_);
  }

}  // namespace json