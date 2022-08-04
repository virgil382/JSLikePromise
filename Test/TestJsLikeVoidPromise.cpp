#include "CppUnitTest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <thread>
#include <deque>

#include "../JSLikeVoidPromise.hpp"
#include "../JSLikePromise.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace JSLike;

namespace TestJSLikeVoidPromise
{
	//***************************************************************************************
	TEST_CLASS(TestPromiseWithVoidTemplateArgument)
	{
		TEST_METHOD(TestExplicitVoidArgument)
		{
			bool wasInitialized = false;

			// From a regular function, call a coroutine that returns immediately.
			Promise<void> p0([&](auto state)
				{
					wasInitialized = true;
				});

			Assert::IsTrue(wasInitialized);
		}

		TEST_METHOD(TestDefaultVoidArgument)
		{
			bool wasInitialized = false;

			// From a regular function, call a coroutine that returns immediately.
			Promise<> p0([&](auto state)
				{
					wasInitialized = true;
				});

			Assert::IsTrue(wasInitialized);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatDoesNotSuspend)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await suspend_never{};
		}

	public:
		TEST_METHOD(GetResultAfterSleeping)
		{
			// From a regular function, call a coroutine that returns immediately.
			auto p = myCoroutine0();
			std::this_thread::sleep_for(1000ms);
			Assert::AreEqual(true, p.isResolved());
		}

		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			// From a regular function, call a coroutine that returns immediately.
			myCoroutine0().Then([&]()
				{
					wasThenCalled = true;
				});

			// Wait for Then to be called.
			for (int i = 0; i < 30 && wasThenCalled == false; i++) {
				std::this_thread::sleep_for(2000ms);
			}

			Assert::IsTrue(wasThenCalled);
		}
	};

	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatSuspendsAndContinuesAfter1sec1)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await resolveAfter1Sec();
		}

		std::shared_ptr<VoidPromiseState> m_promiseState;
		VoidPromise resolveAfter1Sec() {
			return VoidPromise([this](auto promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultAfterSleeping)
		{
			auto result = myCoroutine0();  // Should get resolved to 2 after 1sec

			Assert::AreEqual(false, result.isResolved());
			std::this_thread::sleep_for(1000ms);
			m_promiseState->resolve();  // Resolve

			Assert::AreEqual(true, result.isResolved());
		}
	};

	TEST_CLASS(TestFunctionCallsCoroutineThatSuspendsAndContinuesAfter1sec2)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await resolveAfter1Sec();
		}

		std::shared_ptr<VoidPromiseState> m_promiseState;
		VoidPromise resolveAfter1Sec() {
			return VoidPromise([this](auto promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			myCoroutine0()     // Should get resolved to 2 after 1sec
				.Then([&]()
					{
						wasThenCalled = true;
					});

			std::this_thread::sleep_for(1000ms);
			m_promiseState->resolve();  // Resolve

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineCallsCoroutineThatResolvesImmediately)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await myCoroutine1();
		}

		VoidPromise myCoroutine1() {
			co_return;
		}

	public:
		TEST_METHOD(GetResultAfterSleeping)
		{
			auto result = myCoroutine0();  // Should get resolved to 2 after 1sec

			std::this_thread::sleep_for(1000ms);

			Assert::AreEqual(true, result.isResolved());
		}

		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			myCoroutine0()     // Should get resolved to 2 after 1sec
				.Then([&]()
					{
						wasThenCalled = true;
					});

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineCallsCoroutineThatResolvesAfter1Sec1)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await myCoroutine1();
		}

		VoidPromise myCoroutine1() {
			co_await resolveAfter1Sec();
		}

		std::shared_ptr<VoidPromiseState> m_promiseState;
		VoidPromise resolveAfter1Sec() {
			return VoidPromise([this](auto promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultAfterSleeping)
		{
			auto result = myCoroutine0();  // Should get resolved to 2 after 1sec

			Assert::AreEqual(false, result.isResolved());
			std::this_thread::sleep_for(1000ms);
			m_promiseState->resolve();

			Assert::AreEqual(true, result.isResolved());
		}
	};

	TEST_CLASS(TestCoroutineCallsCoroutineThatResolvesAfter1Sec2)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await myCoroutine1();
		}

		VoidPromise myCoroutine1() {
			co_await resolveAfter1Sec();
		}

		std::shared_ptr<VoidPromiseState> m_promiseState;
		VoidPromise resolveAfter1Sec() {
			return VoidPromise([this](auto promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			myCoroutine0()     // Should get resolved to 2 after 1sec
				.Then([&]()
					{
						wasThenCalled = true;
					});

			std::this_thread::sleep_for(1000ms);
			m_promiseState->resolve();

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatDoesNotSuspendAndThrows)
	{
	private:
		VoidPromise myCoroutine0() {
			int i = std::string().at(1); // this generates an std::out_of_range
			co_return;
		}

	public:
		TEST_METHOD(GetExceptionWithCatch)
		{
			bool wasExceptionThrown = false;

			// From a regular function, call a coroutine that returns immediately.
			myCoroutine0().Catch([&](std::exception_ptr eptr)
				{
					if(!eptr) Assert::Fail();

					try {
						std::rethrow_exception(eptr);
					}
					catch (std::exception &e) {
						if (e.what() == string("invalid string position"))
							wasExceptionThrown = true;
					}
				});

			Assert::IsTrue(wasExceptionThrown);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatSuspendsAndContinuesAfter1secAndThrows)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await resolveAfter1Sec();
			int i = std::string().at(1); // this generates an std::out_of_range
		}

		std::shared_ptr<VoidPromiseState> m_promiseState;
		VoidPromise resolveAfter1Sec() {
			return VoidPromise([this](auto promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultWithThen)
		{
			bool wasExceptionThrown = false;
			myCoroutine0().Catch([&](std::exception_ptr eptr)
				{
					if (!eptr) Assert::Fail();

					try {
						std::rethrow_exception(eptr);
					}
					catch (std::exception& e) {
						if (e.what() == string("invalid string position"))
							wasExceptionThrown = true;
					}
				});

			std::this_thread::sleep_for(1000ms);
			m_promiseState->resolve();  // this will resume the coroutine, which will throw

			Assert::IsTrue(wasExceptionThrown);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatSuspendsAndThrowsAfter1sec)
	{
	private:
		VoidPromise myCoroutine0() {
			co_await rejectAfter1Sec(); // This is supposed to throw... How?
		}

		std::shared_ptr<VoidPromiseState> m_promiseState;
		VoidPromise rejectAfter1Sec() {
			return VoidPromise([this](auto promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultWithThen)
		{
			bool wasExceptionThrown = false;
			myCoroutine0().Catch([&](std::exception_ptr eptr)
				{
					if (!eptr) Assert::Fail();

					try {
						std::rethrow_exception(eptr);
					}
					catch (std::exception& e) {
						if (e.what() == string("invalid string position"))
							wasExceptionThrown = true;
					}
				});

			std::this_thread::sleep_for(1000ms);
			m_promiseState->reject(make_exception_ptr(std::out_of_range("invalid string position")));

			Assert::IsTrue(wasExceptionThrown);
		}
	};
	//***************************************************************************************
}