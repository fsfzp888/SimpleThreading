# SimpleThreading

This project is just a demostration of using C++11 to implement simple thread runner. In many real C++ projects, it's almost impossible to make the whole project parallel since the main routine is designed and implemented to be single thread. However, we could still use multi-thread feature of C++11 to accelerate program partially, this simple thread runner should help to do this.

## features

1. Custimized IdxSeq, GenSeq to prevent using C++14 std::make_index_sequence, etc...
2. implement function closure using tuple to prevent parameter pack expansion in lambda body, since GCC 4.8([BUG][1]) do not support this.

## usage
Copy thread.h to project and write some thing like that:
```c++
#include "threading.h"
#include <iostream>
using namespace std;
using namespace simple_threading;

class ABC {
  public:
    void heavy_method(size_t times, int job_id) {
        // cout << times << endl;
        size_t val = 0;
        for (size_t i = 0; i < times; ++i) {
            val += i * i * i;
        }
        cout << std::this_thread::get_id() << ": finish jobs" << job_id << " and val is:" << val << endl;
    }
};

size_t test_method(size_t times, int job_id) {
    size_t val = 0;
    for (size_t i = 0; i < times; ++i) {
        val += i * i * i;
    }
    cout << std::this_thread::get_id() << ": finish jobs" << job_id << " and val is:" << val << endl;
    return val;
}

int main() {
    SimpleThreadRunner sr(2);
    ABC abc;
    sr.AddTask(ThreadFunc(&test_method, 101233532, 9), 1);
    sr.AddTask(ThreadFunc(&test_method, 101232212, 10), 1);
    sr.AddTask(ThreadFunc(&test_method, 101235512, 11), 1);
    sr.AddTask(ThreadFunc(&test_method, 101236612, 12), 1);
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 100000000, 1));
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 104000000, 2), 10);
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 110000000, 3), 8);
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 100300000, 4));
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 100000010, 5));
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 101000000, 6), 10);
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 110000000, 7), 8);
    sr.AddTask(ThreadFunc(&ABC::heavy_method, abc, 100100000, 8));
    sr.Start();
    sr.Join();
    cout << "end" << endl;
    return 0;
}
```
The main class to implement closure:
```c++
template <typename Ret, typename Class, typename... Params> struct ClassFunctor {
    ClassFunctor(Ret (Class::*func)(Params...), Class &obj)
        : _func(func)
        , _obj(obj) {}
    Ret operator()(const Params &... param) const { return (_obj.*_func)(param...); }
    Ret (Class::*_func)(Params...);
    Class &_obj;
};

template <typename Ret, typename Class, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (Class::*func)(Params...), Class &obj, Params2 &&... param) {
    auto f = ClassFunctor<Ret, Class, Params...>(func, obj);
    return [f, tp = std::make_tuple(std::forward<Params2>(param)...)] { ApplyThreadFunc(f, tp); };
}
```
Indeed, we could write:
```c++
template <typename Ret, typename Class, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (Class::*func)(Params...), Class &obj, Params2 &&... param) {
    return [&, func] { return (obj.*func)(std::forward<Params>(param)...) };
}
```
However, GCC 4.8 do not support above code. We could also use
```c++
template <typename Ret, typename Class, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (Class::*func)(Params...), Class &obj, Params2 &&... param) {
    auto f = ClassFunctor<Ret, Class, Params...>(func, obj);
    return [f, tp = std::make_tuple(std::forward<Params2>(param)...)] { std::apply(f, tp); };
}
```
to implement closure. But the std::apply the interface is defined in C++17.

[1]: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55914
