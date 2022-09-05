auto p = Promise<int>([this](auto promiseState) {
  startAsyncOperation(promiseState);
});

p.Then([](int &result) {
  cout << "result = " << result << "\n";
});

p.Catch([](exception_ptr ex) {
  // Handle exception
});