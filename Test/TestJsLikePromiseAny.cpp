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
	TEST_CLASS(TestResolution)
	{
	public:
		TEST_METHOD(SomePreresolved_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // resolved later
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			bool areSomeResolved = false;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then([&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					areSomeResolved = true;
				});

			Assert::IsTrue(areSomeResolved);
		}

		TEST_METHOD(NonePreresolved_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // resolved later
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // resolved later
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // resolved later

			bool areSomeResolved = false;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then([&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					areSomeResolved = true;
				});

			Assert::IsFalse(areSomeResolved);
			p1state->resolve(1);
			Assert::IsTrue(areSomeResolved);
		}

		TEST_METHOD(SomePreresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then([&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				}).Then([&](auto state)
					{
						Assert::AreEqual(1, state->value<int>());
						nThenCalls++;
					});

				Assert::AreEqual(2, nThenCalls);
		}

		TEST_METHOD(SomePreresolved_Catch_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 })
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto state)
					{
						Assert::AreEqual(1, state->value<int>());
						nThenCalls++;
					});

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(SomePreresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 })
				.Then([&](auto state)
					{
						Assert::AreEqual(1, state->value<int>());
						nThenCalls++;
					})
				.Catch([&](auto state) { nCatchCalls++; });

			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(SomePreresolved_ThenCatch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then(
				[&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto state) { nCatchCalls++; });

				Assert::AreEqual(1, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(SomePreresolved_ThenCatch_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();  // never resolved
			Promise<int> p1(1);                                              // preresolved
			Promise<string> p2("Hello");                                     // preresolved
			Promise<double> p3(3.3);                                         // preresolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then(
				[&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto state) { nCatchCalls++; }).Then(
				[&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				});

			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(NonePreresolved_Then_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then([&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				}).Then([&](auto state)
					{
						Assert::AreEqual(1, state->value<int>());
						nThenCalls++;
					});

				Assert::AreEqual(0, nThenCalls);
				p1state->resolve(1);  // Resolve p1
				Assert::AreEqual(2, nThenCalls);
		}

		TEST_METHOD(NonePreresolved_Then_Catch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then([&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				})
				.Catch([&](auto ex) { nCatchCalls++; });

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p1state->resolve(1);  // Resolve p1
				Assert::AreEqual(1, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(NonePreresolved_ThenCatch)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then(
				[&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; });

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p1state->resolve(1);  // Resolve p1
			Assert::AreEqual(1, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(NonePreresolved_ThenCatch_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 }).Then(
				[&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				},
				[&](auto ex) { nCatchCalls++; }).Then(
				[&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				});

			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
			p1state->resolve(1);  // Resolve p1
			Assert::AreEqual(2, nThenCalls);
			Assert::AreEqual(0, nCatchCalls);
		}

		TEST_METHOD(NonePreresolved_Catch_Then)
		{
			// Create a few Promises to give to PromiseAny
			auto [p0, p0state] = Promise<>::getUnresolvedPromiseAndState();        // never resolved
			auto [p1, p1state] = Promise<int>::getUnresolvedPromiseAndState();     // resolved later
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // never resolved
			auto [p3, p3state] = Promise<double>::getUnresolvedPromiseAndState();  // never resolved

			int nThenCalls = 0;
			int nCatchCalls = 0;

			PromiseAny pa = PromiseAny({ p0, p1, p2, p3 })
				.Catch([&](auto ex) { nCatchCalls++; })
				.Then([&](auto state)
				{
					Assert::AreEqual(1, state->value<int>());
					nThenCalls++;
				});

				Assert::AreEqual(0, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
				p1state->resolve(1);  // Resolve p1
				Assert::AreEqual(1, nThenCalls);
				Assert::AreEqual(0, nCatchCalls);
		}
	};
	//***************************************************************************************
	TEST_CLASS(TestHierarchyOfPromiseAny)
	{
	public:
		TEST_METHOD(Test)
		{
			// Get 2 pre-resolved Promises, and 1 onresolved Promise.
			auto p1 = Promise<int>([](auto junk) {});  // won't resolve
			auto [p2, p2state] = Promise<string>::getUnresolvedPromiseAndState();  // will resolve later
			auto p3 = Promise<double>([](auto junk) {});  // won't resolve
			PromiseAny pa1 = PromiseAny({ p1, p2, p3 });

			auto p0 = BasePromise([](auto junk) {});  // won't resolve

			bool areAnyResolved = false;
			PromiseAny pa2 = PromiseAny({ pa1, p0 }).Then([&](shared_ptr<BasePromiseState> result)
				{
					//// pa2 was resolved, because pa1 was resolved, becasue p2 was resolved.
					Assert::AreEqual(std::string("Hello"), result->value<string>());
					areAnyResolved = true;
				});

			Assert::IsFalse(areAnyResolved);
			p2state->resolve("Hello");  // Resolve p2 --> resolves pa1 --> resolves pa2
			Assert::IsTrue(areAnyResolved);
		}
	};
	//***************************************************************************************
	TEST_CLASS(Test_co_await)
	{
	private:
		Promise<bool> myCoAwaitingCoroutine(PromiseAny &p) {

			std::shared_ptr<BasePromiseState> result = co_await p;
			Assert::AreEqual(1, result->value<int>());

			co_return true;
		}

		Promise<bool> myCoAwaitingCoroutineThatCatches(PromiseAny& p) {
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
			PromiseAny pa = PromiseAny({ p1, p2, p3 });  // Preresolved

			auto result = myCoAwaitingCoroutine(pa);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(ResolvedLater)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAny({ p0, p1, p2 });
			PromiseAny pa = PromiseAny({ p0, p1, p2 });  // Not yet resolved

			auto result = myCoAwaitingCoroutine(pa);
			Assert::IsFalse(result.isResolved());

			p0state->resolve(1);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		}

		TEST_METHOD(Reject_try_catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto pa = PromiseAny({ p0, p1, p2 });

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
			auto pa = PromiseAny({ p0, p1, p2 });

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
		PromiseAny CoReturnPromiseAny(PromiseAny& p) {
			co_return p;
		}

		Promise<bool> CoAwait(PromiseAny& p) {
			co_await CoReturnPromiseAny(p);
			co_return true;
		}

		PromiseAny CoroutineThatThrows() {
			char c = std::string().at(1); // this throws a std::out_of_range
			co_return PromiseAny(vector<BasePromise>{});
		}

	public:
		TEST_METHOD(Preresolved_Then)
		{
			auto p1 = Promise<int>(1);
			auto p2 = Promise<string>("Hello");
			auto p3 = Promise<double>(3.3);
			auto p = PromiseAny({ p1, p2, p3 });


			bool wasThenCalled = false;
			CoReturnPromiseAny(p).Then([&](auto result)
				{
					Assert::AreEqual(1, result->value<int>());
					wasThenCalled = true;
				});

			Assert::IsTrue(wasThenCalled);
		}

		TEST_METHOD(Reject_Catch)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAny({ p0, p1, p2 });

			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa = CoReturnPromiseAny(p);
			pa.Then([&](auto result) { nThenCalls++; });
			pa.Catch([&](auto ex) { nCatchCalls++; });

			Assert::IsFalse(pa.isRejected());

			// Resolve 1 Promises and reject 2.
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
			auto p = PromiseAny({ p0, p1, p2 });


			auto result = CoAwait(p);

			Assert::IsFalse(result.isResolved());
			p0state->resolve(1);
			Assert::IsTrue(result.isResolved());
			Assert::IsTrue(result.value() == true);
		};

		TEST_METHOD(ResolvedLater_Then)
		{
			auto [p0, p0state] = Promise<int>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();
			auto p = PromiseAny({ p0, p1, p2 });

			bool wasThenCalled = false;
			CoReturnPromiseAny(p).Then([&](auto result)
				{
					Assert::AreEqual(1, result->value<int>());
					wasThenCalled = true;
				});

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
			PromiseAny pa1;
			PromiseAny pa2 = pa1;

			Assert::IsTrue(pa1.state() == pa2.state());
		}

		TEST_METHOD(Copy)
		{
			PromiseAny pa1;
			PromiseAny pa2(pa1);

			Assert::IsTrue(pa1.state() == pa2.state());
		}

		TEST_METHOD(Default)
		{
			PromiseAny pa;
			Assert::IsTrue(pa.isRejected());
		}

		TEST_METHOD(EmptyVector)
		{
			PromiseAny pa(vector<BasePromise>{});
			Assert::IsTrue(pa.isResolved());
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
			PromiseAny pa = PromiseAny({ p0, p1, p2 })
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
			PromiseAny pa = PromiseAny({ p0, p1, p2 })
				.Catch([&](auto ex) { nCatchCalls++; })
				.Catch([&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::AreEqual(2, nCatchCalls);
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
			PromiseAny pa = PromiseAny({ p0, p1, p2 })
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

		TEST_METHOD(Then_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa = PromiseAny({ p0, p1, p2 })
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

		TEST_METHOD(ThenCatch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa = PromiseAny({ p0, p1, p2 }).Then(
				[&](auto result) { nThenCalls++; },
				[&](auto ex) { nCatchCalls++; });

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

		TEST_METHOD(ThenCatch_Catch)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa = PromiseAny({ p0, p1, p2 }).Then(
				[&](auto result) { nThenCalls++; },
				[&](auto ex) { nCatchCalls++; }).Catch(
				[&](auto ex) { nCatchCalls++; });

			// Reject p1.  The "Catch" Lambda should be called.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));
			// Reject p1 again to verify that Catch isn't called multiple times.
			p1state->reject(make_exception_ptr(out_of_range("invalid string position")));

			// Verify the result
			Assert::IsTrue(pa.isRejected());
			Assert::IsFalse(pa.isResolved());
			Assert::AreEqual(0, nThenCalls);
			Assert::AreEqual(2, nCatchCalls);
		}

		TEST_METHOD(ThenCatch_Then)
		{
			// Create 3 Promises to give to PromiseAny.  Save their PromiseStates.
			auto [p0, p0state] = Promise<bool>::getUnresolvedPromiseAndState();
			auto [p1, p1state] = Promise<string>::getUnresolvedPromiseAndState();
			auto [p2, p2state] = Promise<double>::getUnresolvedPromiseAndState();

			// "Wire-up" the SUT.
			int nThenCalls = 0;
			int nCatchCalls = 0;
			PromiseAny pa = PromiseAny({ p0, p1, p2 }).Then(
				[&](auto result) { nThenCalls++; },
				[&](auto ex) { nCatchCalls++; }).Then(
				[&](auto result) { nThenCalls++; });

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
