// file:	ThreadPool.h
//
// summary:	Implements the Thread Pool
/**************************************************************************************************
std::future
 C++ Concurrency support library std::future
Defined in header <future>
template< class T > class future;
(1)	(since C++11)
template< class T > class future<T&>;
(2)	(since C++11)
template<>          class future<void>;
(3)	(since C++11)
The class template std::future provides a mechanism to access the result of asynchronous operations:

An asynchronous operation (created via std::async, std::packaged_task, or std::promise) can provide a std::future object to the creator of that asynchronous operation.
The creator of the asynchronous operation can then use a variety of methods to query, wait for, or extract a value from the std::future.
These methods may block if the asynchronous operation has not yet provided a value.
When the asynchronous operation is ready to send a result to the creator, it can do so by modifying shared state (e.g. std::promise::set_value) that is linked to the creator's std::future.
Note that std::future references shared state that is not shared with any other asynchronous return objects (as opposed to std::shared_future).
 **************************************************************************************************/

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

class ThreadPool
{
public:
	ThreadPool(size_t);
	template <class F, class... Args> //Returns an F-based function object whose parameters are bound to Args.
	std::future<std::result_of_t<F(Args ...)>> enqueue(F&& f, Args&&... args);

	int get_workers_num()
	{
		return workers.size();
	}

	std::queue<std::function<void()>> get_tasks()
	{
		return tasks;
	}

	bool get_stop()
	{
		return stop;
	}

	~ThreadPool();
private:
	// need to keep track of threads so we can join them
	std::vector<std::thread> workers;
	// the task queue
	std::queue<std::function<void()>> tasks;

	// synchronization
	std::mutex queue_mutex; //mutex
	std::condition_variable condition;
	bool stop;
};

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads)
	: stop(false)
{
	for (size_t i = 0; i < threads; ++i)
		workers.emplace_back(
			[this]
			{
				for (;;)
				{
					std::function<void()> task;

					{
						std::unique_lock<std::mutex> lock(this->queue_mutex);
						this->condition.wait(lock,
						                     [this] { return this->stop || !this->tasks.empty(); });
						if (this->stop && this->tasks.empty())
							return;
						task = std::move(this->tasks.front());
						this->tasks.pop();
					}

					task();
				}
			}
		);
}

// Add new work item to the pool
template <class F, class... Args>
std::future<std::result_of_t<F(Args ...)>> ThreadPool::enqueue(F&& f, Args&&... args)
{
	using return_type = std::result_of_t<F(Args ...)>;

	auto task = std::make_shared<std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	std::future<return_type> res = task->get_future();
	{
		std::unique_lock<std::mutex> lock(queue_mutex);

		// don't allow enqueueing after stopping the pool
		if (stop)
			throw std::runtime_error("enqueue on stopped ThreadPool");

		tasks.emplace([task]() { (*task)(); });
	}
	condition.notify_one();
	return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	condition.notify_all();
	for (std::thread& worker : workers)
		worker.join();
}

#endif
