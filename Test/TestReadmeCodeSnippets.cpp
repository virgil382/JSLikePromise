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

namespace TestReadmeCodeSnippets
{
	//***************************************************************************************
	TEST_CLASS(ConsumerProducer)
	{
	private:
		class DeepThought {
		public:
			DeepThought() = default;

			void cogitate(function<void(error_code errorCode, int theAnswer)> callback) {
				callback(error_code(0, generic_category()), 42);
			}
		};

		DeepThought deepThoughtAPI;

#include "../docs/producer.hpp"

	public:
		TEST_METHOD(DeepThoughtCogitate)
		{
#include "../docs/consumer.hpp"
		}
	};
	//***************************************************************************************
	TEST_CLASS(CoroutineIntegration_co_await_resolved)
	{
	private:
		Promise<int> taskThatReturnsAPromise() {
			co_return Promise<int>(1);
		}

		Promise<> coroutine1() {
			Promise<int> x = taskThatReturnsAPromise();
			int value = co_await x;  // Suspend here if x is not resolved.  Resume after x is resolved.
			cout << "value=" << value << "\n";
		}

	public:
		TEST_METHOD(DeepThoughtCogitate)
		{
			coroutine1();
		}
	};
	//***************************************************************************************
	TEST_CLASS(CoroutineIntegration_co_await_rejected)
	{
	private:
		Promise<int> taskThatReturnsAPromise() {
			auto p = Promise<int>([](auto promiseState) {
				promiseState->reject(make_exception_ptr(out_of_range("invalid string position")));
			});
			co_return p;
		}

		Promise<> coroutine1() {
			Promise<int> x = taskThatReturnsAPromise();
			int value;
			try {
				value = co_await x;  // Resume and throw if x is rejected.
			}
			catch (exception& ex) {
				cout << "ex=" << ex.what() << "\n";
			}
			cout << "value=" << value << "\n";
		}

	public:
		TEST_METHOD(DeepThoughtCogitate)
		{
			coroutine1();
		}
	};
	//***************************************************************************************
	TEST_CLASS(CoroutineIntegration_ObtainResolvedValue)
	{
	private:
		Promise<int> taskThatReturnsAnIntPromise() {
			co_return 1;
		}

		Promise<> coroutine1() {
			int& v = co_await taskThatReturnsAnIntPromise();
			cout << "v=" << v << "\n";
		}

	public:
		TEST_METHOD(GetValueVia_Then)
		{
			taskThatReturnsAnIntPromise().Then([](int& v) {
				cout << "v=" << v << "\n";
				});
		}

		TEST_METHOD(GetValueVia_co_return)
		{
			coroutine1();
		}
	};
	//***************************************************************************************
	TEST_CLASS(CoroutineIntegration_ResolvedValueLifecycle)
	{
	private:
		class MovableType {
		public:
			MovableType() = delete;
			MovableType(int i) : v(i) { }

			MovableType(const MovableType&) = delete;
			MovableType(MovableType&& other) noexcept { v = other.v; other.v = -1; }; // move constructor
			MovableType& operator=(const MovableType&) = delete;
			MovableType& operator=(MovableType&&) = delete;

			int internalValue() { return v; }
			int v;
		};

		class CopyableType {
		public:
			CopyableType() = delete;
			CopyableType(int i) : v(i) { }

			CopyableType(const CopyableType& other) { v = other.v; };  // copy constructor
			CopyableType(CopyableType&& other) = delete;
			CopyableType& operator=(const CopyableType&) = delete;
			CopyableType& operator=(CopyableType&&) = delete;

			int internalValue() { return v; }
			int v;
		};

		class ShareableFromThisType : public enable_shared_from_this<ShareableFromThisType> {
		public:
			ShareableFromThisType() = delete;
			ShareableFromThisType(int i) : v(i) { }

			ShareableFromThisType(const ShareableFromThisType&) = default;
			ShareableFromThisType(ShareableFromThisType&& other) = default;
			ShareableFromThisType& operator=(const ShareableFromThisType&) = default;
			ShareableFromThisType& operator=(ShareableFromThisType&&) = default;

			int internalValue() { return v; }
			int v;
		};

		Promise<MovableType> taskThatReturnsAMovableValueTypePromise() {
			co_return MovableType(1);
		}

		Promise<CopyableType> taskThatReturnsACopyableValueTypePromise() {
			co_return *(new CopyableType(1));
		}

		Promise<ShareableFromThisType> taskThatReturnsAShareableValueTypePromise() {
			co_return ShareableFromThisType(1);
		}

		Promise<> coroutineThatMovesTheValue() {
			// Get the lvalue reference.
			MovableType& v = co_await taskThatReturnsAMovableValueTypePromise();

			// Move the resolved value into a new instance of MovableType.
			MovableType vv(move(v));  // cast v to an rvalue to invoke the move constructor

			cout << "vv=" << vv.internalValue() << "\n";
		}

		Promise<> coroutineThatCopiesTheValue() {
			// Get the lvalue reference.
			CopyableType& v = co_await taskThatReturnsACopyableValueTypePromise();

			// Copy the resolved value to a new instance of CopyableType.
			CopyableType vv(v);  // use v as an lavalue to invoke the copy constructor

			cout << "vv=" << vv.internalValue() << "\n";
		}

		Promise<> coroutineThatGetsASharedPointerToTheValue() {
			// Copy the lvalue refrence to the resolved value.
			ShareableFromThisType& v = co_await taskThatReturnsAShareableValueTypePromise();

			// Get a shared_ptr to the resolved value.
			shared_ptr<ShareableFromThisType> vv(v.shared_from_this());

			cout << "vv=" << vv->internalValue() << "\n";
		}

	public:
		TEST_METHOD(Move)
		{
			coroutineThatMovesTheValue();
		}

		TEST_METHOD(MoveCaveat)
		{
			Promise<MovableType> p = taskThatReturnsAMovableValueTypePromise();
			p.Then([](MovableType& v) {
				// This Lambda will be called first.

				MovableType vv(move(v));  // cast v to an rvalue to invoke the move constructor
				cout << "vv=" << vv.internalValue() << "\n";

				}).Then([](MovableType& v) {
					// This Lambda will be called next.

					MovableType vv(move(v));  // BUG: v was moved already
					cout << "vv=" << vv.internalValue() << "\n";
				});
		}

		TEST_METHOD(Copy)
		{
			coroutineThatCopiesTheValue();
		}

		TEST_METHOD(Share)
		{
			coroutineThatGetsASharedPointerToTheValue();
		}

	};
	//***************************************************************************************
}
