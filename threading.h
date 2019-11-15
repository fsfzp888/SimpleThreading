/**
 * @brief Simple thread runner based on function closure and variadic parameters
 * packing.
 * @author zpfeng
 * @copyright (C) 2019-2020 zpfeng
 * @license BSD 3-CLAUSE
 */

#ifndef __SIMPLE_THREADING_H__
#define __SIMPLE_THREADING_H__

#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace simple_threading {

template <typename T> using Invoke = typename T::type;

template <size_t...> struct IdxSeq { using type = IdxSeq; };

template <class S1, class S2> struct Concat;

template <size_t... I1, size_t... I2>
struct Concat<IdxSeq<I1...>, IdxSeq<I2...>> : IdxSeq<I1..., (sizeof...(I1) + I2)...> {};

template <size_t N>
struct GenSeq : Invoke<Concat<Invoke<GenSeq<N / 2>>, Invoke<GenSeq<N - N / 2>>>> {};

template <> struct GenSeq<0> : IdxSeq<> {};
template <> struct GenSeq<1> : IdxSeq<0> {};

template <typename... Params> using IdxSeqFor = GenSeq<sizeof...(Params)>;

template <typename Callable, typename Tuple, size_t... I>
constexpr void CallThreadFunc(Callable &&f, Tuple &&t, IdxSeq<I...>) {
    f(std::get<I>(std::forward<Tuple>(t))...);
}

template <typename Callable, typename Tuple>
constexpr void ApplyThreadFunc(Callable &&f, Tuple &&t) {
    CallThreadFunc(
        std::forward<Callable>(f), std::forward<Tuple>(t),
        GenSeq<std::tuple_size<typename std::remove_reference<Tuple>::type>::value>());
}

template <typename Ret, typename... Params> struct Functor {
    Functor(Ret (*func)(Params...))
        : _func(func) {}
    Ret operator()(const Params &... param) const { return _func(param...); }
    Ret (*_func)(Params...);
};

template <typename Ret, typename Class, typename... Params> struct ClassFunctor {
    ClassFunctor(Ret (Class::*func)(Params...), Class &obj)
        : _func(func)
        , _obj(obj) {}
    Ret operator()(const Params &... param) const { return (_obj.*_func)(param...); }
    Ret (Class::*_func)(Params...);
    Class &_obj;
};

template <typename Ret, typename Class, typename... Params> struct ClassConstFunctor {
    ClassConstFunctor(Ret (Class::*func)(Params...) const, const Class &obj)
        : _func(func)
        , _obj(obj) {}
    Ret operator()(const Params &... param) const { return (_obj.*_func)(param...); }
    Ret (Class::*_func)(Params...) const;
    const Class &_obj;
};

template <typename Ret, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (*func)(Params...), Params2 &&... param) {
    auto f = Functor<Ret, Params...>(func);
    return [f, tp = std::make_tuple(std::forward<Params2>(param)...)] {
        ApplyThreadFunc(f, tp);
    };
}

template <typename Ret, typename Class, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (Class::*func)(Params...), Class &obj,
                                 Params2 &&... param) {
    auto f = ClassFunctor<Ret, Class, Params...>(func, obj);
    return [f, tp = std::make_tuple(std::forward<Params2>(param)...)] {
        ApplyThreadFunc(f, tp);
    };
}

template <typename Ret, typename Class, typename... Params, typename... Params2>
std::function<void()> ThreadFunc(Ret (Class::*func)(Params...) const, const Class &obj,
                                 Params2 &&... param) {
    auto f = ClassConstFunctor<Ret, Class, Params...>(func, obj);
    return [f, tp = std::make_tuple(std::forward<Params2>(param)...)] {
        ApplyThreadFunc(f, tp);
    };
}

template <typename B, typename D, typename... Params>
std::shared_ptr<B> MakeShared(Params &&... param) {
    return std::shared_ptr<B>(new D(std::forward<Params>(param)...));
}

struct ThreadFunctorBase {
    ThreadFunctorBase(int priority)
        : _priority(priority) {}
    virtual ~ThreadFunctorBase() noexcept = default;
    virtual void operator()() const = 0;
    int _priority;
    struct LessThan {
        bool operator()(const std::shared_ptr<ThreadFunctorBase> &lhs,
                        const std::shared_ptr<ThreadFunctorBase> &rhs) const {
            return lhs->_priority < rhs->_priority;
        }
    };
};

template <typename Callable> struct ThreadFunctorImpl : ThreadFunctorBase {
    ThreadFunctorImpl(Callable f, int priority = 0)
        : ThreadFunctorBase(priority)
        , _f(std::move(f)) {}
    void operator()() const override { _f(); }
    Callable _f;
};

class SimpleTaskQueue {
  public:
    using task_type = std::shared_ptr<ThreadFunctorBase>;
    SimpleTaskQueue() = default;
    virtual ~SimpleTaskQueue() = default;
    void Push(task_type &&t) { _q.push(std::forward<task_type>(t)); }
    bool Pop(task_type &t) {
        std::lock_guard<std::mutex> lg(_lock);
        if (_q.empty())
            return false;
        else {
            t = _q.top();
            _q.pop();
            return true;
        }
    }

  private:
    std::mutex _lock;
    std::priority_queue<task_type, std::vector<task_type>, ThreadFunctorBase::LessThan>
        _q;
};

class SimpleThreadRunner {
  public:
    SimpleThreadRunner(int run_thread)
        : _thread_cnt(run_thread) {}
    ~SimpleThreadRunner() = default;
    template <typename Callable> void AddTask(Callable &&f, int priority = 0) {
        _q.Push(MakeShared<ThreadFunctorBase, ThreadFunctorImpl<Callable>>(f, priority));
    }
    void Start() {
        for (int i = 0; i < _thread_cnt; ++i) {
            std::thread t([this]() {
                SimpleTaskQueue::task_type f_ptr = nullptr;
                while (this->_q.Pop(f_ptr)) {
                    (*f_ptr)();
                }
            });
            _t.push_back(std::move(t));
        }
    }
    void Join() {
        for (auto &t : _t) {
            t.join();
        }
        _t.clear();
    }

  private:
    int _thread_cnt = 0;
    SimpleTaskQueue _q;
    std::vector<std::thread> _t;
};

} // namespace simple_threading

#endif
