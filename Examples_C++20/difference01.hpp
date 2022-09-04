Promise<> difference01_function() {
  // C++20 coroutines can't return Promise<void>.
  // The code below won't compile.
  //co_return Promise<>(
  //  [](auto promiseState) {
  //    promiseState->resolve();
  //  });

  // You can do this instead:
  co_return co_await Promise<>(
    [](auto promiseState) {
      promiseState->resolve();
    });
}

void difference01() {
  Promise<> p = difference01_function(); p.Then([]() {
    cout << "dif01: resolved\n";
  });
}

void main() { difference01(); }