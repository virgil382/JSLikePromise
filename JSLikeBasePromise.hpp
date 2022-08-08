#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>

using namespace std;

namespace JSLike {

  // Forward declarations needed to declare "friendships" among structs.
  struct BasePromise;
  struct PromiseAllState;
  struct PromiseAll;
  struct PromiseAny;
  template<typename T> struct Promise;

  // Forward declaration for dynamic_cast
  template<typename T> struct PromiseState;



  /**
   * A BasePromiseState, is the classifier that maintains the state of a BasePromise in
   * the data members described below.  It also implements the related behavior described
   * below:
   *  - The flag m_isResolved indicates whether the BasePromise has been resolved or not.
   *  - If the BasePromise is being used in the traditional way, which results in a
   *    call to a user-supplied "Then" Lambda when the BasePromise is resolved, then the
   *    state also includes m_thenVoidLambda, which is a pointer to the "Then" Lambda.
   *    (Note that in the case of a BasePromise, the "Then" Lambda takes no parameters.)
   *  - If the BasePromise is being co_awaited on by a coroutine, then the BasePromise
   *    state also includes the coroutine handle m_h<>, which the resolve() method uses to
   *    resume the coroutine.
   *  - To handle scenarios in which the BasePromise is rejected, the state also includes
   *    an exception_ptr named m_eptr whose value is specified via a call to reject().
   *    If the BasePromise is being awaited on by a coroutine (e.g. via a
   *    BasePromise::awaiter_type), then the exception_ptr is rethrown after the
   *    coroutine resumes execution (by See BasePromise::awaiter_type::await_resume()).
   *    This allows the coroutine to handle the exception if it wants.
   *  - But if the BasePromise is being used in the traditional way, which results in a
   *    call to a user-supplied "Catch" Lambda when the BasePromise is rejected, then the
   *    state also includes a pointer m_catchLambda to the "Catch" Lambda.  When the
   *    BasePromise is rejected, the "Catch" Lambda is called, and the exception_ptr is
   *    passed to it as an argument.  The "Catch" Lambda may rethrow, catch, and handle
   *    it if it wants.
   */
  struct BasePromiseState : public enable_shared_from_this<BasePromiseState> {
    BasePromiseState() : m_isResolved(false) {}

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
    BasePromiseState(const BasePromiseState&) = delete;
    BasePromiseState(BasePromiseState&& other) = delete;
    BasePromiseState& operator=(const BasePromiseState&) = delete;

    virtual ~BasePromiseState() {}  // Force BasePromiseState to be polymorphic to support dynamic_cast<>.

    /**
     * Resolve the BasePromise, and resume the coroutine or execute the "Then" Lambda.
     */
    void resolve() {
      resolve(shared_from_this());
    }

    void resolve(shared_ptr<BasePromiseState> state) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_isResolved = true;

      if (m_h.address() != nullptr) {
        m_h();
      }
      else if (m_thenLambda) {
        m_thenLambda(state);
      }
    }

    bool isResolved() {
      return m_isResolved;
    }

    bool isRejected() {
      return m_eptr != nullptr;
    }

    template<typename T>
    bool isValueOfType() {
      JSLike::PromiseState<T>* castState = dynamic_cast<JSLike::PromiseState<T> *>(this);
      return (castState != nullptr);
    }

    /**
     * Get the value of a PromiseState<T>.  Use this method when you know that your BasePromiseState is
     * a PromiseState<T>.  This allows you to store references to PromiseState<T>s as BasePromiseStates,
     * and to access them as PromiseState<T>s.
     * 
     * @param value The value to which to PromiseState<T> was resolved.  If the PromiseState<T> was not
     *        resolvedd, then the returned value is undefined.
     */
    template<typename T>
    T &value() {
      JSLike::PromiseState<T>* castState = dynamic_cast<JSLike::PromiseState<T> *>(this);
      if (!castState) {
        string err("bad cast from BasePromiseState to PromiseState<");
        err += typeid(T).name();
        err += ">";
        throw bad_cast::__construct_from_string_literal(err.c_str());
      }
      return castState->value();
    }

    /**
     * Resolve a PromiseState<T>.  Use this method when you know that your BasePromiseState is a
     * PromiseState<T>.  This allows you to store references to PromiseState<T>s as BasePromiseStates,
     * and to operate on them as PromiseState<T>s.
     *
     * @param value The value to which to resolve the PromiseState.
     */
    template<typename T>
    void resolve(T const& value) {
      JSLike::PromiseState<T>* castState = dynamic_cast<JSLike::PromiseState<T> *>(this);
      if (!castState) {
        string err("bad cast from BasePromiseState to PromiseState<");
        err += typeid(T).name();
        err += ">";
        throw bad_cast::__construct_from_string_literal(err.c_str());
      }
      castState->resolve(value);
    }

    /**
     * Reject the BasePromise with the specified exception_ptr.  There are two
     * posibilities for what happens next:
     *   1) If a coroutine references a BasePromise::awaiter_type that references this
     *      BasePromiseState, then this method resumes the coroutine, which then calls
     *      BasePromise::awaiter_type::await_resume(), which then throws the specified
     *      exception_ptr so that the coroutine may catch/handle it.
     *   2) If a function (either regular or coroutine) references a BasePromise that
     *      references this BasePromiseState, then this method invokes the Lambda
     *      specified via BasePromise::Catch() passing it the specified exception_ptr.
     *      The Lambda may then throw the exception_ptr in order to catch/handle the
     *      exception.
     *
     * @param eptr An exception_ptr that points to the exception with which to reject
     *        the BasePromise.
     */
    void reject(exception_ptr eptr) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_eptr = eptr;

      // Resume so that the calling coroutine will call the awaiter_type's await_resume(),
      // which will rethrow eptr
      if (m_h.address() != nullptr) {
        // This code executes if a coroutine references a BasePromise::awaiter_type that
        // references this BasePromiseState.
        // Resume the coroutine so that its call to BasePromise::awaiter_type::await_resume()
        // will throw the specified exception.
        m_h();
      }
      else if (m_catchLambda) {
        // This code executes if a regular function (or a coroutine) references a BasePromise
        // that references this BasePromiseState.
        // Invoke the Lambda specified via BasePromise::Catch(). 
        m_catchLambda(eptr);
      }
    }

    /**
     * Rethrow the exception_ptr specified when/if reject() was called.  This method is called
     * by various awaiters (e.g.BasePromise::awaiter_type, and Promise<T>::awaiter_type) from
     * their await_resume() after the awaiting coroutine is resumed so that the exception
     * is rethrown in the context of the coroutine (e.g. so that it can handle the exception).
     */
    void rethrowIfRejected() {
      if (m_eptr)
        rethrow_exception(m_eptr);
    }

  protected:
    friend struct BasePromise;
    friend struct Promise<void>;
    friend struct PromiseAllState;
    friend struct PromiseAll;
    friend struct PromiseAnyState;
    friend struct PromiseAny;

    virtual void Then(function<void(shared_ptr<BasePromiseState>)> thenLambda) {
      m_thenLambda = thenLambda;

      if (m_isResolved) {
        thenLambda(shared_from_this());
      }
    }

    void Catch(function<void(exception_ptr)> catchLambda) {
      m_catchLambda = catchLambda;

      if (m_eptr) {
        catchLambda(m_eptr);
      }
    }

    // Why beat around the bush.  Do both at the same time.
    void Then(function<void(shared_ptr<BasePromiseState>)> thenLambda, function<void(exception_ptr)> catchLambda) {
      Then(thenLambda);
      Catch(catchLambda);
    }

    function<void(shared_ptr<BasePromiseState>)>    m_thenLambda;

    mutable coroutine_handle<>                           m_h;
    bool                                                 m_isResolved;

    function<void(exception_ptr)>              m_catchLambda;
    exception_ptr                                   m_eptr;
  };




  /**
   * A BasePromise represents the eventual completion (or failure) of an asynchronous
   * operation.  A BasePromise does not resolve into a value, although the type Promise<T> does.
   * A BasePromise is just a thin veneer around a BasePromiseState.  It implements the types
   * needed to interoperate with coroutines (e.g. awiter_type and promise_type) as inner classes,
   * which are also thin veneers around the same BasePromiseState.  This design allows the
   * BasePromise, BasePromise::promise_type, and BasePromise::awaiter_type to be easily copied/moved.
   */
  struct BasePromise {
  protected:
    BasePromise() = default;
  public:

    BasePromise(function<void(shared_ptr<BasePromiseState>)> initializer)
    {
      initializer(state());
    }

    // The state of the BasePromise is constructed by this BasePromise and it is shared with a promise_type or
    // an awaiter_type, and with a "then" Lambda to resolve/reject the BasePromise.
    shared_ptr<BasePromiseState> m_state;
    shared_ptr<BasePromiseState> state() {
      if(!m_state) m_state = make_shared<BasePromiseState>();
      return m_state;
    }

    bool isResolved() {
      return state()->isResolved();
    }

    bool isRejected() {
      return state()->isRejected();
    }

    BasePromise Then(function<void(shared_ptr<BasePromiseState>)> thenLambda) {
      BasePromise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      state()->Then([chainedPromiseState, thenLambda](auto resolvedState)
        {
          thenLambda(resolvedState);
          chainedPromiseState->resolve(resolvedState);
        });

      return chainedPromise;
    }

    /**
     * BasePromise.Catch() is used to specify a Lambda to be invoked if the BasePromise is rejected
     * (via its PromiseState) or if an exception was thrown in the context of the coroutine that returned
     * this BasePromise.  The Lambda is saved in the BasePromiseState, so it exists for as long as the
     * VoidePromiseState exists.
     *
     * @param catchLambda A function to be called after the BasePromise is rejected.
     * @return This BasePromise (for call chaining).
     */
    BasePromise Catch(function<void(exception_ptr)> catchLambda) {
      BasePromise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      state()->Catch([chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    BasePromise Then(function<void(shared_ptr<BasePromiseState>)> thenLambda, function<void(exception_ptr)> catchLambda) {
      BasePromise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      state()->Then([chainedPromiseState, thenLambda](auto resolvedState)
        {
          thenLambda(resolvedState);
          chainedPromiseState->resolve(resolvedState);
        },
        [chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }

    /**
     * promise_type_base is the base class for promise_type implemented by various Promises derived
     * from BasePromise.
     */
    struct promise_type_base {
      suspend_never initial_suspend() {
        return {};
      }

      suspend_never final_suspend() noexcept {
        return {};
      }

      /**
       * This method is called if an exception is thrown in the context of a coroutine, and it is
       * not handled by the coroutine.  This method essentially "captures" the exception and rejects the
       * BasePromise returned by the coroutine.
       */
      void unhandled_exception() {
        exception_ptr eptr = current_exception();
        m_state->reject(eptr);
      }

      shared_ptr<BasePromiseState> m_state;
    };


    /**
     * awaiter_type_base is the base for awaiter_type implemented by various Promises derived
     * from BasePromise.
     */
    struct awaiter_type_base {
      awaiter_type_base() = delete;
      awaiter_type_base(const awaiter_type_base&) = delete;
      awaiter_type_base(awaiter_type_base&& other) = default;
      awaiter_type_base& operator=(const awaiter_type_base&) = delete;

      awaiter_type_base(shared_ptr<BasePromiseState> state) : m_state(state) {}

      /**
       * Answer the quest of whether or not the coroutin calling co_await should suspend.
       * This method is called as an optimization in case the coroutine execution should NOT
       * be suspended.
       *
       * @return True if execution should NOT be suspended, otherwise false.
       */
      bool await_ready() const noexcept {
        return m_state->m_isResolved;
      }

      /**
       * This method is called immediately after the coroutine is suspended (after it called
       * co_await).  After this method returns, control is transfered back to the caller/resumer
       * of the coroutine.
       *
       * NOTE that we could safely start an async read operation in here.
       *
       * @param coroutine_handle The coroutine_handle that may be used to resume execution
       *        of the coroutine.
       */
      void await_suspend(coroutine_handle<> h) const noexcept {
        m_state->m_h = h;
      }

      shared_ptr<BasePromiseState> m_state;
    };
  };

};
