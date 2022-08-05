#include "CppUnitTest.h"

#include <string>
#include <coroutine>
#include <functional>
#include <iostream>
#include <vector>
#include <utility>  // for std::pair

#include "TimerExtent.h"
#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAll.hpp"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

using namespace std;
using namespace JSLike;

namespace TestJSLikePromiseAll
{
	//***************************************************************************************
	TEST_CLASS(TestPreResolvedPromisesAndOneResolvedAfter1sec)
	{
	public:
		TEST_METHOD(ConstructPromiseAll)
		{
			// Get a BasePromise that we'll resolve later, and its state.
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();

			// Get 3 pre-resolved Promises.
			Promise<int> p1(1);
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);

			bool areAllResolved = false;

			PromiseAll pa({ p0, p1, p2, p3 });
			pa.Then([&](auto states)
				{
					Assert::AreEqual(1, states[1]->value<int>());
					Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
					Assert::AreEqual(3.3, states[3]->value<double>());
					areAllResolved = true;
				});

			Assert::IsFalse(areAllResolved);
			p0state->resolve();  // Resolve
			Assert::IsTrue(areAllResolved);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestChainedPromiseAll)
	{
	public:
		TEST_METHOD(ConstructPromiseAll)
		{
			// Get 3 pre-resolved Promises.
			Promise<int> p1(1);
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);

			PromiseAll pa1 = PromiseAll({ p1, p2, p3 }).Then([&](auto states)
				{
					Assert::AreEqual(1, states[0]->value<int>());
					Assert::AreEqual(std::string("Hello"), states[1]->value<std::string>());
					Assert::AreEqual(3.3, states[2]->value<double>());
				});

			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();
			PromiseAll pa2({ pa1, p0 });

			bool areAllResolved = false;
			pa2.Then([&](auto states)
				{
					areAllResolved = true;
				});

			Assert::IsFalse(areAllResolved);
			p0state->resolve();  // Resolve
			Assert::IsTrue(areAllResolved);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineGetsResultOfPromiseAll)
	{
	private:
		Promise<bool> myCallingCoroutine() {
			// Get 3 pre-resolved Promises.
			Promise<int> p1(1);
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);

			std::vector<std::shared_ptr<BasePromiseState>> result(co_await PromiseAll({ p1, p2, p3 }));
			Assert::AreEqual(1, result[0]->value<int>());
			Assert::AreEqual(std::string("Hello"), result[1]->value<string>());
			Assert::AreEqual(3.3, result[2]->value<double>());

			co_return true;
		}

	public:
		TEST_METHOD(ConstructPromiseAll)
		{
			// Get 3 pre-resolved Promises.
			auto result = myCallingCoroutine();
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineReturnsResultOfPromiseAll)
	{
	private:
		Promise<PromiseAll::ResultType> myReturningCoroutine() {
			auto p1 = Promise<int>(1);
			auto p2 = Promise<string>("Hello");
			auto p3 = Promise<double>(3.3);
			co_return co_await PromiseAll({ p1, p2, p3 });
		}
		
		Promise<bool>  myAwaitingCoroutine() {
			PromiseAll::ResultType result = co_await myReturningCoroutine();
			Assert::AreEqual(1, result[0]->value<int>());
			Assert::AreEqual(std::string("Hello"), result[1]->value<string>());
			Assert::AreEqual(3.3, result[2]->value<double>());
			co_return true;
		}

	public:
		TEST_METHOD(ThenTheResultOfCoroutine)
		{
			bool wasThenCalled = false;
			myReturningCoroutine().Then([&](auto result)
				{
					Assert::AreEqual(1, result[0]->value<int>());
					Assert::AreEqual(std::string("Hello"), result[1]->value<string>());
					Assert::AreEqual(3.3, result[2]->value<double>());
					wasThenCalled = true;
				});


			Assert::IsTrue(wasThenCalled);
		}

		TEST_METHOD(CoAwaitThePromiseAll)
		{
			myAwaitingCoroutine();
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineReturnsPromiseAll)
	{
	private:
		vector<shared_ptr<BasePromiseState>> m_promiseStates;

		PromiseAll myPromiseAllReturningCoroutine() {
			// Create 3 Promises to give to PromiseAll.  Save their PromiseStates.
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			m_promiseStates.push_back(p0state);
			m_promiseStates.push_back(p1state);
			m_promiseStates.push_back(p2state);

			co_return PromiseAll({ p0, p1, p2 });
		}

	public:
		TEST_METHOD(ThenThePromiseAllFromCoroutine)
		{
			bool wasThenCalled = false;
			myPromiseAllReturningCoroutine().Then([&](auto result)
				{
					Assert::AreEqual(1, result[0]->value<int>());
					Assert::AreEqual(std::string("Hello"), result[1]->value<string>());
					Assert::AreEqual(3.3, result[2]->value<double>());
					wasThenCalled = true;
				});

			Assert::IsFalse(wasThenCalled);
			m_promiseStates[0]->resolve<int>(1);
			Assert::IsFalse(wasThenCalled);
			m_promiseStates[1]->resolve<string>("Hello");
			Assert::IsFalse(wasThenCalled);
			m_promiseStates[2]->resolve<double>(3.3);

			Assert::IsTrue(wasThenCalled);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestPromiseAllWithCatch)
	{
	public:
		TEST_METHOD(RejectAPromise)
		{
			// Create 3 Promises to give to PromiseAll.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			// Resolve 1 Promises and reject 2.
			p0state->resolve(1);
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			p2state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p2 again to verify that Catch isn't called multiple times.
			p2state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::IsFalse(pa.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestPromiseAllWithPreRejectedPromise)
	{
	public:
		TEST_METHOD(RejectAPromise)
		{
			// Create 3 Promises to give to PromiseAll.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState(); // will be rejected
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa({ p0, p1, p2 });
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::IsFalse(pa.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestCoroutineReturnsPromiseAllWithCatch)
	{
	private:
		vector<shared_ptr<BasePromiseState>> m_promiseStates;

		PromiseAll myPromiseAllReturningCoroutine() {
			// Create 3 Promises to give to PromiseAll.  Save their PromiseStates.
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			m_promiseStates.push_back(p0state);
			m_promiseStates.push_back(p1state);
			m_promiseStates.push_back(p2state);

			co_return PromiseAll({ p0, p1, p2 });
		}

	public:
		TEST_METHOD(RejectAPromise)
		{
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa = myPromiseAllReturningCoroutine();
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			// Resolve 1 Promises and reject 2.
			m_promiseStates[0]->resolve(1);
			m_promiseStates[1]->reject(make_exception_ptr(out_of_range("invalid string position")));
			m_promiseStates[2]->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p2 again to verify that Catch isn't called multiple times.
			m_promiseStates[2]->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::IsFalse(pa.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(1, nCatchCalls);
		}
	};
	//***************************************************************************************

};
