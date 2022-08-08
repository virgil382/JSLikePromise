#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>
#include <concepts>

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
      m_result = result;
      BasePromiseState::resolve(shared_from_this());
    }

    T &value() {
      return m_result;
    }

  private:
    friend struct Promise<T>;
    friend struct PromiseAll;
    friend struct PromiseAny;
    friend struct PromiseAnyState;

    T                                       m_result;
  };

  /******************************************************************************************/

  template<>
  struct PromiseState<void> : public BasePromiseState {
    PromiseState() : BasePromiseState() {}

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
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
    Promise(std::function<void(shared_ptr<PromiseState<T>>)> initializer)
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

    // The state of the Promise is constructed by this Promise and it is shared with a promise_type or
    // an awaiter_type, and with a "then" Lambda to resolve/reject the Promise.
    std::shared_ptr<PromiseState<T>> state() {
      if (!m_state) m_state = std::make_shared<PromiseState<T>>();

      auto castState = std::dynamic_pointer_cast<JSLike::PromiseState<T>>(m_state);
      return castState;
    }

    T const &value() {
      return state()->value();
    }

    /**
     * Promise.Then() is used by regular functions to specify a Lambda to be executed after the Promise
     * is complete (i.e. the Promise::promise_type::return_value() was called).  The Lambda is saved in the
     * PromiseState, so it exists for as long as the VPromiseState exists.  However, beware of the
     * lifecycle of any variables that the Lambda may capture.
     * 
     * @param thenLambda A function to be called after the Promise is resolved.
     * @return Another Promise<T> that will be resolved when this Promise<T> is resolved.
     */
    Promise Then(function<void(T)> thenLambda) {
      Promise chainedPromise;
      shared_ptr<PromiseState<T>> chainedPromiseState = chainedPromise.state();

      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          auto const &result(resultState->value<T>());
          thenLambda(result);
          chainedPromiseState->resolve(result);
        },
        [chainedPromiseState](auto ex)
        {
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    Promise Catch(function<void(exception_ptr)> catchLambda) {
      Promise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState](shared_ptr<BasePromiseState> resultState)
        {
          auto const& result(resultState->value<T>());
          chainedPromiseState->resolve(result);
        },
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    Promise Then(function<void(T)> thenLambda, function<void(exception_ptr)> catchLambda) {
      Promise chainedPromise;
      shared_ptr<PromiseState<T>> chainedPromiseState = chainedPromise.state();

      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resultState)
        {
          auto const& result(resultState->value<T>());
          thenLambda(result);
          chainedPromiseState->resolve(result);
        },
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
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

      // Called as a result of a coroutine calling co_return.
      void return_value(T val) {
        auto castState = dynamic_pointer_cast<PromiseState<T>>(m_state);
        castState->resolve(val);
      }

      void return_value(Promise coreturnedPromise) {
        auto savedPromiseState = dynamic_pointer_cast<PromiseState<T>>(m_state);
        auto coreturnedPromiseState = coreturnedPromise.state();

        coreturnedPromiseState->Then(
          [savedPromiseState, coreturnedPromiseState](shared_ptr<BasePromiseState> result)
          {
            // coreturnedPromise got resolved, so resolve savedPromiseState
            savedPromiseState->resolve(coreturnedPromiseState->value());
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

      awaiter_type(std::shared_ptr<BasePromiseState> state) : awaiter_type_base(state) {}

      /**
       * Called right before the call to co_await completes in order to return the result
       * to the coroutine.
       *
       * @return The value with which resolve() was called.
       */
      constexpr T await_resume() const {
        m_state->rethrowIfRejected();

        auto castState = std::dynamic_pointer_cast<JSLike::PromiseState<T>>(m_state);
        return castState->m_result;
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
    std::pair<Promise<T>, std::shared_ptr<PromiseState<T>>> static getUnresolvedPromiseAndState() {
      std::shared_ptr<PromiseState<T>> state;
      Promise<T> p([&](auto promiseState)
        {
          state = promiseState;
        });

      return { std::move(p), state };
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

    Promise(std::function<void(shared_ptr<PromiseState<>>)> initializer)
    {
      auto s = state();
      try {
        initializer(s);
      }
      catch (...) {
        s->reject(current_exception());
      }
    }

    Promise Then(std::function<void()> thenLambda) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();

      std::shared_ptr<PromiseState<>> thisPromiseState = state();
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

    Promise Then(std::function<void()> thenLambda, function<void(exception_ptr)> catchLambda) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();

      std::shared_ptr<PromiseState<>> thisPromiseState = state();
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

      awaiter_type(std::shared_ptr<PromiseState<>> state) : awaiter_type_base(state) {}

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
    std::shared_ptr<PromiseState<>> state() {
      if (!m_state) m_state = std::make_shared<PromiseState<>>();

      auto castState = std::dynamic_pointer_cast<JSLike::PromiseState<>>(m_state);
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
    std::pair<Promise, std::shared_ptr<PromiseState<>>> static getUnresolvedPromiseAndState() {
      std::shared_ptr<PromiseState<void>> state;
      Promise p([&](auto promiseState)
        {
          state = promiseState;
        });

      return { std::move(p), state };
    }
  };

};
