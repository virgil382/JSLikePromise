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


  template<typename T>
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
    void resolve(T const &result) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_isResolved = true;
      m_result = result;

      if (m_h.address() != nullptr) {
        m_h();
      } else {
        if (m_thenVoidLambda)
          m_thenVoidLambda();
        if (m_thenLambda)
          m_thenLambda(m_result);
      }
    }

    T const &value() const {
      return m_result;
    }

  private:
    friend struct Promise<T>;
    friend struct PromiseAll;
    friend struct PromiseAny;

    void Then(std::function<void(T)> thenLambda) {
      m_thenLambda = thenLambda;

      if (m_isResolved) {
        thenLambda(m_result);
      }
    }

    std::function<void(T)>                  m_thenLambda;  // TODO: Make this a list of Lambdas
    T                                       m_result;
  };


  template<typename T=void>
  struct Promise : public BasePromise {
  protected:
    Promise() = default;

  public:
    Promise(std::function<void(shared_ptr<PromiseState<T>>)> initializer)
    {
      //  Call the Labda.  Pass it the state so it can resolve it (immediately or later).
      initializer(state());
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
    Promise Then(std::function<void(T)> thenLambda) {

      // TODO: Consider calling the Lambda with the state, not with the value.  This will require the
      // Lambda's implementation to retrieve the value from the state.

      Promise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      state()->Then([chainedPromiseState, thenLambda](auto result)
        {
          thenLambda(result);
          chainedPromiseState->resolve(result);
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
        auto castState = std::dynamic_pointer_cast<JSLike::PromiseState<T>>(m_state);
        castState->resolve(val);
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
    Promise() = default;
  public:

    Promise(std::function<void(shared_ptr<BasePromiseState>)> initializer)
    {
      initializer(state());
    }
  };

};
