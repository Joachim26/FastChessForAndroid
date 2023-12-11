#pragma once

#include <atomic>
#include <type_traits>

class ScopeEntry {
   public:
    ScopeEntry(bool available) : available_(available) {}

    void release() noexcept { available_ = true; }

   protected:
    std::atomic<bool> available_;
};

template <typename T>

class ScopeGuard {
   public:
    explicit ScopeGuard(T &entry) : entry_(entry) {
        static_assert(std::is_base_of<ScopeEntry, T>::value,
                      "type parameter of this class must derive from ScopeEntry");
    }

    ~ScopeGuard() { entry_.release(); }

    ScopeGuard(const ScopeGuard &)            = delete;
    ScopeGuard &operator=(const ScopeGuard &) = delete;

    [[nodiscard]] auto &get() noexcept { return entry_; }
    [[nodiscard]] auto &get() const noexcept { return entry_; }

   private:
    T &entry_;
};
