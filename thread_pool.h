#pragma once

#include "file_descriptor.h"

#include <thread>
#include <functional>
#include <queue>
#include <memory>
#include <mutex>
#include <type_traits>
#include <condition_variable>
#include <atomic>

template <size_t number_of_threads>
struct thread_pool;

template <typename T>
struct promise_to_task
{	
	promise_to_task(promise_to_task&& another) noexcept
	{
		std::swap(result, another.result);
	}

	promise_to_task(const promise_to_task& another) noexcept:
		result(another.result)
	{}
		
	explicit operator bool() const
	{
		if (result->first)
		{
			return true;
		}
		return false;
	}
	
	T& get()
	{
		return *(reinterpret_cast <T*> (&result->second));
	}
	
	T const& get() const
	{
		return *(reinterpret_cast <T*> (&result->second));
	}

	private:

	typedef std::pair <std::atomic_bool, typename std::aligned_storage <sizeof(T), alignof(T)>::type> special_type;
	
	promise_to_task():
		result(new special_type(), [](special_type* to_del)
		{
			if (to_del->first)
			{
				reinterpret_cast <T*> (&to_del->second)->~T();
			}
			delete to_del;
		})
	{
		result->first = false;
	}

	auto get_pointer()
	{
		return result;
	}

	template <size_t thread_number>
	friend struct thread_pool;

	std::shared_ptr <special_type> result;
};

template <size_t number_of_threads>
struct thread_pool
{
	template <size_t size>
	thread_pool(thread_pool <size>&& another):
		interruptor(false)
	{
		another.destruction();
		std::swap(tasks, another.tasks);
		initialize();
	}
	
	thread_pool():
		interruptor(false)
	{
		initialize();
	}

	template <typename T>
	promise_to_task<T> add_task(std::function <T()> func, file_descriptor <notifier> notify)
	{
		promise_to_task<T> promise;
		
		std::unique_lock <std::mutex> queue_lock(for_queue);
		tasks.emplace(std::move(std::make_pair([func, pointer = promise.get_pointer()]()
		{
			auto&& result = func();
			new (&pointer->second) T(std::forward <decltype(result)> (result));
			pointer->first = true;
		}, std::move(notify))));
		queue_lock.unlock();
		cv.notify_one();
		
		return promise;
	}

	~thread_pool()
	{
		destruction();
	}
	
	private:

	void initialize()
	{
		for (size_t i = 0; i < number_of_threads; i++)
		{
			new (threads + i) std::thread([this]()
			{
				while (!interruptor)
				{
					std::unique_lock <std::mutex> lock(for_queue);
					if (tasks.size() == 0)
					{
						cv.wait(lock, [this]()
						{
							return !tasks.empty() || interruptor;
						});
					}
					else
					{
						auto to_exec = tasks.front().first;
						auto fd = std::move(tasks.front().second);
						tasks.pop();
						lock.unlock();
						
						to_exec();
						fd.notify();
					}
				}
			});
		}
	}

	void destruction()
	{
		if (!interruptor)
		{
			interruptor = true;
			cv.notify_all();
			for (size_t i = 0; i < number_of_threads; i++)
			{
				(reinterpret_cast <std::thread*> (threads + i))->join();
			}
			for (size_t i = 0; i < number_of_threads; i++)
			{
				using std::thread;
				reinterpret_cast <thread*> (threads + i)->~thread();
			}
		}
	}
	
	typename std::aligned_storage <sizeof(std::thread), alignof(std::thread)>::type threads[number_of_threads];
	std::queue <std::pair <std::function <void()>, file_descriptor <notifier>>> tasks;
	std::mutex for_queue;
	std::condition_variable cv;
	std::atomic_bool interruptor;

	template <size_t size>
	friend struct thread_pool;
};
