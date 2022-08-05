#include "CppUnitTest.h"

#include <string>
#include <coroutine>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>
#include <utility>  // for std::pair

#include "TimerExtent.h"
#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAny.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace JSLike;

namespace TestJSLikePromiseAny
{
	//***************************************************************************************
	TEST_CLASS(TestPreResolvedPromisesAndOneResolvedLater)
	{
	public:
		TEST_METHOD(Test)
		{
			// Get a BasePromise that we'll resolve later, and its state.
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// Get 3 pre-resolved Promises.
			Promise<int> p1(1);
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);

			bool areAllResolved = false;

			PromiseAny pa({ p0, p1, p2, p3 });
			pa.Then([&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					areAllResolved = true;
				});

			Assert::IsTrue(areAllResolved);
			p0state->resolve();  // Resolve p0

			Assert::IsTrue(areAllResolved);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestChainedPromiseAny)
	{
	public:
		TEST_METHOD(Test)
		{
			auto p0 = BasePromise([](auto junk) {});  // won't resolve

			// Get 2 pre-resolved Promises, and 1 onresolved Promise.
			auto p1 = Promise<int>([](auto junk) {});  // won't resolve
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // will resolve later
			auto p3 = Promise<double>([](auto junk) {});  // won't resolve

			PromiseAny pa1 = PromiseAny({ p1, p2, p3 });

			PromiseAny pa2({ pa1, p0 });

			bool areAnyResolved = false;
			pa2.Then([&](PromiseAny::ResultType result)
				{
					// pa1 should have been resolved, due to p1 being resolved, due to p2 being resolved.

					// Verify that result matches the result type of pa1.
					Assert::IsTrue(result->isValueOfType<PromiseAny::ResultType>());

					// Get pa1's state from result.
					PromiseAny::ResultType pa1state = result->value<PromiseAny::ResultType>();

					// Get the value from p1's state.
					Assert::AreEqual(std::string("Hello"), pa1state->value<string>());

					//Assert::AreEqual(std::string("Hello"), (result->value<PromiseAnyState>())->value<string>());
					areAnyResolved = true;
				});

			Assert::IsFalse(areAnyResolved);
			p2state->resolve("Hello");  // Resolve p2 --> resolves pa1 --> resolves pa2
			Assert::IsTrue(areAnyResolved);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineGetsResultOfPromiseAny)
	{
	private:
		Promise<bool> myCoAwaitingCoroutine() {
			// Get 3 pre-resolved Promises.
			Promise<int> p1(1);
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);

			std::shared_ptr<BasePromiseState> result = co_await PromiseAny({ p1, p2, p3 });
			Assert::AreEqual(1, result->value<int>());

			co_return true;
		}

	public:
		TEST_METHOD(TestCoAwaitPromiseAny)
		{
			// Get 3 pre-resolved Promises.
			auto result = myCoAwaitingCoroutine();
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineReturnsResultOfPromiseAny)
	{
	private:
		PromiseAny myReturningCoroutine() {
			auto p1 = Promise<int>(1);
			auto p2 = Promise<string>("Hello");
			auto p3 = Promise<double>(3.3);
			co_return PromiseAny({ p1, p2, p3 });
		}
		
		Promise<bool>  myCoAwaitingCoroutine() {
			PromiseAny::ResultType result = co_await myReturningCoroutine();
			Assert::AreEqual(1, result->value<int>());
			co_return true;
		}

	public:
		TEST_METHOD(ThenTheResultOfCoroutine)
		{
			bool wasThenCalled = false;
			myReturningCoroutine().Then([&](auto result)
				{
					Assert::AreEqual(1, result->value<int>());
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
		}

		TEST_METHOD(CoAwaitThePromiseAny)
		{
			auto p = myCoAwaitingCoroutine();
			Assert::IsTrue(p.isResolved());
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestPromiseAnyWithCatch)
	{
	public:
		TEST_METHOD(RejectAPromise)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa({ p0, p1, p2 });
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p1 again to verify that Catch isn't called multiple times.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::IsFalse(pa.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}
	};
	//***************************************************************************************


};
