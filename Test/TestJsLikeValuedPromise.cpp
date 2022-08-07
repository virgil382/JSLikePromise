#include "CppUnitTest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <deque>

#include "../JSLikePromise.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace JSLike;

namespace TestJSLikeValuedPromise
{
	TEST_CLASS(Test_co_await)
	{
	private:
		Promise<bool> myCoAwaitingCoroutine(Promise<int> &p) {

			auto result = co_await p;

			Assert::AreEqual(1, result);

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(Promise<int> &p) {
			try {
				auto result = co_await p;
			}
			catch (exception ex) {
				co_return true;
			}
			co_return false;
		}

	public:
		TEST_METHOD(Preresolved)
		{
			Promise<int> p1(1);

			auto result = myCoAwaitingCoroutine(p1);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(Reject_try_catch)
		{
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutineThatCatches(p1);

			Assert::IsFalse(result.isResolved());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			Assert::IsTrue(result.isResolved());
			Assert::AreEqual(true, result.value());
		}

		TEST_METHOD(Reject_uncaught)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutine(p1);

			Assert::IsFalse(result.isResolved());
			Assert::IsFalse(result.isRejected());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			Assert::IsFalse(result.isResolved());
			Assert::IsTrue(result.isRejected());
		}

		TEST_METHOD(ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutine(p0);

			Assert::IsFalse(result.isResolved());
			p0state->resolve(1);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}
	};
	//***************************************************************************************
	TEST_CLASS(Test_co_return_ValuedPromise)
	{
	private:
		Promise<int> CoReturnPromise(Promise<int>& p) {
			co_return p;
		}

		Promise<bool> CoAwait(Promise<int>& p) {
			co_await CoReturnPromise(p);
			co_return true;
		}

	public:
		TEST_METHOD(Preresolved_Then)
		{
			auto p1 = Promise<int>(1);

			bool wasThenCalled = false;
			CoReturnPromise(p1).Then(
				[&](auto result)
				{
					Assert::AreEqual(1, result);
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
		}

		TEST_METHOD(Reject_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			int nThenCalls = 0;
			int nCatchCalls = 0;
			Promise<int> pa = CoReturnPromise(p0);
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			Assert::IsFalse(pa.isRejected());

			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			Assert::IsTrue(pa.isRejected());
			Assert::IsFalse(pa.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}

		TEST_METHOD(ResolvedLater_co_await)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			auto result = CoAwait(p0);

			Assert::IsFalse(result.isResolved());
			p0state->resolve(1);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(ResolvedLater_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			bool wasThenCalled = false;
			CoReturnPromise(p0).Then([&](auto result)
				{
					Assert::AreEqual(1, result);
					wasThenCalled = true;
				});

			Assert::IsFalse(wasThenCalled);
			p0state->resolve(1);
			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestRejection)
	{
	public:
		TEST_METHOD(Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			p0.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::AreEqual(1, nCatchCalls);
		}

		TEST_METHOD(Catch_Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			p0
				.Catch([&](auto ex) { nCatchCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::AreEqual(2, nCatchCalls);
		}

		TEST_METHOD(Catch_Then)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto result) { nThenCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::IsFalse(p0.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}

		TEST_METHOD(Then_Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then([&](auto result) { nThenCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::IsFalse(p0.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}

		TEST_METHOD(ThenCatch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&](auto result) { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::IsFalse(p0.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}

		TEST_METHOD(ThenCatch_Catch)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&](auto result) { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; })
				.Catch(
					[&](auto ex) { nCatchCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::IsFalse(p0.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(2, nCatchCalls);
		}

		TEST_METHOD(ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&](auto result) { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; })
				.Then(
					[&](auto result) { nThenCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::IsFalse(p0.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestResolution)
	{
	public:
		TEST_METHOD(Preresolved_Catch_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_Then_Catch)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				}).Catch(
				[&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_Then_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				}).Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_ThenCatch)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_ThenCatch_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Catch_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
					[&](auto result) {
						Assert::AreEqual(1, result);
						nThenCalls++;
					});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Then_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				}).Catch(
				[&](auto ex) {
						nCatchCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Then_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				}).Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p0state->resolve(1);  // Resolve
				Assert::AreEqual(2, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_ThenCatch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p0state->resolve(1);  // Resolve
				Assert::AreEqual(1, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				}).Then(
				[&](auto result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}
	};
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
}
