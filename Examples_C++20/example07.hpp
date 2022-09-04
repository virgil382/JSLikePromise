vector<shared_ptr<BasePromiseState>> example07_promiseStates;

template<typename T>
Promise<T> ex7_resolveAfter1Sec() {
  return Promise<T>([](shared_ptr<PromiseState<T>> promiseState) {
    example07_promiseStates.push_back(promiseState);
  });
}

void example07() {
  auto p0 = ex7_resolveAfter1Sec<int>();
  auto p1 = ex7_resolveAfter1Sec<string>();
  auto p2 = ex7_resolveAfter1Sec<double>();

  PromiseAny ex7_promiseAny({ p0, p1, p2 }); ex7_promiseAny.Then([&](auto result) {
    if (result->isValueOfType<int>())    cout << "ex07: result0 after 1sec=" << result->value<int>() << "\n";
    else if (result->isValueOfType<string>()) cout << "ex07: result1 after 1sec=" << result->value<string>() << "\n";
    else                                      cout << "ex07: result2 after 1sec=" << result->value<double>() << "\n";
  });

  // Wait 1sec before resolving just one of the 3 Promise.
  std::this_thread::sleep_for(1000ms);
  example07_promiseStates[1]->resolve<string>("seven");
}

void main() { example07(); }