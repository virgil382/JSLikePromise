#include <iostream>  // for cout
#include <thread>    // for this_thread::sleep_for
using namespace std;

#include "../JSLikePromise.hpp"
#include "../JSLikePromiseAll.hpp"
#include "../JSLikePromiseAny.hpp"
using namespace JSLike;

#define main main01
#include "example01.hpp"
#undef main

#define main main02
#include "example02.hpp"
#undef main

#define main main02a
#include "example02a.hpp"
#undef main

#define main main03
#include "example03.hpp"
#undef main

#define main main04
#include "example04.hpp"
#undef main

#define main main05
#include "example05.hpp"
#undef main


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

  PromiseAll ex6_promiseAll({ p0, p1, p2 });  ex6_promiseAll.Then([&](auto results)
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

  PromiseAny ex7_promiseAny({ p0, p1, p2 }); ex7_promiseAny.Then([&](auto result)
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
	main01();
	main02();
  main02a();
  main03();
  main04();
  main05();
  example06();
  example07();
}
