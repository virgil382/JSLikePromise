#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>
#include <vector>

#include "JSLikePromise.hpp"
#include "JSLikeBasePromise.hpp"

namespace JSLike {

  using namespace std;

  struct PromiseAllState : public PromiseState<vector<shared_ptr<BasePromiseState>>> {
    PromiseAllState() : m_nUnresolved(-1) {}

  private:
    friend struct PromiseAll;

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
    PromiseAllState(const PromiseAllState&) = delete;
    PromiseAllState(PromiseAllState&& other) = delete;
    PromiseAllState& operator=(const PromiseAllState&) = delete;
    PromiseAllState& operator=(PromiseAllState&&) = delete;

    void init(shared_ptr<PromiseAllState> &thisState, vector<BasePromise> const &monitoredPromises)
    {
      m_nUnresolved = monitoredPromises.size();

      if (m_nUnresolved == 0) {
        thisState->resolve(move(thisState->m_promiseStates));
        return;
      }

      for (size_t i = 0, vectorSize = monitoredPromises.size(); i < vectorSize; i++) {
        auto monitoredPromise = monitoredPromises[i];

        m_promiseStates.push_back(monitoredPromise.m_state);
        monitoredPromise.m_state->Then(
          [thisState, i](shared_ptr<BasePromiseState> resolvedState)  // Note that the Lambda keeps a shared_ptr to this PromiseAllState
          {
            thisState->m_nUnresolved--;
            thisState->m_promiseStates[i] = resolvedState;  // "bubble up" the resolved state by placing it in the array
            if (thisState->m_nUnresolved == 0) thisState->resolve(move(thisState->m_promiseStates));
          },
          [thisState](auto ex)  // Note that the Lambda keeps a shared_ptr to this PromiseAllState
          {
            thisState->reject(ex);
          });
      }
    }

    size_t                                                        m_nUnresolved;
    vector<shared_ptr<BasePromiseState>>                          m_promiseStates;
  };  // PromiseAllState


  /**
   * A PromiseAll is initialized/constructed with a vector of BasePromises (e.g. BasePromises,
   * Promise<T>s, or other PromiseAlls), which it then monitors for resolution or rejection.
   *
   * After all the BasePromises are resolved, its "Then" lambda is called with a
   * PromiseAll::ResultType (see typedef), which is vector of shared pointers to the
   * BasePromiseStates (of the BasePromises).  The Lambda can retrieve the value of each
   * (derived) BasePromiseState by calling BasePromiseState::value<T>().  Example:
   *
   * void myFunction(BasePromise &p0, BasePromise &p1, BasePromise &p2, BasePromise &p3)
   * {
   *   PromiseAll pa({ p0, p1, p2, p3 });  // These promises are constructed/resolved elsewhere.
   *   pa.Then([&](auto states)            // The type of states is PromiseAll::ResultType.
   *     {
   *       cout << states[1]->value<int>();
   *       cout << states[2]->value<string>();
   *       cout << states[3]->value<double>();
   *       // In this example we suppose that, p0 is BasePromiseState, so it doesn't have a value.
   *     });
   * }
   *
   * A PromiseAll is rejected if any of the BasePromises with which it is constructed/initialized
   * is rejected.  It's "Catch" Lambda is called with the same exception_ptr with which the
   * BasePromise was rejected.
   *
   * Coroutines can co_await on a PromiseAll via a PromiseAll::awaiter_type, which the compiler
   * automatically constructs.  After all the BasePromises are resolved, the
   * PromiseAll::awaiter_type resumes the coroutine and returns a PromiseAll::ResultType to the
   * coroutine.  However, if a BasePromise is rejected, then the PromiseAll::awaiter_type
   * resumes the coroutine and rethrows the exception_ptr with which the BasePromise was rejected.
   *
   * Coroutines can return a PromiseAll, which is "wired up" to settle at the same time and with
   * the same result as the PromiseAll resulting from co_return.
   */
  struct PromiseAll : public Promise<vector<shared_ptr<BasePromiseState>>> {
  public:
    typedef vector<shared_ptr<BasePromiseState>> ResultType;

    PromiseAll() {
      auto s = state();
      s->init(s, vector<BasePromise>{});
    }

    PromiseAll(const PromiseAll&) = default;
    PromiseAll(PromiseAll&& other) = default;
    PromiseAll& operator=(const PromiseAll&) = default;
    PromiseAll& operator=(PromiseAll&&) = default;

    /**
     * Construct a PromiseAll with the vector of BasePromises that it will monitor for resolution/rejection.
     * @param promises The vector of BasePromises.
     */
    PromiseAll(vector<BasePromise> promises)
    {
      auto s = state();
      s->init(s, promises);
    }

    PromiseAll(function<void(shared_ptr<BasePromiseState>)>) = delete;


    /**
     * Called by coroutines when they co_await on a PromiseAll.
     * @return A PromiseAll::awaiter_type that suspends/resumes the coroutine as needed, and that returns to
     *         the coroutine the PromiseAll::ReturnType or rethrows an exception (e.g. for the coroutine to catch).
     */
    awaiter_type operator co_await() {
      awaiter_type a(state());
      return a;
    }

    /**
     * A promise_type is created each time a coroutine that returns PromiseAll is called.
     * The promise_type immediately creates PromiseAll, which is returned to the caller.
     */
    struct promise_type : promise_type_base {
      /**
       * Called when invoking a coroutine that returns a PromiseAll.  This method creates
       * a PromiseAll to return to the caller and saves a shared_ptr to its PromiseAll in
       * this promise_type.
       *
       * Later, return_value() "wires up" the saved PromiseAllState so that it settles
       * when the PromiseAll evaluated by co_return settles.
       *
       * @return A pending PromiseAll that is not yet "wired up" to the PromiseAll evaluated
       *         by co_retrun.
       */
      PromiseAll get_return_object() {
        PromiseAll p(false);
        m_state = p.state();
        return p;
      }

      /**
       * Called after co_return evaluates an expression that results in a PromiseAll.  If the
       * resulting PromiseAll is settled, then this method immediately settles (i.e. resolves
       * or rejects) the PromiseAllState saved by get_return_object().  Otherwise, it adds
       * Lambdas that settle the saved PromiseAllState when the resulting PromiseAll settles.
       *
       * In essence, this method "wires up" the PromiseAll resulting from a co_return, to the
       * PromiseAll returned when the coroutine was called so that both settle in the same way.
       *
       * @param coreturnedPromiseAll The PromiseAll returned by co_return (i.e. that results from
       *        evaluating the expression following co_return).
       */
      void return_value(PromiseAll coreturnedPromiseAll) {
        auto savedPromiseAllState = dynamic_pointer_cast<PromiseAllState>(m_state);
        auto coreturnedPromiseAllState = coreturnedPromiseAll.state();

        coreturnedPromiseAllState->Then(
          [savedPromiseAllState, coreturnedPromiseAllState](shared_ptr<BasePromiseState> resultState)
          {
            savedPromiseAllState->chainResolve(coreturnedPromiseAllState->m_result);    // Pass the shared_ptr<T> down the chain to avoid copy/move.
          },
          [savedPromiseAllState](auto ex)
          {
            // coreturnedPromiseAll got rejected, so reject savedPromiseAllState
            savedPromiseAllState->reject(ex);
          });
      }
    };


  public:

    shared_ptr<PromiseAllState> state() {
      if (!m_state) m_state = make_shared<PromiseAllState>();

      auto castState = dynamic_pointer_cast<PromiseAllState>(m_state);
      return castState;
    }

  private:
    PromiseAll(bool isResolved) {
      auto s = state();
      if(isResolved)
        s->init(s, vector<BasePromise>{});
    }

  }; // PromiseAll
}; // namespace JSLike
