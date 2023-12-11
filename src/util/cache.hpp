#pragma once

#include <atomic>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>

#include <util/scope_guard.hpp>

template <typename T, typename ID>
class CachedEntry : public ScopeEntry {
   public:
    template <typename... ARGS>
    CachedEntry(const ID &identifier, ARGS &&...arg)
        : ScopeEntry(false), entry_(std::forward<ARGS>(arg)...), id(identifier) {}

    [[nodiscard]] T &get() noexcept { return entry_; }

   private:
    T entry_;
    ID id;

    template <typename TT, typename II>
    friend class CachePool;
};

/// @brief CachePool is a pool of objects which can be reused. An object is indentified by an ID
/// type and value. The pool is thread safe.
/// @tparam T
/// @tparam ID
template <typename T, typename ID>
class CachePool {
   public:
    template <typename... ARGS>
    [[nodiscard]] CachedEntry<T, ID> &getEntry(const ID &identifier, ARGS &&...arg) {
        std::lock_guard<std::mutex> lock(access_mutex_);

        for (auto &entry : cache_) {
            if (entry.available_ && entry.id == identifier) {
                entry.available_ = false;
                return entry;
            }
        }

        cache_.emplace_back(identifier, std::forward<ARGS>(arg)...);
        return cache_.back();
    }

   private:
    std::deque<CachedEntry<T, ID>> cache_;
    std::mutex access_mutex_;
};
