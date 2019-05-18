# ThreadPool-CPP

## Overview
Simple C++ thread pool that maintains multiple threads waiting for tasks to be allocated for concurrent execution by the supervising program.

## Usage
```
tpool::ThreadPool pool(8);
```
Default number of the threads in the pool is _std::thread::hardware_concurrency_ - [More](http://www.cplusplus.com/reference/thread/thread/hardware_concurrency/)
```
auto future_result = pool.enqueue([](int val1, int val2){
  return val1 + val2;  
}, 2, 3);

auto result = future_result.get();
```
