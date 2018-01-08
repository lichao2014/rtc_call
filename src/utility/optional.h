#ifndef _RTC_OPTIONAL_H_INCLUDED
#define _RTC_OPTIONAL_H_INCLUDED

namespace util {

template<typename T>
class Optional {
public:
    using type = T;

    Optional() : c_(0) {}

    ~Optional() {
        destory();
    }

    template<typename ... Args>
    Optional(Args&& ... args)
        : value_(std::forward<Args>(args)...)
        , has_value_(true) {}

    Optional(Optional<T>&& opt)
        : value_(std::move(opt.value_))
        , has_value_(opt.has_value_) {
        opt.has_value_ = false;
    }

    Optional(const Optional<T>& opt)
        : value_(opt.value_)
        , has_value_(opt.has_value_) {}

    template<typename U>
    Optional& operator=(U&& value) {
        emplace(std::forward<U>(value));
        return *this;
    }

    template<typename ... Args>
    void emplace(Args&& ... args) {
        destory();

        new (&value_) T(std::forward<Args>(args)...);
        has_value_ = true;
    }

    constexpr explicit operator bool() const noexcept {
        return has_value_;
    }

    constexpr  bool has_value() const noexcept {
        return has_value_;
    }

    constexpr const T& operator*() const& noexcept {
        return value_;
    }

    constexpr T& operator*()& noexcept {
        return value_;
    }

    constexpr T&& operator*()&& noexcept {
        return value_;
    }

    constexpr T *operator->() noexcept {
        return &value_;
    }

    constexpr const T *operator->() const noexcept {
        return &value_;
    }
private:
    void destory() {
        if (has_value_) {
            value_.~T();
        }
    }

    union {
        char c_;
        T value_; 
    };
    bool has_value_ = false;
};
}

#endif // !_RTC_OPTIONAL_H_INCLUDED
