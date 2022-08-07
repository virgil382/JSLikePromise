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
	TEST_CLASS(Test_co_await)
	{
	private:
		Promise<bool> myCoAwaitingCoroutine(PromiseAll& p) {

			auto result = co_await p;

			Assert::AreEqual(1, result[0]->value<int>());
			Assert::AreEqual(string("Hello"), result[1]->value<string>());
			Assert::AreEqual(3.3, result[2]->value<double>());

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(PromiseAll& p) {
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
			Promise<string> p2("Hello");
			Promise<double> p3(3.3);
			PromiseAll pa = PromiseAll({ p1, p2, p3 });  // Preresolved

			auto result = myCoAwaitingCoroutine(pa);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto pa = PromiseAll({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutine(pa);

			Assert::IsFalse(result.isResolved());
			p0state->resolve(1);
			Assert::IsFalse(result.isResolved());
			p1state->resolve("Hello");
			Assert::IsFalse(result.isResolved());
			p2state->resolve(3.3);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(Reject_try_catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto pa = PromiseAll({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutineThatCatches(pa);

			Assert::IsFalse(result.isResolved());

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			Assert::IsTrue(result.isResolved());
			Assert::AreEqual(true, result.value());
		}

		TEST_METHOD(Reject_uncaught)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto pa = PromiseAll({ p0, p1, p2 });

			auto result = myCoAwaitingCoroutine(pa);

			Assert::IsFalse(result.isResolved());
			Assert::IsFalse(result.isRejected());

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			Assert::IsFalse(result.isResolved());
			Assert::IsTrue(result.isRejected());
		}
	};
	//***************************************************************************************
	TEST_CLASS(Test_co_return)
	{
	private:
		PromiseAll CoReturnPromiseAll(PromiseAll& p) {
			co_return p;
		}

		Promise<bool> CoAwait(PromiseAll& p) {
			co_await CoReturnPromiseAll(p);
			co_return true;
		}

	public:
		TEST_METHOD(Preresolved_Then)
		{
			auto p1 = Promise<int>(1);
			auto p2 = Promise<string>("Hello");
			auto p3 = Promise<double>(3.3);
			auto p = PromiseAll({ p1, p2, p3 });


			bool wasThenCalled = false;
			CoReturnPromiseAll(p).Then([&](auto result)
				{
					Assert::AreEqual(1, result[0]->value<int>());
					Assert::AreEqual(string("Hello"), result[1]->value<string>());
					Assert::AreEqual(3.3, result[2]->value<double>());
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
		}

		TEST_METHOD(ResolvedLater_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAll({ p0, p1, p2 });

			bool wasThenCalled = false;
			CoReturnPromiseAll(p).Then([&](auto result)
				{
					Assert::AreEqual(1, result[0]->value<int>());
					Assert::AreEqual(string("Hello"), result[1]->value<string>());
					Assert::AreEqual(3.3, result[2]->value<double>());
					wasThenCalled = true;
				});

			Assert::IsFalse(wasThenCalled);
			p0state->resolve(1);
			Assert::IsFalse(wasThenCalled);
			p1state->resolve("Hello");
			Assert::IsFalse(wasThenCalled);
			p2state->resolve(3.3);
			Assert::IsTrue(wasThenCalled);
		}

		TEST_METHOD(Reject_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAll({ p0, p1, p2 });

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa = CoReturnPromiseAll(p);
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			Assert::IsFalse(pa.isRejected());

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

		TEST_METHOD(ResolvedLater_co_await)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAll({ p0, p1, p2 });


			auto result = CoAwait(p);

			Assert::IsFalse(result.isResolved());
			p0state->resolve(1);
			Assert::IsFalse(result.isResolved());
			p1state->resolve("Hello");
			Assert::IsFalse(result.isResolved());
			p2state->resolve(3.3);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestResolution)
	{
	public:
		TEST_METHOD(TestWithSomePreresolved_Then)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			bool areAllResolved = false;

			PromiseAll pa = PromiseAll({ p0, p1, p2, p3 })
				.Then([&](auto states)
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

		TEST_METHOD(TestWithAllPreresolved_Then)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			bool areAllResolved = false;

			PromiseAll pa = PromiseAll({ p0, p1, p2, p3})
				.Then([&](auto states)
				{
					Assert::AreEqual(1, states[1]->value<int>());
					Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
					Assert::AreEqual(3.3, states[3]->value<double>());
					areAllResolved = true;
				});

			Assert::IsTrue(areAllResolved);
		}

		TEST_METHOD(TestWithAllPreresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa = PromiseAll({ p0, p1, p2, p3 })
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					})
				.Catch([&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(TestWithAllPreresolved_Catch_Then)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa = PromiseAll({ Promise<>(), p1, p2, p3 })
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(TestWithSomePreresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;

			PromiseAll pa = PromiseAll({ p0, p1, p2, p3 })
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					})
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			Assert::AreEqual(0, nThenCalls);
			p0state->resolve();  // Resolve
			Assert::AreEqual(2, nThenCalls);
		}

		TEST_METHOD(TestWithSomePreresolved_Catch_Then)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAll pa = PromiseAll({ p0, p1, p2, p3 })
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve();  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(TestWithSomePreresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAll
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAll pa = PromiseAll({ p0, p1, p2, p3 })
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					})
				.Catch([&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p0state->resolve();  // Resolve
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(TestWithAllPreresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAll
			Promise<> p0;                                                    // preresolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;

			PromiseAll pa = PromiseAll({ p0, p1, p2, p3 })
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					})
				.Then([&](auto states)
					{
						Assert::AreEqual(1, states[1]->value<int>());
						Assert::AreEqual(std::string("Hello"), states[2]->value<std::string>());
						Assert::AreEqual(3.3, states[3]->value<double>());
						nThenCalls++;
					});

			Assert::AreEqual(2, nThenCalls);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestRejection)
	{
	public:
		TEST_METHOD(Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			PromiseAll pa = PromiseAll({ p0, p1, p2 })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::AreEqual(1, nCatchCalls);
		}

		TEST_METHOD(Catch_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nCatchCalls = 0;
			PromiseAll pa = PromiseAll({ p0, p1, p2 })
				.Catch([&](auto ex) { nCatchCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::AreEqual(2, nCatchCalls);
		}

		TEST_METHOD(Then_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa = PromiseAll({ p0, p1, p2 })
				.Then([&](auto result) { nThenCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

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

		TEST_METHOD(Catch_Then)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAll pa = PromiseAll({ p0, p1, p2 })
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto result) { nThenCalls++; });

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
	TEST_CLASS(TestHierarchicalPromiseAll)
	{
	public:
		TEST_METHOD(Test)
		{
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();    // resolve later
			Promise<int> p1(1);                                                // preresolved
			Promise<string> p2("Hello");                                       // preresolved
			Promise<double> p3(3.3);                                           // preresolved

			bool areAllResolved = false;

			// Top of the hierarchy (should be preresolved)
			PromiseAll pa1 = PromiseAll({ p1, p2, p3 }).Then([&](auto states)
				{
					Assert::AreEqual(1, states[0]->value<int>());
					Assert::AreEqual(std::string("Hello"), states[1]->value<std::string>());
					Assert::AreEqual(3.3, states[2]->value<double>());
				});

			// Bottom of the hierarchy (should resolve when p0 resolves)
			PromiseAll pa2 = PromiseAll({ pa1, p0 }).Then([&](auto states)
				{
					areAllResolved = true;
				});

			Assert::IsFalse(areAllResolved);
			p0state->resolve();  // Resolve
			Assert::IsTrue(areAllResolved);
		}
	};
	//***************************************************************************************
};
