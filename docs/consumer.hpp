auto p = Promise<int>([this](auto promiseState) {
  // This is the TaskInitializer.
  startAsyncOperation(promiseState);
});

p.Then([](int &result) {
  // This is the ThenCallback.
  cout << "result = " << result << "\n";
});

p.Catch([](exception_ptr ex) {
  // This is the CatchCallback.
  // Handle exception
});