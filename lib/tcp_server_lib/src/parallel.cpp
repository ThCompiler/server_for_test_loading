#include "parallel.hpp"

#include <utility>

namespace prll {

Parallel::Parallel()
        : _exit(false)
          , _max_threads(MAXNTHREADS)
          , _threads() {
    _main_thread = std::thread(&Parallel::_supervisor, this);
}

void Parallel::set_max_threads(size_t max_threads) {
    {
        std::lock_guard lk(_task_mutex);
        _max_threads = max_threads;
    }
    _wait.notify_one();
}

Parallel::~Parallel() {
    if (!_exit) {
        stop();
    }
}

Parallel::Thread::Thread(std::function<void()> task,
                         std::condition_variable &wait)
        : _have_proccess(0)
        , _wait(wait)
        , _task(std::move(task)) {
    _have_proccess++;
    _main_thread = std::thread(&Thread::_main, this);
}

void Parallel::Thread::join() {
    _main_thread.join();
}

bool Parallel::Thread::is_finished() const {
    return _have_proccess == 0;
}

Parallel::Thread::~Thread() {
    join();
}

void Parallel::Thread::_main() {
    _task();
    _have_proccess--;

    _wait.notify_one();
}

void Parallel::_supervisor() {
    while (true) {
        std::unique_lock<std::mutex> lck(_task_mutex);

        bool end = false;
        _wait.wait(lck, [this, &end] {
            for(auto & thread : _threads) {
                if ((end = thread->is_finished())) {
                    break;
                }
            }
            return _exit || (!_tasks.empty() && _threads.size() < (size_t) _max_threads) || end;
        });

        if (_exit) {
            break;
        }

        _threads.remove_if([](const auto& thread){
            return thread->is_finished();
        });
        
        while (!_tasks.empty() && _threads.size() < (size_t) _max_threads) {
            _threads.emplace_back(new Thread(_tasks.front(), _wait));
            _tasks.pop();
        }
    }
}

size_t Parallel::get_count_threads() const {
    return _max_threads;
}

void Parallel::stop() {
    _exit = true;
    _wait.notify_one();
    _main_thread.join();
    join();
}

void Parallel::join() {
    _main_thread.join();
}
}