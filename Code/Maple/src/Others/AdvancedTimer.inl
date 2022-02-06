//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////


namespace maple
{
	template <class Timeout, class Interrupt>
	Timer<Timeout, Interrupt>::Timer() :
	    done(false)
	{}

	template <class Timeout, class Interrupt>
	auto Timer<Timeout, Interrupt>::start(std::chrono::milliseconds length) -> void
	{
		done   = false;
		thread = std::thread(&Timer::privStart, this, length);
		thread.detach();
	}

	template <class Timeout, class Interrupt>
	auto Timer<Timeout, Interrupt>::setTimeoutFunction(const std::function<Timeout> &function) -> void
	{
		timeoutFunction = function;
	}

	template <class Timeout, class Interrupt>
	auto Timer<Timeout, Interrupt>::setInterruptFunction(const std::function<Interrupt> &function) -> void
	{
		interruptFunction = function;
	}

	template <class Timeout, class Interrupt>
	auto Timer<Timeout, Interrupt>::interrupt() -> void
	{
		done = true;
		cv.notify_all();
	}

	template <class Timeout, class Interrupt>
	auto Timer<Timeout, Interrupt>::isDone() const -> bool
	{
		return done;
	}

	template <class Timeout, class Interrupt>
	auto Timer<Timeout, Interrupt>::privStart(std::chrono::milliseconds length) -> void
	{
		std::unique_lock<std::mutex> lock(cvMutex);
		if (cv.wait_for(lock, length, [this] { return done; }))
		{
			if (interruptFunction)
			{
				interruptFunction();
			}
		}
		else
		{
			done = true;
			if (timeoutFunction)
			{
				timeoutFunction();
			}
		}
	}

}        // namespace maple
