#ifndef _RTC_INVOKER_H_INCLUDED
#define _RTC_INVOKER_H_INCLUDED

#include <future>
#include <memory>

namespace util {

template<typename C>
class Invoker {
public:
    template<typename Fn>
    void Post(Fn&& fn) {
        impl()->PostImpl(std::forward<Fn>(fn));
    }

    template<typename Fn>
    void Dispatch(Fn&& fn) {
        this->InThisThread() ? (void)fn() : this->Post(std::forward<Fn>(fn));
    }

    template<typename Fn, typename ... Args>
    auto Invoke(Fn&& fn, Args&& ... args) -> decltype(fn(args...)) {
        if (InThisThread()) {
            return fn(std::forward<Args>(args)...);
        }

        std::promise<decltype(fn(args...))> promise;
        this->Post([&] {
            promise.set_value(fn(std::forward<Args>(args)...));
        });

        return promise.get_future().get();
    }

    template<typename T, typename ... Args>
    std::shared_ptr<T> CreateShared(Args&& ... args) {
        return {
            this->Invoke([&] {
                return new T(std::forward<Args>(args)...);
            }),

            [this](auto p) {
                this->Post([p] { delete p; });
            }
        };
    }

    bool InThisThread() const {
        return resip::ThreadIf::selfId() == impl()->tid();
    }
private:
    C *impl() {
        return static_cast<C *>(this);
    }

    const C *impl() const {
        return static_cast<const C *>(this);
    }
};
}

#endif //_RTC_INVOKER_H_INCLUDED
