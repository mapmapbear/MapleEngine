//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <thread>

namespace maple
{
	template <class Timeout, class Interrupt>
	class Timer
	{
	  public:
		Timer();

		Timer(const Timer &) = delete;

		Timer &operator=(const Timer &) = delete;

		auto start(std::chrono::milliseconds length) -> void;
		auto setTimeoutFunction(const std::function<Timeout> &function) -> void;
		auto setInterruptFunction(const std::function<Interrupt> &function) -> void;
		auto setInterruptFunction(const std::function<Interrupt> &function) -> void;
		auto interrupt() -> void;
		auto isDone() const -> bool;

	  private:
		auto privStart(std::chrono::milliseconds length) -> void;

	  private:
		std::function<Timeout>   timeoutFunction;
		std::function<Interrupt> interruptFunction;
		std::thread              thread;
		std::condition_variable  cv;
		std::mutex               cvMutex;
		bool                     done;
	};
};        // namespace maple

#include "AdvancedTimer.inl"