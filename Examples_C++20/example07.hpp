Promise<int> ex02_function() {
  return Promise<int>([](shared_ptr<PromiseState<int>> promiseState) {
    promiseState->resolve(2);
  });
}

void example02() {
  ex02_function().Then([](int& result) {
    std::cout << "ex02: result=" << result << "\n";
  });
}

void main() { example02(); }