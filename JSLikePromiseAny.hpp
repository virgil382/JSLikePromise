#pragma once

#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>
#include <vector>

#include "JSLikePromise.hpp"
#include "JSLikeBasePromise.hpp"

using namespace std;

namespace JSLike {
  struct PromiseAnyState : public PromiseState<std::shared_ptr<BasePromiseState>> {
    PromiseAnyState() = default;

  private:
    friend struct PromiseAny;

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
    PromiseAnyState(const PromiseAnyState&) = delete;
    PromiseAnyState(PromiseAnyState&& other) = delete;
    PromiseAnyState& operator=(const PromiseAnyState&) = delete;

    void init(std::shared_ptr<PromiseAnyState> &thisState, std::vector<JSLike::BasePromise>& monitoredPromises)
    {
      for (size_t i = 0, vectorSize = monitoredPromises.size(); i < vectorSize; i++) {
        auto monitoredPromise = monitoredPromises[i];
        auto monitoredPromiseState = monitoredPromise.m_state;

        // May call BasePromiseState::Then()  or  PromiseAnyState::Then()
        monitoredPromiseState->Then([thisState, monitoredPromiseState](shared_ptr<BasePromiseState> resultState)  // Note that the Lambda keeps a shared_ptr to this PromiseAllState
          {
            // Call PromiseAny::resolve()
            thisState->resolve(resultState);
          });

        monitoredPromise.Catch([thisState](auto ex)  // Note that the Lambda keeps a shared_ptr to this PromiseAllState
          {
            thisState->reject(ex);
          });

        // Optimization: If already resolved/rejected, then don't add any more "Then" or "Catch".
        if (m_eptr || m_isResolved) return;
      }
    }

    // Overrie BasePromiseState::Then()
    void Then(std::function<void(shared_ptr<BasePromiseState>)> thenLambda) override {
      m_thenLambda = thenLambda;

      if (m_isResolved) {
        thenLambda(m_result); // Difference vs. BasePromiseState
      }
    }

    // Override PromiseState::resolve()
    void resolve(shared_ptr<BasePromiseState> const &result) override {
      if (m_eptr || m_isResolved) return;
      m_result = result;
      BasePromiseState::resolve(result);  // Difference vs. PromiseState
    }
  };  // PromiseAnyState


  /**
   * A PromiseAny is initialized/constructed with a vector of BasePromises (e.g. BasePromises,
   * Promise<T>s, or other PromiseAnys), which it then monitors for resolution or rejection.
   *
   * After one of the BasePromises is resolved, the PromiseAny's "Then" lambda is called with a
   * PromiseAny::ResultType (see typedef), which is a shared pointer to the
   * BasePromiseState (of the BasePromises) that was resolved.  The Lambda can retrieve the
   * type and value of the (derived) BasePromiseState by calling BasePromiseState::isValueOfType<T>()
   * and BasePromiseState::value<T>().  Example:
   * 
   * void myFunction(BasePromise &p0, BasePromise &p1, BasePromise &p2, BasePromise &p3)
   * {
   *   PromiseAny pa({ p0, p1, p2, p3 });  // One of these promises is resolved elsewhere.
   *   pa.Then([&](auto state)             // The type of states is PromiseAny::ResultType.
   *     {
   *       if(state->isValueOfType<int>())    cout << state->value<int>();
   *       if(state->isValueOfType<string>()) cout << state->value<string>();
   *       if(state->isValueOfType<double>()) cout << state->value<double>();
   *       // In this example we suppose that, p0 is BasePromiseState, so it doesn't have a value.
   *     });
   * }
   * 
   * A PromiseAny is rejected if any of the BasePromises with which it is constructed/initialized
   * is rejected.  It's "Catch" Lambda is called with the same exception_ptr with which the
   * BasePromise was rejected.
   *
   * Coroutines can return a PromiseAny, which is "wired up" to settle at the same time and with
   * the same result as the PromiseAany resulting from co_return.
   */
  struct PromiseAny : public BasePromise {
  public:
    typedef shared_ptr<BasePromiseState> ResultType;

    /**
     * Construct a PromiseAny with the vector of BasePromises that it will monitor for resolution/rejection.
     * @param promises The vector of BasePromises.
     */
    PromiseAny(std::vector<JSLike::BasePromise> promises)
    {
      auto s = state();
      s->init(s, promises);
    }

    shared_ptr<PromiseAnyState> state() {
      if (!m_state) m_state = make_shared<PromiseAnyState>();

      auto castState = dynamic_pointer_cast<PromiseAnyState>(m_state);
      return castState;
    }

    PromiseAny Then(std::function<void(shared_ptr<BasePromiseState>)> thenLambda) {
      PromiseAny chainedPromise;  // The new "chained" Promise that we'll return to the caller.
      auto chainedPromiseState = chainedPromise.state();

      // Create a "bridge" between the current Promise and the new "chained" Promise that we'll return.
      // When this Promise gets settled, one of the following two Lambdas will settle the "chained" promise.
      // Note that the Lambdas only operate on the PromiseState of each Promise.  The Promises are just
      // "handles" to the Promise states.
      auto currentState = state();
      currentState->BasePromiseState::Then(
        // Then Lambda
        [chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resolvedState)
        {
          thenLambda(resolvedState);
          chainedPromiseState->resolve(resolvedState);
        },
        // Catch Lambda
        [chainedPromiseState](exception_ptr ex)
        {
          chainedPromiseState->reject(ex);
        }
      );

      return chainedPromise;
    }

    PromiseAny Catch(std::function<void(std::exception_ptr)> catchLambda) {
      PromiseAny chainedPromise;
      auto chainedPromiseState = chainedPromise.state();

      auto currentState = state();

      // Create a "bridge" between the current Promise and the new "chained" Promise that we'll return.
      // When this Promise gets settled, one of the following two Lambdas will settle the "chained" promise.
      // Note that the Lambdas only operate on the PromiseState of each Promise.  The Promises are just
      // "handles" to the Promise states.
      currentState->BasePromiseState::Then(
        // Then Lambda
        [chainedPromiseState](shared_ptr<BasePromiseState> resolvedState)
        {
          chainedPromiseState->resolve(resolvedState);
        },
        // Catch Lambda
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    PromiseAny Then(std::function<void(shared_ptr<BasePromiseState>)> thenLambda, std::function<void(std::exception_ptr)> catchLambda) {
      PromiseAny chainedPromise;  // The new "chained" Promise that we'll return to the caller.
      auto chainedPromiseState = chainedPromise.state();

      // Create a "bridge" between the current Promise and the new "chained" Promise that we'll return.
      // When this Promise gets settled, one of the following two Lambdas will settle the "chained" promise.
      // Note that the Lambdas only operate on the PromiseState of each Promise.  The Promises are just
      // "handles" to the Promise states.
      auto currentState = state();
      currentState->BasePromiseState::Then(
        // Then Lambda
        [chainedPromiseState, thenLambda](shared_ptr<BasePromiseState> resolvedState)
        {
          thenLambda(resolvedState);
          chainedPromiseState->resolve(resolvedState);
        },
        // Catch Lambda
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * A promise_type is created each time a coroutine that returns PromiseAny is called.
     * The promise_type immediately creates PromiseAny, which is returned to the caller.
     */
    struct promise_type : promise_type_base {
      /**
       * Called when invoking a coroutine that returns a PromiseAny.  This method creates
       * a PromiseAny to return to the caller and saves a shared_ptr to its PromiseAny in
       * this promise_type.
       *
       * Later, return_value() "wires up" the saved PromiseAnyState so that it settles
       * when the PromiseAny evaluated by co_return settles.
       *
       * @return A pending PromiseAny that is not yet "wired up" to the PromiseAny evaluated
       *         by co_retrun.
       */
      PromiseAny get_return_object() {
        PromiseAny p;
        m_state = p.state();
        return p;
      }

      /**
       * Called after co_return evaluates an expression that results in a PromiseAny.  If the
       * resulting PromiseAny is settled, then this method immediately settles (i.e. resolves
       * or rejects) the PromiseAnyState saved by get_return_object().  Otherwise, it adds
       * Lambdas that settle the saved PromiseAnyState when the resulting PromiseAny settles.
       *
       * In essence, this method "wires up" the PromiseAny resulting from a co_return, to the
       * PromiseAny returned when the coroutine was called so that both settle in the same way.
       *
       * @param coreturnedPromiseAny The PromiseAny returned by co_return (i.e. that results from
       *        evaluating the expression following co_return).
       */
      void return_value(PromiseAny coreturnedPromiseAny) {
        auto savedPromiseAnyState = std::dynamic_pointer_cast<PromiseAnyState>(m_state);
        auto coreturnedPromiseAnyState = coreturnedPromiseAny.state();

        // Use a "Then" Lambda to get notified if coreturnedPromiseAll gets resolved.
        coreturnedPromiseAnyState->Then([savedPromiseAnyState, coreturnedPromiseAnyState](shared_ptr<BasePromiseState> resultState)
          {
            // coreturnedPromiseAny got resolved, so resolve savedPromiseAnyState
            savedPromiseAnyState->resolve(coreturnedPromiseAnyState->m_result);
          });

        // Use a "Catch" Lambda to get notified if savedPromiseAnyState gets rejected.
        coreturnedPromiseAnyState->Catch([savedPromiseAnyState](auto ex)
          {
            // coreturnedPromiseAny got rejected, so reject savedPromiseAnyState
            savedPromiseAnyState->reject(ex);
          });
      }
    };



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
      shared_ptr<BasePromiseState> await_resume() const {
        m_state->rethrowIfRejected();

        shared_ptr<PromiseAnyState> castState = std::dynamic_pointer_cast<PromiseAnyState>(m_state);
        return castState->m_result;
      }
    };

    /**
     * Called by coroutines when they co_await on a PromiseAny.
     * @return A PromiseAny::awaiter_type that suspends/resumes the coroutine as needed, and that returns to
     *         the coroutine the PromiseAny::ReturnType or rethrows an exception (e.g. for the coroutine to catch).
     */
    awaiter_type operator co_await() {
      awaiter_type a(state());
      return a;
    }


  private:
    PromiseAny() = default;

  }; // PromiseAny
}; // namespace JSLike
