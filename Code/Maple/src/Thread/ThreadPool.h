//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Engine/Core.h"
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace maple
{
	class MAPLE_EXPORT Thread
	{
	  public:
		struct MAPLE_EXPORT Task
		{
			Task() = default;
			Task(const std::function<void *()> &j, std::function<void(void *)> c) :
			    job(j),
			    complete(c){};
			std::function<void *()>     job;
			std::function<void(void *)> complete;
			bool                        wait = false;
		};

		static auto sleep(int64_t ms) -> void;
		Thread(const std::string &name);
		~Thread();
		auto wait() -> void;
		auto getTaskSize() -> int32_t;
		auto addTask(const Task &task) -> void;
		auto addTask(const std::function<void *()> &job, const std::function<void(void *)> &complete) -> void;

	  private:
		auto                         run() -> void;
		std::shared_ptr<std::thread> thread;
		std::list<Task>              jobs;
		std::mutex                   mutex;
		std::condition_variable      condition;
		bool                         close = false;
		std::string                  name;
	};

	class MAPLE_EXPORT ThreadPool
	{
	  public:
		ThreadPool(int32_t threadCount);
		auto         waitAll() -> void;
		auto         addTask(const Thread::Task &task, int32_t threadIndex = -1) -> void;
		auto         addTask(const std::function<void *()> &job, const std::function<void(void *)> &complete = nullptr, int32_t threadIndex = -1) -> void;
		inline auto &getThreads()
		{
			return threads;
		};
		inline auto getThreadCount() const
		{
			return threads.size();
		}

	  private:
		std::vector<std::shared_ptr<Thread>> threads;
	};
};        // namespace maple
