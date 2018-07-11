#ifndef SIMPLE_THREADPOOL_H
#define SIMPLE_THREADPOOL_H

#include <thread>
#include <memory>
#include <vector>
#include <mutex>
#include <queue>
#include <functional>
#include <condition_variable>

class SimpleThreadPool {
 public:
  SimpleThreadPool(int size) {
    for (int worker_index = 0; worker_index < size; ++worker_index) {
      auto mutex = std::make_unique<std::mutex>();
      mutex->lock();
      mutexes.push_back(std::move(mutex));
      workers.push_back(std::thread([worker_index, this]() {
        while (true) {
          // Wait util get a job
          this->mutexes[worker_index]->lock();

          // Get a job
          jobs_mutex.lock();
          if (this->jobs.empty()) {
            jobs_mutex.unlock();
            return;
          }
          auto job = this->jobs.front();
          this->jobs.pop();
          jobs_mutex.unlock();

          // Do a job
          job();

          usable_workers_mutex.lock();
          usable_workers.push(worker_index);
          usable_workers_mutex.unlock();
          this->cv.notify_one();
        }
      }));
      usable_workers.push(worker_index);
    }
  }

  ~SimpleThreadPool() {
    while (!jobs.empty()) jobs.pop();
    for (auto &mutex : mutexes) mutex->unlock();
    for (auto &worker : workers) worker.join();
  }

  // This method can block
  void AddJob(const std::function<void()> &job) {
    // wait
    std::unique_lock<std::mutex> lock(wait_add_job_mutex);
    cv.wait(lock, [this]() { return !usable_workers.empty(); });

    usable_workers_mutex.lock();
    if (!usable_workers.empty()) {
      auto worker_index = usable_workers.front();
      usable_workers.pop();
      jobs.push(job);
      mutexes[worker_index]->unlock();
    }
    usable_workers_mutex.unlock();
  }

  std::vector<std::thread> workers;
  std::vector<std::unique_ptr<std::mutex>> mutexes;
  std::queue<int> usable_workers;
  std::queue<std::function<void()>> jobs;
  std::mutex jobs_mutex, usable_workers_mutex;
  std::mutex wait_add_job_mutex;
  std::condition_variable cv;
};

#endif
