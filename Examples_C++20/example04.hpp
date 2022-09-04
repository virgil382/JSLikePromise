shared_ptr<PromiseState<int>> example04_promiseState;

Promise<int> ex04_resolveAfter1Sec() {
  return Promise<int>([](shared_ptr<PromiseState<int>> promiseState) {
    example04_promiseState = promiseState;
  });
}

Promise<int> ex04_coroutine() {
  int result = co_await ex04_resolveAfter1Sec();
  co_return result;
}

void example04() {
  ex04_coroutine().Then([](int& result) {
    cout << "ex04: result after 1sec=" << result << "\n";
  });

  // Wait 1sec before resolving the Promise to the value 4.
  this_thread::sleep_for(1000ms);
  example04_promiseState->resolve(4);
}

void main() { example04(); }