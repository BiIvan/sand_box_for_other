#pragma once
//2026.05.27_23:20
#include "json.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace json {

  class Context;
  class DictItemContext;
  class KeyItemContext;
  class ArrayItemContext;

  namespace detail {

    enum class ContainerType {
      Dict,
      Array
    };

    struct ContainerContext {
      Node* node = nullptr;
      ContainerType type = ContainerType::Dict;
    };

    class BuilderCore {
      bool InDict() const;
      bool InArray() const;

    public:
      Node root_ = nullptr;
      bool has_root_ = false;
      std::vector<ContainerContext> nodes_stack_;
      std::optional<std::string> current_key_;

      Node& AddNode(Node node);
      void SetKey(std::string key);
      void StartDict();
      void StartArray();
      void AddValue(Node value);
      void EndDict();
      void EndArray();
      Node Build() const;
    };

    struct IParent {
      virtual ~IParent() = default;
      virtual Context Return(std::shared_ptr<BuilderCore> core) = 0;
    };

    template <typename T>
    class ParentModel final : public IParent {
    public:
      explicit ParentModel(T value)
        : value_(std::move(value)) {
      }

      Context Return(std::shared_ptr<BuilderCore> core) override;

    private:
      T value_;
    };

  }  // namespace detail

  class Context {
  public:
    explicit Context(std::shared_ptr<detail::BuilderCore> core)
      : core_(std::move(core)) {
    }

    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = default;
    Context& operator=(Context&&) = default;

    Context Key(std::string key);
    Context Value(Node value);
    Context StartDict();
    Context StartArray();
    Context EndDict();
    Context EndArray();
    Node Build();

  private:
    std::shared_ptr<detail::BuilderCore> core_;
  };

  class DictItemContext {
  public:
    DictItemContext(std::shared_ptr<detail::BuilderCore> core,
      std::unique_ptr<detail::IParent> parent)
      : core_(std::move(core))
      , parent_(std::move(parent)) {
    }

    DictItemContext(const DictItemContext&) = delete;
    DictItemContext& operator=(const DictItemContext&) = delete;
    DictItemContext(DictItemContext&&) = default;
    DictItemContext& operator=(DictItemContext&&) = default;

    KeyItemContext Key(std::string key);
    Context EndDict();

  private:
    std::shared_ptr<detail::BuilderCore> core_;
    std::unique_ptr<detail::IParent> parent_;
  };

  class KeyItemContext {
  public:
    KeyItemContext(std::shared_ptr<detail::BuilderCore> core,
      std::unique_ptr<detail::IParent> parent)
      : core_(std::move(core))
      , parent_(std::move(parent)) {
    }

    KeyItemContext(const KeyItemContext&) = delete;
    KeyItemContext& operator=(const KeyItemContext&) = delete;
    KeyItemContext(KeyItemContext&&) = default;
    KeyItemContext& operator=(KeyItemContext&&) = default;

    DictItemContext Value(Node value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();

  private:
    std::shared_ptr<detail::BuilderCore> core_;
    std::unique_ptr<detail::IParent> parent_;
  };

  class ArrayItemContext {
  public:
    ArrayItemContext(std::shared_ptr<detail::BuilderCore> core,
      std::unique_ptr<detail::IParent> parent)
      : core_(std::move(core))
      , parent_(std::move(parent)) {
    }

    ArrayItemContext(const ArrayItemContext&) = delete;
    ArrayItemContext& operator=(const ArrayItemContext&) = delete;
    ArrayItemContext(ArrayItemContext&&) = default;
    ArrayItemContext& operator=(ArrayItemContext&&) = default;

    ArrayItemContext Value(Node value);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Context EndArray();

  private:
    std::shared_ptr<detail::BuilderCore> core_;
    std::unique_ptr<detail::IParent> parent_;
  };

  class Builder {
  public:
    Builder()
      : core_(std::make_shared<detail::BuilderCore>()) {
    }

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;
    Builder(Builder&&) = default;
    Builder& operator=(Builder&&) = default;

    DictItemContext StartDict();
    ArrayItemContext StartArray();
    Context Value(Node value);

  private:
    std::shared_ptr<detail::BuilderCore> core_;
  };

}  // namespace json