#pragma once

#include "json.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace json {

  class FinalContext;

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

  }  // namespace detail

  class FinalContext {
    std::shared_ptr<detail::BuilderCore> core_;
    
  public:
    explicit FinalContext(std::shared_ptr<detail::BuilderCore> core)
      : core_(std::move(core)) {
    }

    FinalContext(const FinalContext&) = delete;
    FinalContext& operator=(const FinalContext&) = delete;
    FinalContext(FinalContext&&) = default;
    FinalContext& operator=(FinalContext&&) = default;

    Node Build()&& {
      return core_->Build();
    }
  };

  class Builder {
    std::shared_ptr<detail::BuilderCore> core_;

    template <typename Parent>
    class DictItemContext;

    template <typename Parent>
    class KeyItemContext;

    template <typename Parent>
    class ArrayItemContext;

  public:
    Builder()
      : core_(std::make_shared<detail::BuilderCore>()) {
    }

    Builder(const Builder&) = delete;
    Builder& operator=(const Builder&) = delete;
    Builder(Builder&&) = default;
    Builder& operator=(Builder&&) = default;

    DictItemContext<FinalContext> StartDict()&&;
    ArrayItemContext<FinalContext> StartArray()&&;
    FinalContext Value(Node value)&&;

    template <typename Parent>
    class DictItemContext {
        std::shared_ptr<detail::BuilderCore> core_;
        Parent parent_;

    public:
        DictItemContext(std::shared_ptr<detail::BuilderCore> core, Parent parent)
            : core_(std::move(core))
            , parent_(std::move(parent)) {
        }

        DictItemContext(const DictItemContext&) = delete;
        DictItemContext& operator=(const DictItemContext&) = delete;
        DictItemContext(DictItemContext&&) = default;
        DictItemContext& operator=(DictItemContext&&) = default;

        KeyItemContext<Parent> Key(std::string key)&& {
            core_->SetKey(std::move(key));
            return KeyItemContext<Parent>(core_, std::move(parent_));
        }

        Parent EndDict()&& {
            core_->EndDict();
            return std::move(parent_);
        }

        Parent EndArray()&& {
            throw std::logic_error("EndArray() called while current context is dict");
        }
    };

    template <typename Parent>
    class KeyItemContext {
        std::shared_ptr<detail::BuilderCore> core_;
        Parent parent_;

    public:
        KeyItemContext(std::shared_ptr<detail::BuilderCore> core, Parent parent)
            : core_(std::move(core))
            , parent_(std::move(parent)) {
        }

        KeyItemContext(const KeyItemContext&) = delete;
        KeyItemContext& operator=(const KeyItemContext&) = delete;
        KeyItemContext(KeyItemContext&&) = default;
        KeyItemContext& operator=(KeyItemContext&&) = default;

        DictItemContext<Parent> Value(Node value)&& {
            core_->AddValue(std::move(value));
            return DictItemContext<Parent>(core_, std::move(parent_));
        }

        DictItemContext<DictItemContext<Parent>> StartDict()&& {
            DictItemContext<Parent> parent_ctx(core_, std::move(parent_));
            core_->StartDict();
            return DictItemContext<DictItemContext<Parent>>(core_, std::move(parent_ctx));
        }

        ArrayItemContext<DictItemContext<Parent>> StartArray()&& {
            DictItemContext<Parent> parent_ctx(core_, std::move(parent_));
            core_->StartArray();
            return ArrayItemContext<DictItemContext<Parent>>(core_, std::move(parent_ctx));
        }
    };

    template <typename Parent>
    class ArrayItemContext {
        std::shared_ptr<detail::BuilderCore> core_;
        Parent parent_;

    public:
        ArrayItemContext(std::shared_ptr<detail::BuilderCore> core, Parent parent)
            : core_(std::move(core))
            , parent_(std::move(parent)) {
        }

        ArrayItemContext(const ArrayItemContext&) = delete;
        ArrayItemContext& operator=(const ArrayItemContext&) = delete;
        ArrayItemContext(ArrayItemContext&&) = default;
        ArrayItemContext& operator=(ArrayItemContext&&) = default;

        ArrayItemContext Value(Node value)&& {
            core_->AddValue(std::move(value));
            return ArrayItemContext(core_, std::move(parent_));
        }

        DictItemContext<ArrayItemContext<Parent>> StartDict()&& {
            ArrayItemContext<Parent> parent_ctx(core_, std::move(parent_));
            core_->StartDict();
            return DictItemContext<ArrayItemContext<Parent>>(core_, std::move(parent_ctx));
        }

        ArrayItemContext<ArrayItemContext<Parent>> StartArray()&& {
            ArrayItemContext<Parent> parent_ctx(core_, std::move(parent_));
            core_->StartArray();
            return ArrayItemContext<ArrayItemContext<Parent>>(core_, std::move(parent_ctx));
        }

        Parent EndArray()&& {
            core_->EndArray();
            return std::move(parent_);
        }

        Parent EndDict()&& {
            throw std::logic_error("EndDict() called while current context is array");
        }
    };
  };

}  // namespace json