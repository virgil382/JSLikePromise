#include <iostream>
#include <coroutine>
#include <thread>

#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAll.hpp"
#include "../JSLikePromiseAny.hpp"

using namespace JSLike;

//================================================================================
// Unable to co_return Promise<void>
Promise<> difference01_function() {
  // The commented code is invalid because the Promise<void> class can't implement
  // return_value() due to https://eel.is/c++draft/dcl.fct.def.coroutine#6
  // which is arguably a superfluous constraint on coroutine promise types:
  //co_return Promise<>(
  //  [](auto promiseState) {
  //    promiseState->resolve();
  //  });

  // You can do this instead, which exhibits the intended behavior:
  co_await Promise<>(
    [](auto promiseState) {
      promiseState->resolve();
    });
  co_return;
}

void difference01() {
  auto p = difference01_function();
}


//================================================================================
//int main()
//{
//  difference01();
//}
