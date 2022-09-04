vector<shared_ptr<BasePromiseState>> example06_promiseStates;

template<typename T>
Promise<T> ex6_resolveAfter1Sec() {
  return Promise<T>([](shared_ptr<PromiseState<T>> promiseState) {
    example06_promiseStates.push_back(promiseState);
  });
}

void example06() {
  auto p0 = ex6_resolveAfter1Sec<int>();
  auto p1 = ex6_resolveAfter1Sec<string>();
  auto p2 = ex6_resolveAfter1Sec<double>();

  PromiseAll ex6_promiseAll({ p0, p1, p2 });  ex6_promiseAll.Then([&](auto results) {
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

void main() { example06(); }