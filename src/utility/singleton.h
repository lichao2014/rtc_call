#ifndef _RTC_SINGLETON_H_INCLUDED
#define _RTC_SINGLETON_H_INCLUDED

namespace util {

template<typename T>
class Singleton {
public:
    static T& Instance() {
        static T _instance;
        return _instance;
    }
};
}

#endif // !_RTC_SINGLETON_H_INCLUDED
