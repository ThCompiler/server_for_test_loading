#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <list>
#include <functional>

namespace prll {
#define MAXNTHREADS (size_t)50

class Parallel {
  public:
    Parallel();

    template<typename Callable, typename... Args>
    void add(Callable &&f, Args &&... args) {
        if (_max_threads == 0) {
            f(args...);
        } else {
            {
                std::lock_guard<std::mutex> lck(_task_mutex);

                if (_exit) {
                    return;
                }

                _tasks.push(std::move([f, args...] {
                    f(args...);
                }));
            }

            _wait.notify_one();;
        }
    }

    void join();

    void stop();

    void set_max_threads(size_t max_threads);

    [[nodiscard]] size_t get_count_threads() const;

    ~Parallel();

  private:

    class Thread {
      public:
        Thread() = delete;

        Thread(std::function<void()> task,
               std::condition_variable &threads);

        Thread(Thread &&) noexcept = delete;

        void join();

        [[nodiscard]] bool is_finished() const;

        ~Thread();

      private:

        void _main();

        std::thread                 _main_thread;
        std::atomic<long>           _have_proccess;
        std::condition_variable&    _wait;
        std::function<void(void)>   _task;
    };

    void _supervisor();

    bool                                    _exit = false;
    size_t                                  _max_threads;
    std::mutex                              _task_mutex;
    std::condition_variable                 _wait;
    std::list<std::unique_ptr<Thread>>      _threads;
    std::queue<std::function<void(void)>>   _tasks;

    std::thread _main_thread;
};
}