#pragma once

#include <iostream>
#include <coroutine>
#include <memory>
#include <functional>
#include <exception>

using namespace std;

namespace JSLike {

  // Forward declarations needed to declare "friendships" among structs.
  struct VoidPromise;
  struct PromiseAllState;
  struct PromiseAll;
  struct PromiseAny;

  // Forward declaration for dynamic_cast
  template<typename T> struct PromiseState;


  /**
   * A VoidPromiseState, is the classifier that maintains the state of a VoidPromise in
   * the data members described below.  It also implements the related behavior described
   * below:
   *  - The flag m_isResolved indicates whether the VoidPromise has been resolved or not.
   *  - If the VoidPromise is being used in the traditional way, which results in a
   *    call to a user-supplied "Then" Lambda when the VoidPromise is resolved, then the
   *    state also includes m_thenVoidLambda, which is a pointer to the "Then" Lambda.
   *    (Note that in the case of a VoidPromise, the "Then" Lambda takes no parameters.)
   *  - If the VoidPromise is being co_awaited on by a coroutine, then the VoidPromise
   *    state also includes the coroutine handle m_h<>, which the resolve() method uses to
   *    resume the coroutine.
   *  - To handle scenarios in which the VoidPromise is rejected, the state also includes
   *    an exception_ptr named m_eptr whose value is specified via a call to reject().
   *    If the VoidPromise is being awaited on by a coroutine (e.g. via a
   *    VoidPromise::awaiter_type), then the exception_ptr is rethrown after the
   *    coroutine resumes execution (by See VoidPromise::awaiter_type::await_resume()).
   *    This allows the coroutine to handle the exception if it wants.
   *  - But if the VoidPromise is being used in the traditional way, which results in a
   *    call to a user-supplied "Catch" Lambda when the VoidPromise is rejected, then the
   *    state also includes a pointer m_catchLambda to the "Catch" Lambda.  When the
   *    VoidPromise is rejected, the "Catch" Lambda is called, and the exception_ptr is
   *    passed to it as an argument.  The "Catch" Lambda may rethrow, catch, and handle
   *    it if it wants.
   */
  struct VoidPromiseState {
    VoidPromiseState() : m_isResolved(false) {}

    // Don't allow copies of any kind.  Always use std::shared_ptr to instances of this type.
    VoidPromiseState(const VoidPromiseState&) = delete;
    VoidPromiseState(VoidPromiseState&& other) = delete;
    VoidPromiseState& operator=(const VoidPromiseState&) = delete;

    /**
     * Resolve the VoidPromise, and resume the coroutine or execute the "Then" Lambda.
     */
    void resolve() {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_isResolved = true;

      if (m_h.address() != nullptr) {
        m_h();
      } else if (m_thenVoidLambda) {
        m_thenVoidLambda();
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
     * Get the value of a PromiseState<T>.  Use this method when you know that your VoidPromiseState is
     * a PromiseState<T>.  This allows you to store references to PromiseState<T>s as VoidPromiseStates,
     * and to access them as PromiseState<T>s.
     * 
     * @param value The value to which to PromiseState<T> was resolved.  If the PromiseState<T> was not
     *        resolvedd, then the returned value is undefined.
     */
    template<typename T>
    T value() {
      JSLike::PromiseState<T>* castState = dynamic_cast<JSLike::PromiseState<T> *>(this);
      if (!castState) {
        string err("bad cast from VoidPromiseState to PromiseState<");
        err += typeid(T).name();
        err += ">";
        throw std::bad_cast::__construct_from_string_literal(err.c_str());
      }
      return castState->value();
    }

    /**
     * Resolve a PromiseState<T>.  Use this method when you know that your VoidPromiseState is a
     * PromiseState<T>.  This allows you to store references to PromiseState<T>s as VoidPromiseStates,
     * and to operate on them as PromiseState<T>s.
     * 
     * @param value The value to which to resolve the PromiseState.
     */
    template<typename T>
    void resolve(T const &value) {
      JSLike::PromiseState<T> *castState = dynamic_cast<JSLike::PromiseState<T> *>(this);
      if (!castState) {
        string err("bad cast from VoidPromiseState to PromiseState<");
        err += typeid(T).name();
        err += ">";
        throw std::bad_cast::__construct_from_string_literal(err.c_str());
      }
      castState->resolve(value);
    }

    /**
     * Reject the VoidPromise with the specified exception_ptr.  There are two
     * posibilities for what happens next:
     *   1) If a coroutine references a VoidPromise::awaiter_type that references this
     *      VoidPromiseState, then this method resumes the coroutine, which then calls
     *      VoidPromise::awaiter_type::await_resume(), which then throws the specified
     *      exception_ptr so that the coroutine may catch/handle it.
     *   2) If a function (either regular or coroutine) references a VoidPromise that
     *      references this VoidPromiseState, then this method invokes the Lambda
     *      specified via VoidPromise::Catch() passing it the specified exception_ptr.
     *      The Lambda may then throw the exception_ptr in order to catch/handle the
     *      exception.
     *
     * @param eptr An exception_ptr that points to the exception with which to reject
     *        the VoidPromise.
     */
    void reject(std::exception_ptr eptr) {
      if (m_eptr || m_isResolved) return;  // prevent multiple rejection/resolution
      m_eptr = eptr;

      // Resume so that the calling coroutine will call the awaiter_type's await_resume(),
      // which will rethrow eptr
      if (m_h.address() != nullptr) {
        // This code executes if a coroutine references a VoidPromise::awaiter_type that
        // references this VoidPromiseState.
        // Resume the coroutine so that its call to VoidPromise::awaiter_type::await_resume()
        // will throw the specified exception.
        m_h();
      }
      else if (m_catchLambda) {
        // This code executes if a regular function (or a coroutine) references a VoidPromise
        // that references this VoidPromiseState.
        // Invoke the Lambda specified via VoidPromise::Catch(). 
        m_catchLambda(eptr);
      }
    }

    /**
     * Rethrow the exception_ptr specified when/if reject() was called.  This method is called
     * by various awaiters (e.g.VoidPromise::awaiter_type, and Promise<T>::awaiter_type) from
     * their await_resume() after the awaiting coroutine is resumed so that the exception
     * is rethrown in the context of the coroutine (e.g. so that it can handle the exception).
     */
    void rethrowIfRejected() {
      if (m_eptr)
        std::rethrow_exception(m_eptr);
    }

  protected:
    friend struct VoidPromise;
    friend struct PromiseAllState;
    friend struct PromiseAll;
    friend struct PromiseAnyState;
    friend struct PromiseAny;

    virtual void Then(std::function<void()> thenVoidLambda) {
      m_thenVoidLambda = thenVoidLambda;

      if (m_isResolved) {
        thenVoidLambda();
      }
    }

    void Catch(std::function<void(std::exception_ptr)> catchLambda) {
      m_catchLambda = catchLambda;

      if (m_eptr) {
        catchLambda(m_eptr);
      }
    }

    std::function<void()>                   m_thenVoidLambda;  // TODO: Make this a list of Lambdas

    mutable coroutine_handle<>              m_h;
    bool                                    m_isResolved;

    std::function<void(std::exception_ptr)> m_catchLambda;
    std::exception_ptr                      m_eptr;
  };




  /**
   * A VoidPromise represents the eventual completion (or failure) of an asynchronous
   * operation.  A VoidPromise does not resolve into a value, although the type Promise<T> does.
   * A VoidPromise is just a thin veneer around a VoidPromiseState.  It implements the types
   * needed to interoperate with coroutines (e.g. awiter_type and promise_type) as inner classes,
   * which are also thin veneers around the same VoidPromiseState.  This design allows the
   * VoidPromise, VoidPromise::promise_type, and VoidPromise::awaiter_type to be easily copied/moved.
   */
  struct VoidPromise {
  protected:
    VoidPromise() = default;
  public:

    VoidPromise(std::function<void(shared_ptr<VoidPromiseState>)> initializer)
    {
      initializer(state());
    }

    // The state of the VoidPromise is constructed by this VoidPromise and it is shared with a promise_type or
    // an awaiter_type, and with a "then" Lambda to resolve/reject the VoidPromise.
    std::shared_ptr<VoidPromiseState> m_state;
    std::shared_ptr<VoidPromiseState> state() {
      if(!m_state) m_state = std::make_shared<VoidPromiseState>();
      return m_state;
    }

    bool isResolved() {
      return state()->isResolved();
    }

    bool isRejected() {
      return state()->isRejected();
    }

    /**
     * VoidPromise.Then() is used by regular functions to specify a Lambda to be executed after the VoidPromise
     * is complete (i.e. the VoidPromise::promise_type::return_value() was called).  The Lambda is saved in the
     * VoidPromiseState, so it exists for as long as the VoidePromiseState exists.  However, beware of the
     * lifecycle of any variables that the Lambda may capture.
     *
     * @param thenVoidLambda A function to be called after the VoidPromise is resolved.
     * @return Another VoidPromise that will be resolved when this VoidPromise is resolved.
     */
    VoidPromise Then(std::function<void()> thenVoidLambda) {
      VoidPromise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      state()->Then([chainedPromiseState, thenVoidLambda]()
        {
          thenVoidLambda();
          chainedPromiseState->resolve();
        });

      return chainedPromise;
    }

    /**
     * VoidPromise.Catch() is used to specify a Lambda to be invoked if the VoidPromise is rejected
     * (via its PromiseState) or if an exception was thrown in the context of the coroutine that returned
     * this VoidPromise.  The Lambda is saved in the VoidPromiseState, so it exists for as long as the
     * VoidePromiseState exists.
     *
     * @param catchLambda A function to be called after the VoidPromise is rejected.
     * @return This VoidPromise (for call chaining).
     */
    VoidPromise Catch(std::function<void(std::exception_ptr)> catchLambda) {
      VoidPromise chainedPromise;
      auto chainedPromiseState = chainedPromise.state();
      state()->Catch([chainedPromiseState, catchLambda](auto ex)
        {
          catchLambda(ex);
          chainedPromiseState->reject(ex);
        });

      return chainedPromise;
    }


    /**
     * promise_type_base is the base class for promise_type implemented by various Promises derived
     * from VoidPromise.
     */
    struct promise_type_base {
      std::suspend_never initial_suspend() {
        return {};
      }

      std::suspend_never final_suspend() noexcept {
        return {};
      }

      /**
       * This method is called if an exception is thrown in the context of a coroutine, and it is
       * not handled by the coroutine.  This method essentially "captures" the exception and rejects the
       * VoidPromise returned by the coroutine.
       */
      void unhandled_exception() {
        std::exception_ptr eptr = std::current_exception();
        m_state->reject(eptr);
      }

      std::shared_ptr<VoidPromiseState> m_state;
    };

    /**
     * A promise_type is created each time a coroutine that returns VoidPromise is called.
     * The promise_type immediately creates VoidPromise, which is returned to the caller.
     */
    struct promise_type : promise_type_base {
      // Called when a coroutine is invoked.  This method returns a VoidPromise to the caller.
      VoidPromise get_return_object() {
        VoidPromise p;
        m_state = p.state();
        return p;
      }

      void return_void() {
        m_state->resolve();
      }
    };

    /**
     * awaiter_type_base is the base for awaiter_type implemented by various Promises derived
     * from VoidPromise.
     */
    struct awaiter_type_base {
      awaiter_type_base() = delete;
      awaiter_type_base(const awaiter_type_base&) = delete;
      awaiter_type_base(awaiter_type_base&& other) = default;
      awaiter_type_base& operator=(const awaiter_type_base&) = delete;

      awaiter_type_base(std::shared_ptr<VoidPromiseState> state) : m_state(state) {}

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

      std::shared_ptr<VoidPromiseState> m_state;
    };

    /**
     * An awaiter_type is constructed each time a coroutine co_awaits on a VoidPromise.
     */
    struct awaiter_type : awaiter_type_base {
      awaiter_type() = delete;
      awaiter_type(const awaiter_type&) = delete;
      awaiter_type(awaiter_type&& other) = default;
      awaiter_type& operator=(const awaiter_type&) = delete;

      awaiter_type(std::shared_ptr<VoidPromiseState> state) : awaiter_type_base(state) {}

      /**
       * Called right before the call to co_await completes in order to give the awaiter_type
       * to rethrow the exception with which the VoidPromise was rejected (via its VoidPromiseState).
       * The xecption is rethrown in the context of the coroutine (e.g. so that it may optionally
       * catch/handle the exception).
       */
      void await_resume() const {
        m_state->rethrowIfRejected();
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
    std::pair<VoidPromise, std::shared_ptr<VoidPromiseState>> static getUnresolvedPromiseAndState() {
      std::shared_ptr<VoidPromiseState> state;
      VoidPromise p([&](auto promiseState)
        {
          state = promiseState;
        });

      return { std::move(p), state };
    }
  };

};
