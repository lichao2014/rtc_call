#ifndef _RTC_CALLBACK_WRAPPER_H_INCLUDED
#define _RTC_CALLBACK_WRAPPER_H_INCLUDED

#include <memory>
#include "utility/optional.h"

namespace util {

template<typename Fn>
struct return_of;

template<typename R, typename C, typename ... Args>
struct return_of<R(C::*)(Args...)> {
    using type = R;
};

template<typename Fn>
using return_of_t = typename return_of<Fn>::type;

template<typename C>
class CallbackWrapper : private std::weak_ptr<C> {
public:
    using std::weak_ptr<C>::weak_ptr;

    template<typename Fn, typename ... Args>
    auto operator ()(Fn&& fn, Args&& ... args)
        -> std::enable_if_t<std::is_same_v<return_of_t<Fn>, void>, void> {
        auto safe_callback = lock();
        if (safe_callback) {
            (safe_callback.get()->*fn)(std::forward<Args>(args)...);
        }
    }

    template<typename Fn, typename ... Args>
    auto operator ()(Fn&& fn, Args&& ... args)
        -> std::enable_if_t<
        !std::is_same_v<return_of_t<Fn>, void>,
        Optional<return_of_t<Fn>>> {
        auto safe_callback = lock();
        if (!safe_callback) {
            return {};
        }

        return (safe_callback.get()->*fn)(std::forward<Args>(args)...);
    }
};
}

#endif // !_RTC_CALLBACK_WRAPPER_H_INCLUDED
