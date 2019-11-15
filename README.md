# SimpleThreading
## 简介
当前项目是个c++11线程thread的简单封装，包含一个简单的优先任务队列和一个多线程执行器类。在实际项目中，通常都不是并行的，在需要并行的时候才会启动多个线程同时执行任务，这个工程就是为这个目的而设计的。实现非常简单，只是为了只使用c++11语法，同时兼容gcc4.8等相对较老的编译器，实现了如下功能以避免直接依赖于编译器特性和c++14,c++17的接口：
1. 自定义实现了IdxSeq，GenSeq取代c++14的std::make_index_sequence\<N>等接口
2. 自行实现了闭包在避免再lambda body中进行parameter pack expansion，因为gcc 4.8存在[BUG][1]

实现没有太多内容，可以作为学习c++的素材
## 用法
使用时把threading.h拷贝到工程目录即可
示例：
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
实现闭包的部分：
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
其实只需要：
```c++
template <typename Ret, typename Class, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (Class::*func)(Params...), Class &obj, Params2 &&... param) {
    return [&, func] { return (obj.*func)(std::forward<Params>(param)...) };
}
```
但是很可惜，老点的编译器gcc 4.8不支持这样，所以只能自行打包。当然，上边也可以使用：
```c++
template <typename Ret, typename Class, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (Class::*func)(Params...), Class &obj, Params2 &&... param) {
    auto f = ClassFunctor<Ret, Class, Params...>(func, obj);
    return [f, tp = std::make_tuple(std::forward<Params2>(param)...)] { std::apply(f, tp); };
}
```
只是std::apply是c++17的接口

[1]: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=55914
