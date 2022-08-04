#include <iostream>
#include <coroutine>
#include <thread>

#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAll.hpp"
#include "../JSLikePromiseAny.hpp"

using namespace JSLike;

//================================================================================
// Function returns pre-resolved Promise.
// Caller with "then".
Promise<int> ex01_function() {
  return 1;   //  <= cool stuff occurs here
}

void example01() {
  ex01_function().Then([](int result)
    {
      std::cout << "ex01: result=" << result << "\n";
    });
}

//================================================================================
// Function returns initializer-resolved Promise.
// Caller with "then".
Promise<int> ex02_function() {
  return Promise<int>([](shared_ptr<PromiseState<int>> promiseState)
    {
      promiseState->resolve(2);
    });
}

void example02() {
  ex02_function().Then([](int result)
    {
      std::cout << "ex02: result=" << result << "\n";
    });
}

//================================================================================
// Async/coroutine returns initializer-resolved Promise.
// Caller with "then".
Promise<int> ex02a_function() {
  co_return co_await Promise<int>([](shared_ptr<PromiseState<int>> promiseState)
    {
      promiseState->resolve(2);
    });
}

void example02a() {
  ex02a_function().Then([](int result)
    {
      std::cout << "ex02a: result=" << result << "\n";
    });
}

//================================================================================
// Async/coroutine returns a implicitly-resolved Promise.
// Caller with "then".
Promise<int> ex03_coroutine() {
  co_return 3;
}

void example03() {
  ex03_coroutine().Then([](int result)
    {
      std::cout << "ex03: result=" << result << "\n";
    });
}

//================================================================================
// Async/Coroutine awaits on a Promise before resuming.
shared_ptr<PromiseState<int>> example04_promiseState;

Promise<int> ex04_resolveAfter1Sec() {
  return Promise<int>([](shared_ptr<PromiseState<int>> promiseState)
    {
      example04_promiseState = promiseState;
    });
}


Promise<int> ex04_coroutine() {
  int result = co_await ex04_resolveAfter1Sec();
  co_return result;
}

void example04() {
  ex04_coroutine().Then([](int result)
    {
      std::cout << "ex04: result after 1sec=" << result << "\n";
    });

  // Wait 1sec before resolving the Promise to the value 4.
  std::this_thread::sleep_for(1000ms);
  example04_promiseState->resolve(4);
}

//================================================================================
// Async/Coroutine awaits on another async/coroutine that get resolved after 1sec.
shared_ptr<PromiseState<int>> example05_promiseState;

Promise<int> ex5_resolveAfter1Sec() {
  return Promise<int>([](shared_ptr<PromiseState<int>> promiseState)
    {
      example05_promiseState = promiseState;
    });
}

Promise<int> ex5_coroutine1() {
  int result = co_await ex5_resolveAfter1Sec();
  co_return result;
}

Promise<int> ex5_coroutine2() {
  int result = co_await ex5_coroutine1();
  co_return result;
}

void example05() {
  ex5_coroutine2().Then([](int result)
    {
      std::cout << "ex05: result after 1sec=" << result << "\n";
    });

  // Wait 1sec before resolving the Promise to the value 5.
  std::this_thread::sleep_for(1000ms);
  example05_promiseState->resolve(5);
}
//================================================================================
// Use PromiseAll to wait for 3 promises to all get resolved to different value types after 1sec
vector<shared_ptr<BasePromiseState>> example06_promiseStates;

template<typename T>
Promise<T> ex6_resolveAfter1Sec() {
  return Promise<T>([](shared_ptr<PromiseState<T>> promiseState)
    {
      example06_promiseStates.push_back(promiseState);
    });
}

void example06() {
  auto p0 = ex6_resolveAfter1Sec<int>();
  auto p1 = ex6_resolveAfter1Sec<string>();
  auto p2 = ex6_resolveAfter1Sec<double>();

  PromiseAll ex6_promiseAll = PromiseAll({ p0, p1, p2 }).Then([&](auto results)
    {
      cout << "ex06: result0 after 1sec=" << results[0]->value<int>() << "\n";
      cout << "ex06: result1 after 1sec=" << results[1]->value<string>() << "\n";
      cout << "ex06: result2 after 1sec=" << results[2]->value<double>() << "\n";
    });

  // Wait 1sec before resolving all 3 Promises.
  std::this_thread::sleep_for(1000ms);
  example06_promiseStates[0]->resolve<int>(6);
  example06_promiseStates[1]->resolve<string>("six");
  example06_promiseStates[2]->resolve<double>(6.6);
}
//================================================================================
// Use PromiseAny() to wait for any one of 3 promises of different value types to get resolved after 1sec
vector<shared_ptr<BasePromiseState>> example07_promiseStates;

template<typename T>
Promise<T> ex7_resolveAfter1Sec() {
  return Promise<T>([](shared_ptr<PromiseState<T>> promiseState)
    {
      example07_promiseStates.push_back(promiseState);
    });
}

void example07() {
  auto p0 = ex7_resolveAfter1Sec<int>();
  auto p1 = ex7_resolveAfter1Sec<string>();
  auto p2 = ex7_resolveAfter1Sec<double>();

  PromiseAny ex7_promiseAny = PromiseAny({ p0, p1, p2 }).Then([&](auto result)
    {
      if      (result->isValueOfType<int>())    cout << "ex07: result0 after 1sec=" << result->value<int>() << "\n";
      else if (result->isValueOfType<string>()) cout << "ex07: result1 after 1sec=" << result->value<string>() << "\n";
      else                                      cout << "ex07: result2 after 1sec=" << result->value<double>() << "\n";
    });

  // Wait 1sec before resolving just one of the 3 Promise.
  std::this_thread::sleep_for(1000ms);
  example07_promiseStates[1]->resolve<string>("seven");
}


//================================================================================
int main()
{
	example01();
	example02();
  example02a();
  example03();
  example04();
  example05();
  example06();
  example07();
}
