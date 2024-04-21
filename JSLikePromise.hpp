#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>
#include <utility>  // std::forward

#include "JSLikeBasePromise.hpp"

namespace JSLike {

  using namespace std;

  // Forward declarations needed to declare "friendships" among structs.
  template<typename T> struct Promise;
  struct PromiseAll;
  struct PromiseAny;
  struct PromiseAnyState;

  /**
   * PromiseState<T> represents the state of a Promise that can be resolved to a value of
   * type T.  In addition to BaseState, a PromiseState<T> contains a shared_ptr<T> that
   * == nullptr until the Promise is resolved.  When it is, all the Promises down the
   * chain are chain-resolved with a copy of this shared_ptr<T>.  This design choice results
   * in an efficient implementation that interoperates with result types that cannot be copied
   * (e.g. asio::ip::tcp::socket).
   */
  template<typename T=void>
  struct PromiseState : public BasePromiseState {
    typedef shared_ptr<PromiseState<T>> ptr;
    typedef function<void(T&)> ThenCallback;

    PromiseState() = default;

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
    PromiseState(const PromiseState&) = delete;
    PromiseState(PromiseState&& other) = delete;
    PromiseState& operator=(const PromiseState&) = delete;
    PromiseState& operator=(PromiseState&&) = delete;

    /**
     * Resolve the Promise with an lvalue reference to the result.  This PromiseState
     * makes a copy of the specified value, and maintains a reference to it via a shared_ptr.
     * 
     * @param result The result that should be copied.
     */
    void resolve(T& result) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_result = make_shared<T>(result);      // copy
      BasePromiseState::resolve(shared_from_this());
    }

    /**
     * Resolve the Promise with an rvalue reference to the result.  This variant of resolve()
     * supports move semantics.  If possible, it move-constructs the result referenced by the
     * shared_ptr.  Otherwise it copy-constructs it.
     * 
     * @param result The result that should be moved (if possible), otherwise copied.
     */
    void resolve(T&& result) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_result = make_shared<T>(forward<T>(result));      // move
      BasePromiseState::resolve(shared_from_this());
    }

    /**
     * Get a reference to the result.
     * 
     * @return an lvalue reference to the result.
     */
    T &value() {
      return *m_result;
    }

  private:
    /**
     * A variant of resolve() that copies the specified reference to the result.
     * 
     * @param result A shared_ptr to the result.  This PromiseState copies the
     *        shared_ptr and saves it.
     */
    void chainResolve(shared_ptr<T> result) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_result = result;
      BasePromiseState::resolve(shared_from_this());
    }

  private:
    friend struct Promise<T>;
    friend struct PromiseAll;
    friend struct PromiseAny;
    friend struct PromiseAnyState;

    // shared_ptr<T> offers the following benefits (over containing a T):
    //   1) resolve() constructs m_result, so there is no need for T to have a default constructor.
    //   2) The shared_ptr<T> can be passed down the chain of PromiseStates without copying/moving T.
    shared_ptr<T>                                      m_result;
  };

  /******************************************************************************************/

  template<>
  struct PromiseState<void> : public BasePromiseState {
    typedef function<void()> ThenCallback;

    PromiseState() : BasePromiseState() {}

    // Don't allow copies of any kind.  Always use shared_ptr to instances of this type.
    PromiseState(const PromiseState&) = delete;
    PromiseState(PromiseState&& other) = delete;
    PromiseState& operator=(const PromiseState&) = delete;
    PromiseState& operator=(PromiseState&&) = delete;
  };

  /******************************************************************************************/

  /**
   * A Promise<T> a.k.a. a valued Promise, is a communication channel between a Producer
   * (usually an asynchronous operation a.k.a. task) and a Consumer.  The channel may be used
   * exactly once by the Producer to send to the Consumer either:
   *  - A notification that an event occurred (usually accompanied by a value a.k.a. the
   *    result).  This occurs when the Consumer resolves the Promise; or
   *  - A notification of an exception (e.g. an error detected by the Producer) along with the
   *    exception object.  This occurs when the Producer rejects the Promise.
   *
   * A Promise<T> can be used as a "thenable", or it can be used in conjunction with coroutines
   * via the promise_type and the awaiter_type.  (See the comments associated with these
   * inner classes for additional details.)
   * 
   * If used as a "thenable", the consumer of the result registers callbacks by calling Then()
   * and/or Catch().  These callbacks get called to handle the resolve/reject events.  These
   * methods return a new "chained" Promise whose Then() and/or Catch() methods can be called
   * to register an additional set of callbacks, and so on.  The chained Promise gets
   * resolved/rejected after the initial Promise gets resolved/rejected.
   * 
   * When consturcting a Promise the caller specifies a TaskInitializer callback, which takes
   * as parameter a shared_ptr<PromiseState>.  Typically, the TaskInitializer starts an async
   * operation a.k.a. task, which keeps a copy of the shared_ptr<PromiseState> so it can later
   * call the resolve() method with the result (e.g. after the task completes and the result
   * is available).
   * 
   * A Promise is just a "thin veneer" to the PromiseState.  A Promise maintains a
   * reference to the PromiseState via a shared_ptr<PromiseState>.  Thus, Promises can be
   * copied with little overhead if needed.  But be aware that each copy increments the
   * reference count.
   */
  template<typename T=void>
  struct Promise : public BasePromise {
  protected:
    Promise() = default;

  public:
    typedef function<void(shared_ptr<PromiseState<T>>)> TaskInitializer;
    typedef PromiseState<T>::ThenCallback ThenCallback;

    /**
     * Construct a Promise and initialize an async operation a.k.a. task.
     * 
     * @param initialize A TaskInitializer that gets called immediately with a shared_ptr<PromiseState<T>>
     *        that may be passed along to the task.
     */
    Promise(TaskInitializer initializer)
    {
      auto s = state();
      try {
        initializer(s);
      }
      catch (...) {
        s->reject(current_exception());
      }
    }

    /**
     * Construct a Promise<T> resolve it with the specified value.  The value will
     * be copied.
     * 
     * @param result The value with which to resolve the Promise<T>.
     */
    Promise(T & result)
    {
      state()->chainResolve(make_shared<T>(result));
    }

    /**
     * Construct a Promise<T> resolved with the specified value.  If the value can be
     * moved, then it will be.
     *
     * @param result The value with which to resolve the Promise<T>.
     */
    Promise(T && result)
    {
      state()->chainResolve(make_shared<T>(forward<T>(result)));
    }

    /**
     * Construct a Promise<T> resolve it with the specified value.  The value will
     * be copied.
     *
     * @param result The value with which to resolve the Promise<T>.
     * @return The resolved Promise<T>.
     */
    static Promise resolve(T & val) {
      return Promise(val);
    }

    /**
     * Construct a Promise<T> resolved with the specified value.  If the value can be
     * moved, then it will be.
     *
     * @param result The value with which to resolve the Promise<T>.
     * @return The resolved Promise<T>.
     */
    static Promise resolve(T&& val) {
      return Promise(forward<T>(val));
    }

    /**
     * If necessary, construct a PromiseState<T>, save it, and return a reference to it.  The
     * PromiseState<T> may be shared with:
     *   - the TaskInitializer callback
     *   - copies of this Promise<T>
     *   - Promise<T>::promise_type e.g. via which coroutines calling co_return resolve the Promise<T>
     *   - Promise<T>::awaiter_type e.g. so that coroutines calling co_await will suspend execution
     *     if the Promise<T> is neither resolved nor rejected, and will resume execution if/when the
     *     Promise<T> is resolved/rejected
     */
    shared_ptr<PromiseState<T>> state() {
      if (!m_state) m_state = make_shared<PromiseState<T>>();

      auto castState = dynamic_pointer_cast<JSLike::PromiseState<T>>(m_state);
      return castState;
    }

    /**
     * Get the result of the Promise.  Try to avoid using this method.  If you use it, then consider
     * changing your design.
     * 
     * @return A reference to the Promise result.
     */
    T &value() {
      return state()->value();
    }

    /**
     * Then() is used by normal routines (i.e. not coroutines) to specify a ThenCallback to be
     * invoked after the Promise<T> is resolved.  This variant of "Then" expects a ThenCallback
     * whose parameter is a regular reference to the result.
     * 
     * @param thenCallback A function to be called after the Promise<T> is resolved.
     * @return Another Promise<T> "chained" to this one that will be resolved/rejected when this
     *         Promise<T> is resolved.
     */
    Promise Then(ThenCallback thenCallback) {
      Promise chainedPromise;
      shared_ptr<PromiseState<T>> chainedPromiseState = chainedPromise.state();

      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState, thenCallback](shared_ptr<BasePromiseState> resultState)
        {
          auto castState = dynamic_pointer_cast<PromiseState<T>>(resultState);

          thenCallback(castState->value());
          chainedPromiseState->chainResolve(castState->m_result);  // Pass the shared_ptr<T> down the chain to avoid copy/move.
        },
        [chainedPromiseState](auto ex)
        {
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * Catch() is used by normal routines (i.e. not coroutines) to specify a CatchCallback to
     * be invoked after the Promise<T> is rejected.
     *
     * @param catchCallback A function to be called after the Promise<T> is rejected.  The
     *        function's parameter is an exception_ptr to the exception with which the Promise<T>
     *        was rejected.
     * @return Another Promise<T> "chained" to this one that will be resolved/rejected when this
     *         Promise<T> is resolved.
     */
    Promise Catch(CatchCallback catchCallback) {
      Promise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState](shared_ptr<BasePromiseState> resultState)
        {
          auto castState = dynamic_pointer_cast<PromiseState<T>>(resultState);
          chainedPromiseState->chainResolve(castState->m_result);  // Pass the shared_ptr<T> down the chain to avoid copy/move.
        },
        [chainedPromiseState, catchCallback](auto ex)
        {
          catchCallback(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * Same as calling Then() and Catch() except that both callbacks are set in the PromiseState<T>
     * of this Promise<T>, and a single chained Promise<T> is created and returned.
     * 
     * @param thenCallback A function to be called after the Promise<T> is resolved.
     * @param catchCallback A function to be called after the Promise<T> is rejected.  The function's
     *        parameter is an exception_ptr to the exception with which the Promise<T> was rejected.
     * @return Another Promise<T> "chained" to this one that will be resolved/rejected when this
     *         Promise<T> is resolved/rejected.
     */
    Promise Then(ThenCallback thenCallback, CatchCallback catchCallback) {
      Promise chainedPromise;
      shared_ptr<PromiseState<T>> chainedPromiseState = chainedPromise.state();

      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState, thenCallback](shared_ptr<BasePromiseState> resultState)
        {
          auto castState = dynamic_pointer_cast<PromiseState<T>>(resultState);
          thenCallback(castState->value());
          chainedPromiseState->chainResolve(castState->m_result);  // Pass the shared_ptr<T> down the chain to avoid copy/move.
        },
        [chainedPromiseState, catchCallback](auto ex)
        {
          catchCallback(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * A promise_type is created each time a coroutine that returns Promise<T> is called.
     * The promise_type creates the Promise<T> and keeps a reference to the PromiseState<T>.
     * Subsequently, the promise_type acts as an adapter that allows the coroutine to interact
     * with the PromiseState<T> e.g. see return_value() variants.
     *
     * A coroutine interacts with a promise_type when the coroutine
     *   - calls co_return, in which case the promise_type resolves the PromiseState<T>; or
     *   - does not handle an exception, in which case the promise_type rejects the
     *     PromiseState<T>
     */
    struct promise_type : promise_type_base {
      // Called when a coroutine is invoked.  This method returns a Promise<T> to the caller.
      Promise get_return_object() {
        Promise p;
        m_state = p.state();
        return p;
      }

      /**
       * Called when a coroutine that returns a Promise<T> calls co_return with
       * an lvalue reference to a T in order to resolve the Promise<T> with a
       * copy of the specified value.
       * 
       * @param val The lvalue reference to the value that should be copied and
       *        stored as the result of the Promise<T>.
       */
      void return_value(const T& val) {
        auto castState = dynamic_pointer_cast<PromiseState<T>>(m_state);
        castState->resolve(const_cast<T&>(val));
      }

      /**
       * Called when a coroutine that returns a Promise<T> calls co_return with
       * an rvalue reference to a T in order to resolve the Promise<T> by moving
       * (if possible) the specified value into the result of the Promise<T>.
       * If moving is not possible, then the value is copied.
       *
       * @param val The rvalue reference to the value that should be moved and
       *        stored as the result of the Promise<T>.
       */
      void return_value(T &&val) {
        auto castState = dynamic_pointer_cast<PromiseState<T>>(m_state);
        castState->resolve(forward<T>(val));
      }

      /**
       * Called when a coroutine calls co_return with Promise<T>.  Unlike the other
       * variants of return_value(), this variant does not resolve the Promise<T>
       * returned by the coroutine.  Instead, it chains the Promise<T> returned by
       * the coroutine to the specified Promise<T>.
       * 
       * @param coreturnedPromise The Promise<T> to which the Promise<T> returned by
       *        the coroutine should be chained.
       */
      void return_value(Promise coreturnedPromise) {
        auto savedPromiseState = dynamic_pointer_cast<PromiseState<T>>(m_state);
        auto coreturnedPromiseState = coreturnedPromise.state();
        weak_ptr<PromiseState<T>> weakCoreturnedPromiseState(coreturnedPromiseState);

        coreturnedPromiseState->Then(
          [savedPromiseState, weakCoreturnedPromiseState](shared_ptr<BasePromiseState> resultState)
          {
            auto coreturnedPromiseState = weakCoreturnedPromiseState.lock();
            if (!coreturnedPromiseState)
              return;  // This should never happen

            // coreturnedPromise got resolved, so resolve savedPromiseState
            savedPromiseState->chainResolve(coreturnedPromiseState->m_result);  // Pass the shared_ptr<T> down the chain to avoid copy/move.
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
     * The awaiter_type gets a reference to the PromiseState of the Promise<T> when
     * it is constructed (see Promise<T>::co_await()).  The awaiter_type then
     * interacts with the PromiseState through this reference (e.g. to determine if
     * execution should be suspended/resumed, to return a value to the caller of
     * co_return).
     */
    struct awaiter_type : awaiter_type_base {
      awaiter_type() = delete;
      awaiter_type(const awaiter_type&) = delete;
      awaiter_type(awaiter_type&& other) = default;
      awaiter_type& operator=(const awaiter_type&) = delete;

      awaiter_type(shared_ptr<BasePromiseState> state) : awaiter_type_base(state) {}

       /**
       * Called right before the call to co_await completes.  If the Promise<T> on which the
       * coroutine is co_awaiting was rejected, then this method rethrows the exception
       * with which the Promise<T> was rejected.  Consequently, the coroutine may catch/handle
       * it.  Otherwise, if the Promise<T> was resolved, then this method returns the result
       * to the coroutine.
       *
       * @return An rvalue reference to the value with which resolve() was called.  It is
       *         up to the coroutine to move the value if appropriate.
       */
      T &await_resume() {
        m_state->rethrowIfRejected();
        T& ref = m_state->value<T>();
        return ref;
      }
    };

    /**
     * Called when a coroutine calls co_await on this Promise<T> to obtain an
     * awaiter_type through which the coroutine collaborates with the PromiseState<T>
     * of this Promise<T> e.g. to decide if the coroutine should suspend/resume execution,
     * to return the result of the Promise<T> after some other code resolves it, etc.
     * 
     * @return a new promise_type that has a reference to the PromiseState<T> of this
     * Promise<T> i.e. the one being co_awaited.
     */
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





  /**
   * A void Promise or Promise<void> or Promise<> is essentially a Promise<T> without a result.
   * It can be used to communicate an event (i.e. that the Promise<> was resolved) from a
   * producer to a consumer.
   */
  template<>
  struct Promise<void> : BasePromise {
  protected:
    Promise(bool isResolved) {
      if(isResolved)
        state()->resolve();
    }
  public:
    typedef function<void(shared_ptr<PromiseState<>>)> TaskInitializer;
    typedef PromiseState<>::ThenCallback               ThenCallback;

    /**
     * Construct a Promise<> and initialize an async operation a.k.a. task.
     *
     * @param initialize A TaskInitializer that gets called immediately with a shared_ptr<PromiseState<>>
     *        that may be passed along to the task.
     */
    Promise(TaskInitializer initializer)
    {
      auto s = state();
      try {
        initializer(s);
      }
      catch (...) {
        s->reject(current_exception());
      }
    }

    /**
     * Construct a pre-resolved Promise<>.
     */
    Promise() {
      state()->resolve();
    }

    /**
     * Construct a pre-resolved Promise<> and return it.
     * @return The resolved Promise<>.
     */
    static Promise resolve() {
      return Promise();
    }

    /**
     * If necessary, construct a PromiseState<>, save it, and return a reference to it.  The
     * PromiseState<> may be shared with:
     *   - the TaskInitializer callback
     *   - copies of this Promise<>
     *   - Promise<>::promise_type e.g. via which coroutines calling co_return resolve the Promise<>
     *   - Promise<>::awaiter_type e.g. so that coroutines calling co_await will suspend execution
     *     if the Promise<> is neither resolved nor rejected, and will resume execution if/when the
     *     Promise<> is resolved/rejected
     */
    shared_ptr<PromiseState<>> state() {
      if (!m_state) m_state = make_shared<PromiseState<>>();

      auto castState = dynamic_pointer_cast<JSLike::PromiseState<>>(m_state);
      return castState;
    }

    /**
     * Then() is used by normal routines (i.e. not coroutines) to specify a ThenCallback to be
     * invoked after the Promise<> is resolved.  This variant of "Then" expects a ThenCallback that
     * does not have any parameters.
     *
     * @param thenCallback A function to be called after the Promise<> is resolved.
     * @return Another Promise<> "chained" to this one that will be resolved/rejected when this
     *         Promise<> is resolved.
     */
    Promise Then(ThenCallback thenCallback) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();

      shared_ptr<PromiseState<>> thisPromiseState = state();
      thisPromiseState->Then([chainedPromiseState, thenCallback](shared_ptr<BasePromiseState> resultState)
        {
          thenCallback();
          chainedPromiseState->resolve();
        },
        [chainedPromiseState](auto ex)
        {
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * Catch() is used by normal routines (i.e. not coroutines) to specify a CatchCallback to
     * be invoked after the Promise<> is rejected.
     *
     * @param catchCallback A function to be called after the Promise<> is rejected.  The
     *        function's parameter is an exception_ptr to the exception with which the Promise<>
     *        was rejected.
     * @return Another Promise<> "chained" to this one that will be resolved/rejected when this
     *         Promise<> is resolved.
     */
    Promise Catch(CatchCallback catchCallback) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();
      auto thisPromiseState = state();
      thisPromiseState->Then(
        [chainedPromiseState](shared_ptr<BasePromiseState> resultState)
        {
          chainedPromiseState->resolve();
        },
        [chainedPromiseState, catchCallback](auto ex)
        {
          catchCallback(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * Same as calling Then() and Catch() except that both callbacks are set in the PromiseState<>
     * of this Promise<>, and a single chained Promise<> is created and returned.
     *
     * @param thenCallback A function to be called after the Promise<> is resolved.
     * @param catchCallback A function to be called after the Promise<> is rejected.  The function's
     *        parameter is an exception_ptr to the exception with which the Promise<> was rejected.
     * @return Another Promise<> "chained" to this one that will be resolved/rejected when this
     *         Promise<> is resolved/rejected.
     */
    Promise Then(ThenCallback thenCallback, CatchCallback catchCallback) {
      Promise chainedPromise(false);
      auto chainedPromiseState = chainedPromise.state();

      shared_ptr<PromiseState<>> thisPromiseState = state();
      thisPromiseState->Then([chainedPromiseState, thenCallback](shared_ptr<BasePromiseState> resultState)
        {
          thenCallback();
          chainedPromiseState->resolve();
        },
        [chainedPromiseState, catchCallback](auto ex)
        {
          catchCallback(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }


    /**
     * A promise_type is created each time a coroutine that returns Promise<> is called.
     * The promise_type creates the Promise<> and keeps a reference to the PromiseState<>.
     * Subsequently, the promise_type acts as an adapter that allows the coroutine to interact
     * with the PromiseState<> e.g. see return_void().
     *
     * A coroutine interacts with a promise_type when the coroutine
     *   - calls co_return, in which case the promise_type resolves the PromiseState<>; or
     *   - does not handle an exception, in which case the promise_type rejects the
     *     PromiseState<>
     */
    struct promise_type : promise_type_base {
      // Called when a coroutine is invoked.  This method returns a BasePromise to the caller.
      Promise get_return_object() {
        Promise p(false);
        m_state = p.state();
        return p;
      }

      /**
       * Called when a coroutine that returns a Promise<> calls co_return, which resolves the
       * Promise<> returned by the coroutine.
       */
      void return_void() {
        m_state->resolve();
      }
    };

    /**
     * An awaiter_type is constructed each time a coroutine co_awaits on a Promise<>.
     * During construction, the awaiter_type gets a reference to the PromiseState<> of the
     * Promise<> on which it is co_awaiting (see Promise<>::co_await()).  The awaiter_type then
     * interacts with the PromiseState<> through this reference (e.g. to determine if
     * execution should be suspended/resumed).
     */
    struct awaiter_type : awaiter_type_base {
      awaiter_type() = delete;
      awaiter_type(const awaiter_type&) = delete;
      awaiter_type(awaiter_type&& other) = default;
      awaiter_type& operator=(const awaiter_type&) = delete;

      awaiter_type(shared_ptr<PromiseState<>> state) : awaiter_type_base(state) {}

      /**
       * Called right before the call to co_await completes.  If the Promise<> on which the
       * coroutine is co_awaiting was rejected, then this method rethrows the exception
       * with which the Promise<> was rejected.  Consequently, the coroutine may catch/handle
       * it.
       */
      void await_resume() const {
        m_state->rethrowIfRejected();
      }
    };

    /**
     * Called when a coroutine calls co_await on this Promise<> to obtain an
     * awaiter_type through which the coroutine collaborates with the PromiseState<>
     * of this Promise<> e.g. to decide if the coroutine should suspend/resume execution.
     *
     * @return a new promise_type that has a reference to the PromiseState<> of this
     * Promise<> i.e. the one being co_awaited.
     */
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
