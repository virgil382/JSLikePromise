#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>
#include <utility>  // std::forward

#include "JSLikeBasePromise.hpp"

using namespace std;

namespace JSLike {

  // Forward declarations needed to declare "friendships" among structs.
  template<typename T> struct Promise;
  struct PromiseAll;
  struct PromiseAny;
  struct PromiseAnyState;


  template<typename T=void>
  struct PromiseState : public BasePromiseState {
    typedef shared_ptr<PromiseState<T>> ptr;

    PromiseState() = default;

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
    PromiseState(const PromiseState&) = delete;
    PromiseState(PromiseState&& other) = delete;
    PromiseState& operator=(const PromiseState&) = delete;

    /**
     * Resolve the Promise to the specified value, and resume the coroutine or
     * execute the "Then" Lambda.
     */
    virtual void resolve(T const &result) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_result = make_shared<T>(result);      // copy
      BasePromiseState::resolve(shared_from_this());
    }

    virtual void resolve(T&& result) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_result = make_shared<T>(forward<T>(result));      // move
      BasePromiseState::resolve(shared_from_this());
    }

    T &value() {
      return *m_result;
    }

  private:
    friend struct Promise<T>;
    friend struct PromiseAll;
    friend struct PromiseAny;
    friend struct PromiseAnyState;

    // resolve() constructs m_result, so there is no need for T to have a default constructor.
    shared_ptr<T>                                      m_result;
    // TODO: Pass m_result down the chain of PromiseStates instead of resolving each by reference.
  };

  /******************************************************************************************/

  template<>
  struct PromiseState<void> : public BasePromiseState {
    PromiseState() : BasePromiseState() {}

    // Don't allow copies of any kind.  Always use shared_ptr to instances of this type.
    PromiseState(const PromiseState&) = delete;
    PromiseState(PromiseState&& other) = delete;
    PromiseState& operator=(const PromiseState&) = delete;
  };

  /******************************************************************************************/

  template<typename T=void>
  struct Promise : public BasePromise {
  protected:
    Promise() = default;

  public:
    Promise(function<void(shared_ptr<PromiseState<T>>)> initializer)
    {
      auto s = state();
      try {
        initializer(s);
      }
      catch (...) {
        s->reject(current_exception());
      }
    }

    Promise(T const &val)
    {
      state()->resolve(val);
    }

    Promise(T &&val)
    {
      state()->resolve(forward<T>(val));
    }

    // The state of the Promise is constructed by this Promise and it is shared with a promise_type or
    // an awaiter_type, and with a "then" Lambda to resolve/reject the Promise.
    shared_ptr<PromiseState<T>> state() {
      if (!m_state) m_state = make_shared<PromiseState<T>>();

      auto castState = dynamic_pointer_cast<JSLike::PromiseState<T>>(m_state);
      return castState;
    }

    T &value() {
      return state()->value();
    }

    /**
     * Promise.Then() is used by normal routines (i.e. not coroutines) to specify a Lambda to be invoked
     * after the Promise is resolved.  This variant of "Then" expects a Lambda whose parameter
     * is a regular reference to the resolved value (stored in the Promise state).
     * 
     * @param thenLambda A function to be called after the Promise is resolved.
     * @return Another Promise<T> "chained" to this one that will be resolved when this Promise<T> is
     *         resolved.
     */
    Promise Then(function<void(T &)> thenLambda) {
      Promise chainedPromise;
      shared_ptr<PromiseState<T>> chainedPromiseState = chainedPromise.state();

      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          T &result(resultState->value<T>());
          thenLambda(result);
          chainedPromiseState->resolve(result);  // TODO: Resolve by using the shared pointer to avoid copy/move
        },
        [chainedPromiseState](auto ex)
        {
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * Promise.Then() is used by normal routines (i.e. not coroutines) to specify a Lambda to be invoked
     * after the Promise is resolved.  This variant of "Then" expects a Lambda whose parameter
     * is an rvalue that refers the (resolved) value stored in the Promise state.
     *
     * The Lambda is expected to move (i.e. pilfer the resouurces of the value stored in the Promise state).
     * So it makes no sense to allow another Promise to be chained to this one because if we did, then
     * that Promise could not provide a valid value to the Lambda that might be "Then"ed to it.  Therefore, 
     * this method does not return a chained Promise.
     *
     * @param thenLambda A Lambda function to be called after the Promise is resolved.  The Lambda is
     *        invoked expected to move the value stored in the Promise state by using the rvalue provided
     *        to it.
     */
    void Then(function<void(T&&)> thenLambda) {
      auto thisPromiseState = state();
      thisPromiseState->Then(
        [thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          T& result(resultState->value<T>());
          thenLambda(move(result));
        });
    }

    Promise Catch(function<void(exception_ptr)> catchLambda) {
      Promise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState](shared_ptr<BasePromiseState> resultState)
        {
          // Do a move-resolve because there is no Then attached to thisPromiseState, and therefore
          // no Lambda to ever "look" at the result. So the result can be moved to the chainedPromise.
          // Needless to say, resolve() will only move the result if T implements a move assignment
          // operator, otherwise it will copy the result.
          chainedPromiseState->resolve(move(resultState->value<T>()));  // TODO: Resolve by using the shared pointer to avoid copy/move
        },
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    Promise Then(function<void(T&)> thenLambda, function<void(exception_ptr)> catchLambda) {
      Promise chainedPromise;
      shared_ptr<PromiseState<T>> chainedPromiseState = chainedPromise.state();

      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          T &result(resultState->value<T>());
          thenLambda(result);
          chainedPromiseState->resolve(result);  // TODO: Resolve by using the shared pointer to avoid copy/move
        },
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    void Then(function<void(T&&)> thenLambda, function<void(exception_ptr)> catchLambda) {
      auto thisPromiseState = state();
      thisPromiseState->Then(
        [thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          thenLambda(move(resultState->value<T>()));
        },
        [catchLambda](auto ex)
        {
          catchLambda(ex);
        });
    }

    /**
     * A promise_type is created each time a coroutine that returns Promise<T> is called.
     * The promise_type immediately creates Promise<T>, which is returned to the caller.
     */
    struct promise_type : promise_type_base {
      // Called when a coroutine is invoked.  This method returns a Promise<T> to the caller.
      Promise get_return_object() {
        Promise p;
        m_state = p.state();
        return p;
      }

      // Called when a coroutine calls co_return with T.
      void return_value(T &&val) {
        auto castState = dynamic_pointer_cast<PromiseState<T>>(m_state);
        castState->resolve(forward<T>(val));
      }

      // Called when a coroutine calls co_return with Promise<T>.
      void return_value(Promise coreturnedPromise) {
        auto savedPromiseState = dynamic_pointer_cast<PromiseState<T>>(m_state);
        auto coreturnedPromiseState = coreturnedPromise.state();

        coreturnedPromiseState->Then(
          [savedPromiseState, coreturnedPromiseState](shared_ptr<BasePromiseState> result)
          {
            // coreturnedPromise got resolved, so resolve savedPromiseState
            savedPromiseState->resolve(move(coreturnedPromiseState->value()));  // TODO: Resolve by using the shared pointer to avoid copy/move
          },
          [savedPromiseState](auto ex)
          {
            // coreturnedPromise got rejected, so reject savedPromiseState
            savedPromiseState->reject(ex);
          });
      }
    };


    /**
     * An awaiter_type is constructed each time a coroutine co_awaits on a Promise<T>.
     */
    struct awaiter_type : awaiter_type_base {
      awaiter_type() = delete;
      awaiter_type(const awaiter_type&) = delete;
      awaiter_type(awaiter_type&& other) = default;
      awaiter_type& operator=(const awaiter_type&) = delete;

      awaiter_type(shared_ptr<BasePromiseState> state) : awaiter_type_base(state) {}

      /**
       * Called right before the call to co_await completes in order to return the result
       * to the coroutine.
       *
       * @return An rvalue reference to the value with which resolve() was called.  It is
       *         up to the coroutine to move the value if appropriate.
       */
      T &await_resume() {
        m_state->rethrowIfRejected();

        shared_ptr<PromiseState<T>> castState = dynamic_pointer_cast<PromiseState<T>>(m_state);

        return castState->value();
      }
    };

    awaiter_type operator co_await() {
      awaiter_type a(state());
      return a;
    }



    /**
     * Create an unresolved Promise<T> and return it and its PromiseState<T>.  Call this method like this:
     * 
     *   auto [p0, p0state] = Promise&lt;T&gt;::getUnresolvedPromiseAndState();
     *
     * @return A std::pair containing the Promise&lt;T&gt; and its PromiseState&lt;T&gt;.
     */
    pair<Promise<T>, shared_ptr<PromiseState<T>>> static getUnresolvedPromiseAndState() {
      shared_ptr<PromiseState<T>> state;
      Promise<T> p([&](auto promiseState)
        {
          state = promiseState;
        });

      return { move(p), state };
    }
  };






  template<>
  struct Promise<void> : BasePromise {
  protected:
    Promise(bool isResolved) {
      if(isResolved)
        state()->resolve();
    }
  public:
    Promise() {
      state()->resolve();
    }

    Promise(function<void(shared_ptr<PromiseState<>>)> initializer)
    {
      auto s = state();
      try {
        initializer(s);
      }
      catch (...) {
        s->reject(current_exception());
      }
    }

    Promise Then(function<void()> thenLambda) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();

      shared_ptr<PromiseState<>> thisPromiseState = state();
      thisPromiseState->Then([chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          thenLambda();
          chainedPromiseState->resolve();
        },
        [chainedPromiseState](auto ex)
        {
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    Promise Catch(function<void(exception_ptr)> catchLambda) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();
      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState](shared_ptr<BasePromiseState> resultState)
        {
          chainedPromiseState->resolve();
        },
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    Promise Then(function<void()> thenLambda, function<void(exception_ptr)> catchLambda) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();

      shared_ptr<PromiseState<>> thisPromiseState = state();
      thisPromiseState->Then([chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          thenLambda();
          chainedPromiseState->resolve();
        },
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }


    /**
     * A promise_type is created each time a coroutine that returns BasePromise is called.
     * The promise_type immediately creates BasePromise, which is returned to the caller.
     */
    struct promise_type : promise_type_base {
      // Called when a coroutine is invoked.  This method returns a BasePromise to the caller.
      Promise get_return_object() {
        Promise p(false);
        m_state = p.state();
        return p;
      }

      void return_void() {
        m_state->resolve();
      }
    };

    /**
     * An awaiter_type is constructed each time a coroutine co_awaits on a BasePromise.
     */
    struct awaiter_type : awaiter_type_base {
      awaiter_type() = delete;
      awaiter_type(const awaiter_type&) = delete;
      awaiter_type(awaiter_type&& other) = default;
      awaiter_type& operator=(const awaiter_type&) = delete;

      awaiter_type(shared_ptr<PromiseState<>> state) : awaiter_type_base(state) {}

      /**
       * Called right before the call to co_await completes in order to give the awaiter_type
       * to rethrow the exception with which the BasePromise was rejected (via its BasePromiseState).
       * The xecption is rethrown in the context of the coroutine (e.g. so that it may optionally
       * catch/handle the exception).
       */
      void await_resume() const {
        m_state->rethrowIfRejected();
      }
    };

    // The state of the Promise is constructed by this Promise and it is shared with a promise_type or
    // an awaiter_type, and with a "then" Lambda to resolve/reject the Promise.
    shared_ptr<PromiseState<>> state() {
      if (!m_state) m_state = make_shared<PromiseState<>>();

      auto castState = dynamic_pointer_cast<JSLike::PromiseState<>>(m_state);
      return castState;
    }

    awaiter_type operator co_await() {
      awaiter_type a(state());
      return a;
    }

    /**
     * Create an unresolved Promise<> and return it and its PromiseState<>.  Call this method like this:
     *
     *   auto [p0, p0state] = Promise&lt;&gt;::getUnresolvedPromiseAndState();
     *
     * @return A std::pair containing the Promise&lt;&gt; and its PromiseState&lt;&gt;.
     */
    pair<Promise, shared_ptr<PromiseState<>>> static getUnresolvedPromiseAndState() {
      shared_ptr<PromiseState<void>> state;
      Promise p([&](auto promiseState)
        {
          state = promiseState;
        });

      return { move(p), state };
    }
  };

};
