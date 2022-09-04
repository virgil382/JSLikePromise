Promise<int> ex03_coroutine() {
  co_return 3;
}

void example03() {
  ex03_coroutine().Then([](int& result) {
    cout << "ex03: result=" << result << "\n";
  });
}

void main() { example03(); }