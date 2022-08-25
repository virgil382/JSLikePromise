#include "CppUnitTest.h"

#include <coroutine>
#include <functional>
#include <iostream>
#include <deque>

#include "../JSLikePromise.hpp"
#include "TranscriptionCounter.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace JSLike;

namespace TestJSLikeValuedPromise
{
	//***************************************************************************************
	TEST_CLASS(Test_co_await)
	{
	private:
		Promise<bool> myCoAwaitingCoroutine(Promise<int> &p) {

			auto result = co_await p;

			Assert::AreEqual(1, result);

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatMoves(Promise<TranscriptionCounter>& p) {
			TranscriptionCounter r;
			r = move(co_await p);   // should invoke the move assignment operator

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

		TEST_METHOD(ResolvedLater_move)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();

			auto result = myCoAwaitingCoroutineThatMoves(p0);

			Assert::IsFalse(result.isResolved());

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(move(*obj));  // Resolve

			Assert::AreEqual(0, nCopyCtor);
			Assert::AreEqual(0, nCopyAssign);
			Assert::AreEqual(1, nMoveCtor);    // performed by resolve()
			Assert::AreEqual(1, nMoveAssign);  // performed inside the coroutine


			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}
	};
	//***************************************************************************************
	TEST_CLASS(Test_co_return_Value)
	{
	private:
		Promise<int> CoReturnPromise(int val) {
			co_return val;
		}

		Promise<bool> CoAwait(int val) {
			auto result = co_await CoReturnPromise(val);
			Assert::AreEqual(result, val);
			co_return true;
		}

	public:
		TEST_METHOD(Co_await)
		{
			auto result = CoAwait(1);

			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(Then)
		{
			bool wasThenCalled = false;
			CoReturnPromise(1).Then(
				[&](int &result)
				{
					Assert::AreEqual(1, result);
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
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

		Promise<int> CoroutineThatThrows() {
			char c = std::string().at(1); // this throws a std::out_of_range
			co_return 1;
		}

	public:
		TEST_METHOD(Preresolved_Then)
		{
			auto p1 = Promise<int>(1);

			bool wasThenCalled = false;
			CoReturnPromise(p1).Then(
				[&](int &result)
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
			bool wasExceptionThrown = false;
			Promise<int> pa = CoReturnPromise(p0);
			pa.Then([&](int &result) { nThenCalls++; });
			pa.Catch([&](auto ex) {
				if (!ex) Assert::Fail();

				try {
					std::rethrow_exception(ex);
				}
				catch (std::exception& e) {
					if (e.what() == string("invalid string position"))
						wasExceptionThrown = true;
				}

				nCatchCalls++;
				});

			Assert::IsFalse(pa.isRejected());

			p0state->reject(make_exception_ptr(out_of_range("invalid string position")));

			Assert::IsTrue(pa.isRejected());
			Assert::IsFalse(pa.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
			Assert::IsTrue(wasExceptionThrown);
		}

		TEST_METHOD(ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();

			bool wasThenCalled = false;
			auto p = CoReturnPromise(p0);

			Assert::IsFalse(p.isResolved());
			p0state->resolve(1);
			Assert::IsTrue(p.isResolved());
			Assert::AreEqual(1, p.value());
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
			CoReturnPromise(p0).Then([&](int &result)
				{
					Assert::AreEqual(1, result);
					wasThenCalled = true;
				});

			Assert::IsFalse(wasThenCalled);
			p0state->resolve(1);
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
	TEST_CLASS(Test_constructors)
	{
	public:
		TEST_METHOD(Assign)
		{
			Promise<int> pa1(1);
			Promise<int> pa2 = pa1;

			Assert::IsTrue(pa1.state() == pa2.state());
		}

		TEST_METHOD(Copy)
		{
			Promise<int> pa1(1);
			Promise<int> pa2(pa1);

			Assert::IsTrue(pa1.state() == pa2.state());
		}

		TEST_METHOD(InitializerThatResolves)
		{
			Promise<int> p0(
				[](auto state) {
					state->resolve(1);
				});
			Assert::IsTrue(p0.isResolved());
			Assert::IsTrue(p0.value() == 1);
		}

		TEST_METHOD(InitializerThatRejects)
		{
			Promise<int> p0(
				[](auto state) {
					state->reject(make_exception_ptr(out_of_range("invalid string position")));
				});
			Assert::IsTrue(p0.isRejected());
		}

		TEST_METHOD(InitializerThatThrows)
		{
			Promise<int> p0(
				[](auto state) {
					int i = std::string().at(1); // this generates an std::out_of_range
				});
			Assert::IsTrue(p0.isRejected());
		}

		TEST_METHOD(WithValue)
		{
			Promise<int> p(1);
			Assert::IsTrue(p.isResolved());
			Assert::IsTrue(p.value() == 1);
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
				.Then([&](bool &result) { nThenCalls++; });

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
				.Then([&](bool &result) { nThenCalls++; })
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
					[&](bool &result) { nThenCalls++; },
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
					[&](bool &result) { nThenCalls++; },
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
					[&](bool &result) { nThenCalls++; },
					[&](auto ex) { nCatchCalls++; })
				.Then(
					[&](bool&result) { nThenCalls++; });

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
		TEST_METHOD(PostresolvedCopy_ThenCatchMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter& result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(*obj);             // This should invoke the copy assignment operator

			Assert::AreEqual(0, nMoveCtor);
			Assert::AreEqual(1, nMoveAssign);  // performed inside the Then Lambda above
			Assert::AreEqual(1, nCopyCtor);    // performed by resolve(T&)
			Assert::AreEqual(0, nCopyAssign);

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PostresolvedCopy_ThenMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter& result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(*obj);  // This should invoke the move assignment operator

			Assert::AreEqual(0, nMoveCtor);
			Assert::AreEqual(1, nMoveAssign);  // performed inside the Then Lambda above
			Assert::AreEqual(1, nCopyCtor);    // performed by resolve(T&)
			Assert::AreEqual(0, nCopyAssign);

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PostresolvedLiteral_Catch_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
					[&](int& result) {
						Assert::AreEqual(1, result);
						nThenCalls++;
					});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PostresolvedLiteral_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve(1);  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PostresolvedLiteral_Then_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
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

		TEST_METHOD(PostresolvedLiteral_Then_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				}).Then(
					[&](int& result) {
						Assert::AreEqual(1, result);
						nThenCalls++;
					});

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p0state->resolve(1);  // Resolve
				Assert::AreEqual(2, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PostresolvedLiteral_ThenCatch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
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

		TEST_METHOD(PostresolvedLiteral_ThenCatch_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](int& result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				}).Then(
					[&](int& result) {
						Assert::AreEqual(1, result);
						nThenCalls++;
					});

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p0state->resolve(1);  // Resolve
				Assert::AreEqual(2, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PostresolvedMove_ThenCatchMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter& result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				},
				[&](auto ex) {
					nCatchCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(move(*obj));      // This should invoke the move assignment operator

			Assert::AreEqual(1, nMoveCtor);    // performed by resolve(T&&)
			Assert::AreEqual(1, nMoveAssign);  // performed inside the Then Lambda above
			Assert::AreEqual(0, nCopyCtor);
			Assert::AreEqual(0, nCopyAssign);

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PostresolvedMove_ThenMove)
		{
			auto [p0, p0state] = Promise<TranscriptionCounter>::getUnresolvedPromiseAndState();  // resolved later

			int nThenCalls = 0;
			int nCatchCalls = 0;

			p0.Then(
				[&](TranscriptionCounter & result) {
					TranscriptionCounter r; // This should invoke the default constructor
					r = move(result);       // This should invoke the move assignment operator
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);

			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter *obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			p0state->resolve(move(*obj));  // This should invoke the move assignment operator

			Assert::AreEqual(1, nMoveCtor);    // performed by resolve(T&&)
			Assert::AreEqual(1, nMoveAssign);  // performed inside the Then Lambda above
			Assert::AreEqual(0, nCopyCtor);
			Assert::AreEqual(0, nCopyAssign);

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PreresolvedCopy_Then)
		{
			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			Promise<TranscriptionCounter> p1(*obj);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](TranscriptionCounter& result) {
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);

			Assert::AreEqual(0, nMoveCtor);
			Assert::AreEqual(0, nMoveAssign);
			Assert::AreEqual(1, nCopyCtor);    // performed by Promise(T &)
			Assert::AreEqual(0, nCopyAssign);
		}

		TEST_METHOD(PreresolvedLiteral_Catch_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Catch(
				[&](auto ex) { nCatchCalls++; }).Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PreresolvedLiteral_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PreresolvedLiteral_Then_Catch)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				}).Catch(
				[&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PreresolvedLiteral_Then_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				}).Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PreresolvedLiteral_ThenCatch)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PreresolvedLiteral_ThenCatch_Then)
		{
			Promise<int> p1(1);                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
				[&](int &result) {
					Assert::AreEqual(1, result);
					nThenCalls++;
				});

			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(PreresolvedMove_Then)
		{
			// Construct a TranscriptionCounter, and use it to resolve the Promise.
			int nMoveCtor = 0, nMoveAssign = 0, nCopyCtor = 0, nCopyAssign = 0;
			TranscriptionCounter* obj = TranscriptionCounter::constructAndSetCounters("obj1", &nMoveCtor, &nMoveAssign, &nCopyCtor, &nCopyAssign);
			Promise<TranscriptionCounter> p1(move(*obj));                                              // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			p1.Then(
				[&](TranscriptionCounter& result) {
					nThenCalls++;
				});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);

			Assert::AreEqual(1, nMoveCtor);    // performed by Promise(T &&)
			Assert::AreEqual(0, nMoveAssign);
			Assert::AreEqual(0, nCopyCtor);
			Assert::AreEqual(0, nCopyAssign);
		}
		//***************************************************************************************
	};
}
