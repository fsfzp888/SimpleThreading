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
