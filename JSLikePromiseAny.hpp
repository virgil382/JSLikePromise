#pragma once

#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>
#include <vector>

#include "JSLikePromise.hpp"
#include "JSLikeVoidPromise.hpp"

using namespace std;

namespace JSLike {
  struct PromiseAnyState : public PromiseState< std::shared_ptr<VoidPromiseState>> {
    PromiseAnyState() = default;

  private:
    friend struct PromiseAny;

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
    PromiseAnyState(const PromiseAnyState&) = delete;
    PromiseAnyState(PromiseAnyState&& other) = delete;
    PromiseAnyState& operator=(const PromiseAnyState&) = delete;

    void init(std::shared_ptr<PromiseAnyState> thisState, std::vector<JSLike::VoidPromise>& monitoredPromises)
    {
      for (size_t i = 0, vectorSize = monitoredPromises.size(); i < vectorSize; i++) {
        auto monitoredPromiseState = monitoredPromises[i].m_state;

        monitoredPromiseState->Then([thisState, monitoredPromiseState]()  // Note that the Lambda keeps a shared_ptr to this PromiseAllState
          {
            thisState->resolve(monitoredPromiseState);
          });

        monitoredPromiseState->Catch([thisState](auto ex)  // Note that the Lambda keeps a shared_ptr to this PromiseAllState
          {
            thisState->reject(ex);
          });
      }
    }
  };  // PromiseAnyState


  /**
   * A PromiseAny is initialized/constructed with a vector of VoidPromises (e.g. VoidPromises,
   * Promise<T>s, or other PromiseAnys), which it then monitors for resolution or rejection.
   *
   * After one of the VoidPromises is resolved, the PromiseAny's "Then" lambda is called with a
   * PromiseAny::ResultType (see typedef), which is a shared pointer to the
   * VoidPromiseState (of the VoidPromises) that was resolved.  The Lambda can retrieve the
   * type and value of the (derived) VoidPromiseState by calling VoidPromiseState::isValueOfType<T>()
   * and VoidPromiseState::value<T>().  Example:
   * 
   * void myFunction(VoidPromise &p0, VoidPromise &p1, VoidPromise &p2, VoidPromise &p3)
   * {
   *   PromiseAny pa({ p0, p1, p2, p3 });  // One of these promises is resolved elsewhere.
   *   pa.Then([&](auto state)             // The type of states is PromiseAny::ResultType.
   *     {
   *       if(state->isValueOfType<int>())    cout << state->value<int>();
   *       if(state->isValueOfType<string>()) cout << state->value<string>();
   *       if(state->isValueOfType<double>()) cout << state->value<double>();
   *       // In this example we suppose that, p0 is VoidPromiseState, so it doesn't have a value.
   *     });
   * }
   * 
   * A PromiseAny is rejected if any of the VoidPromises with which it is constructed/initialized
   * is rejected.  It's "Catch" Lambda is called with the same exception_ptr with which the
   * VoidPromise was rejected.
   *
   * Coroutines can return a PromiseAny, which is "wired up" to settle at the same time and with
   * the same result as the PromiseAany resulting from co_return.
   */
  struct PromiseAny : public Promise<std::shared_ptr<VoidPromiseState>> {
  public:
    typedef std::shared_ptr<VoidPromiseState> ResultType;

    /**
     * Construct a PromiseAny with the vector of VoidPromises that it will monitor for resolution/rejection.
     * @param promises The vector of VoidPromises.
     */
    PromiseAny(std::vector<JSLike::VoidPromise> promises)
    {
      auto s = state();
      s->init(s, promises);
    }

    PromiseAny& Then(std::function<void(ResultType)> thenLambda) {
      state()->Then(thenLambda);
      return *this;
    }

    /**
     * Called by coroutines when they co_await on a PromiseAny.
     * @return A PromiseAny::awaiter_type that suspends/resumes the coroutine as needed, and that returns to
     *         the coroutine the PromiseAny::ReturnType or rethrows an exception (e.g. for the coroutine to catch).
     */
    awaiter_type operator co_await() {
      awaiter_type a(state());
      return a;
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
        auto savedPromiseAnyState = std::dynamic_pointer_cast<JSLike::PromiseAnyState>(m_state);

        // If coreturnedPromiseAny is already rejected, then also reject savedPromiseAnyState.
        if (coreturnedPromiseAny.m_state->isRejected()) {
          savedPromiseAnyState->reject(coreturnedPromiseAny.m_state->m_eptr);
          return;
        }

        // If coreturnedPromiseAny is already resolved, then resolve my state.
        if (coreturnedPromiseAny.m_state->isResolved()) {
          savedPromiseAnyState->resolve(coreturnedPromiseAny.state()->value());
          return;
        }

        // Use a "Then" Lambda to get notified if coreturnedPromiseAll gets resolved.
        coreturnedPromiseAny.Then([savedPromiseAnyState](auto result)
          {
            // coreturnedPromiseAny got resolved, so resolve savedPromiseAnyState
            savedPromiseAnyState->resolve(result);
          });

        // Use a "Catch" Lambda to get notified if savedPromiseAnyState gets rejected.
        coreturnedPromiseAny.Catch([savedPromiseAnyState](auto ex)
          {
            // coreturnedPromiseAny got rejected, so reject savedPromiseAnyState
            savedPromiseAnyState->reject(ex);
          });
      }
    };


  private:
    PromiseAny() = default;

    std::shared_ptr<PromiseAnyState> state() {
      if (!m_state) m_state = std::make_shared<PromiseAnyState>();

      auto castState = std::dynamic_pointer_cast<JSLike::PromiseAnyState>(m_state);
      return castState;
    }
  }; // PromiseAny
}; // namespace JSLike
