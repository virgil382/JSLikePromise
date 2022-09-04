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
  ex5_coroutine2().Then([](int& result)
    {
      cout << "ex05: result after 1sec=" << result << "\n";
    });

  // Wait 1sec before resolving the Promise to the value 5.
  this_thread::sleep_for(1000ms);
  example05_promiseState->resolve(5);
}

void main() { example05(); }