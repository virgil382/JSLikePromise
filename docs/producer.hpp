void startAsyncOperation(shared_ptr<PromiseState<int>> promiseState) {

  // Start an async operation via an API named Deep Thought.
  deepThoughtAPI.cogitate(

    // Deep Thought calls this lambda after it finds the answer to
    // Life, the Universe, and Everything.
    [promiseState](error_code errorCode, int theAnswer) {

      if(errorCode) {
        // Deep Thought detected an error.  Notify the Consumer.
        promiseState->reject(make_exception_ptr(system_error(errorCode)));
      } else {
        // Send the answer to the Consumer.
        promiseState->resolve(theAnswer);
      }

    });
}