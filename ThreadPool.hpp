#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <functional>
#include <future>
#include <queue>

namespace tpool {

class ThreadPool final {
   public:
    explicit ThreadPool(size_t threads = std::thread::hardware_concurrency());

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;

    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    template <class F, class... Args>
    decltype(auto) enqueue(F&& f, Args&&... args);

    ~ThreadPool();

   private:
    std::vector<std::thread> workers;
    std::queue<std::packaged_task<void()> > tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;

    bool stop;
};

ThreadPool::ThreadPool(size_t threads) : stop(false) {
    if (threads == 0)
        throw std::invalid_argument("Threads number should be > 0");

    workers.reserve(threads);
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back([this] {
            for (;;) {
                std::packaged_task<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });

                    if (this->stop && this->tasks.empty())
                        return;

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                task();
            }
        });
}

template <class F, class... Args>
decltype(auto) ThreadPool::enqueue(F&& f, Args&&... args) {
    using return_type = std::invoke_result_t<F, Args...>;

    std::packaged_task<return_type()> task(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task.get_future();

    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        if (stop)
            throw std::runtime_error("Enqueue to stopped ThreadPool");

        tasks.emplace(std::move(task));
    }

    condition.notify_one();

    return res;
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all();

    for (auto& worker : workers) {
        worker.join();
    }
}

}  // namespace tpool

#endif