#include "CppUnitTest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <deque>

#include "../JSLikePromise.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace JSLike;

namespace TestJSLikeVoidPromise
{
	//***************************************************************************************
	TEST_CLASS(Test_co_await)
	{
	private:
		Promise<bool> myCoAwaitingCoroutine(Promise<>& p) {
			co_await p;
			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(Promise<>& p) {
			try {
				co_await p;
			}
			catch (exception ex) {
				co_return true;
			}
			co_return false;
		}

	public:
		TEST_METHOD(Prereject_uncaught)
		{
			auto [p1, p1state] = Promise<>::getUnresolvedPromiseAndState();
			// Prereject p1.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			auto result = myCoAwaitingCoroutine(p1);
			Assert::IsFalse(result.isResolved());
			Assert::IsTrue(result.isRejected());
		}

		TEST_METHOD(Preresolved)
		{
			Promise<> p1;

			auto result = myCoAwaitingCoroutine(p1);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(Reject_try_catch)
		{
			auto [p1, p1state] = Promise<>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutineThatCatches(p1);

			Assert::IsFalse(result.isResolved());

			// Reject p1.  An exception should be thrown in the coroutine.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			Assert::IsTrue(result.isResolved());
			Assert::AreEqual(true, result.value());
		}

		TEST_METHOD(Reject_uncaught)
		{
			auto [p1, p1state] = Promise<>::getUnresolvedPromiseAndState();

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
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutine(p0);

			Assert::IsFalse(result.isResolved());
			p0state->resolve();
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}
	};
	//***************************************************************************************
	TEST_CLASS(Test_co_return_Explicit)
	{
	private:
		Promise<> CoReturnPromise() {
			co_return;
		}

		Promise<bool> CoAwait() {
			co_await CoReturnPromise();
			co_return true;
		}

		Promise<> CoroutineThatThrows() {
			char c = std::string().at(1); // this throws a std::out_of_range
			co_return;
		}

	public:
		TEST_METHOD(Co_await)
		{
			auto result = CoAwait();

			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(Then)
		{
			bool wasThenCalled = false;
			CoReturnPromise().Then(
				[&]()
				{
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
		}

		TEST_METHOD(throw_Catch)
		{
			bool wasExceptionThrown = false;

			CoroutineThatThrows().Catch([&](std::exception_ptr eptr)
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

			Assert::IsTrue(wasExceptionThrown);
		}
	};
	//***************************************************************************************
	TEST_CLASS(Test_co_return_Implicit)
	{
	private:
		Promise<> CoReturnPromise() {
			co_await suspend_never{};
			// Implicit co_return
		}

		Promise<bool> CoAwait() {
			co_await CoReturnPromise();
			co_return true;
		}

	public:
		TEST_METHOD(Co_await)
		{
			auto result = CoAwait();

			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(Test)
		{
			auto p = CoReturnPromise();
			Assert::IsTrue(p.isResolved());
		}

		TEST_METHOD(Then)
		{
			bool wasThenCalled = false;
			CoReturnPromise().Then(
				[&]()
				{
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(Test_constructors)
	{
	public:
		TEST_METHOD(Assign)
		{
			Promise<> pa1;
			Promise<> pa2 = pa1;

			Assert::IsTrue(pa1.state() == pa2.state());
		}

		TEST_METHOD(Copy)
		{
			Promise<> pa1;
			Promise<> pa2(pa1);

			Assert::IsTrue(pa1.state() == pa2.state());
		}

		TEST_METHOD(Default)
		{
			Promise<> p;
			Assert::IsTrue(p.isResolved());
		}

		TEST_METHOD(InitializerThatResolves)
		{
			Promise<> p0(
				[](auto state) {
					state->resolve();
				});
			Assert::IsTrue(p0.isResolved());
		}

		TEST_METHOD(InitializerThatRejects)
		{
			Promise<> p0(
				[](auto state) {
					state->reject(make_exception_ptr(out_of_range("invalid string position")));
				});
			Assert::IsTrue(p0.isRejected());
		}

		TEST_METHOD(InitializerThatThrows)
		{
			Promise<> p0(
				[](auto state) {
					int i = std::string().at(1); // this generates an std::out_of_range
				});
			Assert::IsTrue(p0.isRejected());
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestRejection)
	{
	public:
		TEST_METHOD(Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			bool wasExpectedExceptionThrown = false;
			p0.Catch(
				[&](auto ex) {
					if (!ex) Assert::Fail();

					try {
						std::rethrow_exception(ex);
					}
					catch (std::exception& e) {
						if (e.what() == string("invalid string position"))
							wasExpectedExceptionThrown = true;
					}

					nCatchCalls++;
				});

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::AreEqual(1, nCatchCalls);
			Assert::IsTrue(wasExpectedExceptionThrown);
		}

		TEST_METHOD(Catch_Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			bool wasExpectedException1Thrown = false;
			bool wasExpectedException2Thrown = false;
			p0
				.Catch(
					[&](auto ex) {
						if (!ex) Assert::Fail();

						try {
							std::rethrow_exception(ex);
						}
						catch (std::exception& e) {
							if (e.what() == string("invalid string position"))
								wasExpectedException1Thrown = true;
						}

						nCatchCalls++;
					})
				.Catch(
					[&](auto ex) {
						if (!ex) Assert::Fail();

						try {
							std::rethrow_exception(ex);
						}
						catch (std::exception& e) {
							if (e.what() == string("invalid string position"))
								wasExpectedException2Thrown = true;
						}

						nCatchCalls++;
					});

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::AreEqual(2, nCatchCalls);
			Assert::IsTrue(wasExpectedException1Thrown);
			Assert::IsTrue(wasExpectedException2Thrown);
		}

		TEST_METHOD(Catch_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			bool wasExpectedExceptionThrown = false;
			p0
				.Catch(
					[&](auto ex) {
						if (!ex) Assert::Fail();

						try {
							std::rethrow_exception(ex);
						}
						catch (std::exception& e) {
							if (e.what() == string("invalid string position"))
								wasExpectedExceptionThrown = true;
						}

						nCatchCalls++;
					})
				.Then([&]() { nThenCalls++; });

			// Reject p0.  The "Catch" Lambda should be called.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p0 again to verify that Catch isn't called multiple times.
			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(p0.isRejected());
			Assert::IsFalse(p0.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
			Assert::IsTrue(wasExpectedExceptionThrown);
		}

		TEST_METHOD(Then_Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then([&]() { nThenCalls++; })
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
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&]() { nThenCalls++; },
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
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&]() { nThenCalls++; },
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
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			p0
				.Then(
					[&]() { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; })
				.Then(
					[&]() { nThenCalls++; });

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
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
				[&]() {
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_Then)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_Then_Catch)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				}).Catch(
				[&](auto ex) { nCatchCalls++; });

				Assert::AreEqual(1, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_Then_Then)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				}).Then(
				[&]() {
					nThenCalls++;
				});

				Assert::AreEqual(2, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_ThenCatch)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Preresolved_ThenCatch_Then)
		{
			Promise<> p1;                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
				[&]() {
					nThenCalls++;
				});

			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Catch_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
				[&]() {
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve();  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve();  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Then_Catch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				}).Catch(
				[&](auto ex) {
					nCatchCalls++;
				});

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p0state->resolve();  // Resolve
				Assert::AreEqual(1, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_Then_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				}).Then(
				[&]() {
					nThenCalls++;
				});

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p0state->resolve();  // Resolve
				Assert::AreEqual(2, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_ThenCatch)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve();  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(Unresolved_ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&]() {
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				}).Then(
				[&]() {
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve();  // Resolve
			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}
	};
	//***************************************************************************************
}
