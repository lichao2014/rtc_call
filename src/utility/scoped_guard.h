#ifndef _RTC_SCOPED_GUARD_H_INCLUDED
#define _RTC_SCOPED_GUARD_H_INCLUDED

namespace util {

template<typename Fn>
class ScopedGuard {
public:
    ScopedGuard(Fn&& fn) : fn_(std::forward<Fn>(fn)) {}

    ~ScopedGuard() {
        if (!deposed_) {
            fn_();
        }
    }

    void depose() { deposed_ = true; }
private:
    Fn fn_;
    bool deposed_ = false;
};

template<typename Fn>
ScopedGuard<Fn> MakeScopedGuard(Fn&& fn) {
    return std::forward<Fn>(fn);
}
}

#endif // !_RTC_SCOPED_GUARD_H_INCLUDED
