#include <thread>
#include <chrono>
#include <random>

#include "rutil/ThreadIf.hxx"

void SleepForRandomSec() {
    std::default_random_engine e;
    std::uniform_int_distribution<> u(0, 5);
    std::this_thread::sleep_for(std::chrono::seconds(u(e)));
}

class MyThread : public resip::ThreadIf {
public:

private:
    void thread() override {
        while (!isShutdown()) {
            SleepForRandomSec();
        }
    }
};

int main() {
    MyThread th;

    while (true) {
        th.run();
        SleepForRandomSec();
        th.shutdown();
        th.join();
    }
}