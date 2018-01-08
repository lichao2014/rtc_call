#ifndef _RTC_UNIQUE_PTR_H_INCLUDED
#define _RTC_UNIQUE_PTR_H_INCLUDED

#include <memory>
#include <functional>

namespace util {

template<typename T, typename R = void>
using UniquePtr = std::unique_ptr<T, std::function<R(T*)>>;
}

#endif // !_RTC_UNIQUE_PTR_H_INCLUDED
