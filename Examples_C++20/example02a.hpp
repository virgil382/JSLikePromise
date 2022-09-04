Promise<int> ex02a_function() {
  co_return Promise<int>([](shared_ptr<PromiseState<int>> promiseState) {
    promiseState->resolve(2);
  });
}

void example02a() {
  ex02a_function().Then([](int &result) {
    cout << "ex02a: result=" << result << "\n";
  });
}

void main() { example02a(); }