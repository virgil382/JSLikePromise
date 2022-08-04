#include "CppUnitTest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <thread>
#include <deque>

#include "../JSLikePromise.hpp"
#include "../JSLikeBasePromise.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace JSLike;

namespace TestJSLikePromise
{
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatDoesNotSuspend)
	{
	private:
		Promise<int> routine0() {
			return 1;
		}

		Promise<int> myCoroutine0() {
			co_return 1;
		}

	public:
		TEST_METHOD(PreResolvedPromise1)
		{
			auto result = routine0();
			Assert::AreEqual(true, result.isResolved());
			Assert::AreEqual(1, result.value());
		}

		TEST_METHOD(PreResolvedPromise2)
		{
			// From a regular function, call a coroutine that returns immediately.
			auto result = myCoroutine0();
			Assert::AreEqual(true, result.isResolved());
			Assert::AreEqual(1, result.value());
		}

		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			// From a regular function, call a coroutine that returns immediately.
			myCoroutine0().Then([&](int val)
				{
					Assert::AreEqual(1, val);
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatSuspendsAndContinuesLater)
	{
	private:
		Promise<int> myCoroutine0() {
			int val = co_await resolveAfter1Sec();
			co_return val;
		}

		std::shared_ptr<PromiseState<int>> m_promiseState;
		Promise<int> resolveAfter1Sec() {
			return Promise<int>([this](std::shared_ptr<PromiseState<int>> promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultAfterSleeping)
		{
			auto result = myCoroutine0();  // Should get resolved to 2 after 1sec

			Assert::AreEqual(false, result.isResolved());

			m_promiseState->resolve(2);

			Assert::AreEqual(true, result.isResolved());
			Assert::AreEqual(2, result.value());
		}
	};

	TEST_CLASS(TestFunctionCallsCoroutineThatSuspendsAndContinuesAfter1sec2)
	{
	private:
		Promise<int> myCoroutine0() {
			int val = co_await resolveLater();
			co_return val;
		}

		std::shared_ptr<PromiseState<int>> m_promiseState;
		Promise<int> resolveLater() {
			return Promise<int>([this](std::shared_ptr<PromiseState<int>> promiseState)
				{
					m_promiseState = promiseState;
				});
		}

	public:
		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			myCoroutine0()     // Should get resolved to 2 after 1sec
				.Then([&](int val)
					{
						Assert::AreEqual(2, val);
						wasThenCalled = true;
					});

			Assert::IsFalse(wasThenCalled);

			m_promiseState->resolve(2);

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineCallsCoroutineThatResolvesImmediately1)
	{
	private:
		Promise<int> myCoroutine0() {
			int val = co_await myCoroutine1();
			co_return val;
		}

		Promise<int> myCoroutine1() {
			co_return 3;
		}

	public:

		TEST_METHOD(GetResultAfterSleeping)
		{
			Promise<int> result = myCoroutine0();  // Should get resolved to 2 after 1sec

			Assert::AreEqual(true, result.isResolved());
			Assert::AreEqual(3, result.value());
		}
	};

	TEST_CLASS(TestCoroutineCallsCoroutineThatResolvesImmediately2)
	{
	private:
		Promise<int> myCoroutine0() {
			int val = co_await myCoroutine1();
			co_return val;
		}

		Promise<int> myCoroutine1() {
			co_return 3;
		}

	public:
		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			myCoroutine0()
				.Then([&](int val)
					{
						Assert::AreEqual(3, val);
						wasThenCalled = true;
					});

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineCallsCoroutineThatResolvesAfter1Sec1)
	{
	private:
		Promise<int> myCoroutine0() {
			int val = co_await myCoroutine1();
			co_return val;
		}

		Promise<int> myCoroutine1() {
			co_return co_await resolveAfter1Sec();
		}

		std::shared_ptr<PromiseState<int>> m_promiseState;
		Promise<int> resolveAfter1Sec() {
			return Promise<int>([this](auto promiseState)
				{
					m_promiseState = promiseState;  // save off the state so we can resolve the promise later
				});
		}

	public:
		TEST_METHOD(GetResultAfterSleeping)
		{
			Promise<int> result = myCoroutine0();  // Should get resolved to 2 after 1sec

			Assert::IsFalse(m_promiseState->isResolved());
			m_promiseState->resolve(3);  // Resolve to 3
			Assert::IsTrue(m_promiseState->isResolved());
			Assert::AreEqual(3, m_promiseState->value());
		}
	};

	TEST_CLASS(TestCoroutineCallsCoroutineThatResolvesAfter1Sec2)
	{
	private:

		Promise<int> myCoroutine0() {
			int val = co_await myCoroutine1();
			co_return val;
		}

		Promise<int> myCoroutine1() {
			co_return co_await resolveAfter1Sec();
		}

		std::shared_ptr<PromiseState<int>> m_promiseState;
		Promise<int> resolveAfter1Sec() {
			return Promise<int>([this](auto promiseState)
				{
					m_promiseState = promiseState;  // save off the state so we can resolve the promise later
				});
		}

	public:
		TEST_METHOD(GetResultWithThen)
		{
			bool wasThenCalled = false;

			myCoroutine0()
				.Then([&](int val)
					{
						Assert::AreEqual(4, val);
						wasThenCalled = true;
					});

			m_promiseState->resolve(4);  // Resolve to 4

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatDoesNotSuspendAndThrows)
	{
	private:
		Promise<int> myCoroutine0() {
			char c = std::string().at(1); // this generates an std::out_of_range
			co_return 1;
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
		Promise<int> myCoroutine0() {
			int val = co_await resolveAfter1Sec();
			char c = std::string().at(1); // this generates an std::out_of_range
			co_return val;
		}

		std::shared_ptr<PromiseState<int>> m_promiseState;
		Promise<int> resolveAfter1Sec() {
			return Promise<int>([this](auto promiseState)
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

			Assert::IsFalse(wasExceptionThrown);

			m_promiseState->resolve(2);  // Resolve to 2

			Assert::IsTrue(wasExceptionThrown);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestFunctionCallsCoroutineThatSuspendsAndThrowsAfter1sec)
	{
	private:
		Promise<int> myCoroutine0() {
			int val = co_await rejectAfter1Sec(); // This is supposed to throw... How?
			co_return val;
		}

		std::shared_ptr<PromiseState<int>> m_promiseState;
		Promise<int> rejectAfter1Sec() {
			return Promise<int>([this](auto promiseState)
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

			m_promiseState->reject(make_exception_ptr(std::out_of_range("invalid string position")));

			Assert::IsTrue(wasExceptionThrown);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestChaining)
	{
	public:

		TEST_METHOD(TestThenChaining)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			Assert::IsFalse(p0state->isResolved());

			p0state->resolve(1);

			int counter = 0;
			p0.Then([&](auto result) {
					Assert::AreEqual(1, result);
					Assert::AreEqual(1, ++counter);
				})
				.Then([&](auto result) {
					Assert::AreEqual(1, result);
					Assert::AreEqual(2, ++counter);
				});

			Assert::IsTrue(p0state->isResolved());
		}

		TEST_METHOD(TestCatchChaining)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			Assert::IsFalse(p0state->isRejected());

			p0state->reject(make_exception_ptr(std::out_of_range("invalid string position")));

			int counter = 0;
			p0.Catch([&](auto result) {
					Assert::AreEqual(1, ++counter);
				})
				.Catch([&](auto result) {
					Assert::AreEqual(2, ++counter);
				});

			Assert::IsTrue(p0state->isRejected());
		}
	};
}
