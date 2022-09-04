# JavaScript-like Promises for C++20

A [JavaScript Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise) is an object that represents the eventual completion (or failure) of an asynchronous operation and its resulting value.

JavaScript Promises and *async* functions introduced by ES6 (also known as ECMAScript-6) are arguably the best thing since the invention of threads.  JavaScript Promises help you to implement legible and maintainable single-threaded asynchronous logic (e.g. for network applications).  Such programs entirely avoid the pitfalls of threads and the overhead associated with thread pools.

This library, named "JS-like Promises" for brevity, enables you to write single-threaded asynchronous C+\+20 code that looks and feels like JavaScript ES6 code.  The semantics and syntax of JS-like Promises are "close enough" to those of JavaScript ES6 Promises (see the [examples](#Examples) section below), and the clarity of any code that uses it is expected to be comparable.  But because it's C++, the speed of the resulting optimized executable is **awesome!**

Use this library to build a wrapper for your favorite asynchronous network IO library (e.g. ASIO) and you'll find that you can easily take your network application to the next level (well beyond just a simple chat client/server or TCP proxy) because of its readability and maintainability.

## Header Files

Include these files in your <code>.cpp</code> files:

```
#include <iostream>  // for cout if you need it
#include <thread>    // for this_thread::sleep_for if you need it
using namespace std;

#include "JSLikeBasePromise.hpp"
#include "JSLikePromise.hpp"
#include "JSLikePromiseAny.hpp"
#include "JSLikePromiseAll.hpp"
using namespace JSLike;
```


## Examples

Here are a few side-by-side code examples that illustrate the capabilities of JS-like Promises and its remarkable syntactic and semantic similarity to JavaScript Promises.  As expected, the C++ code is slightly more verbose due to the language's requirement for type specificity.  So if you are familiar with JavaScipt Promises, then these examples should give you a running start with JS-like Promises.

### Ex 01: Return explicitly resolved Promise from a *regular function* and handle it with "Then"

<!-- BEGIN_MDAUTOGEN: code_table_body('JavaScript', 'C++20', '../Examples_JavaScript/example01.js', '../Examples_C++20/example01.hpp') -->
|JavaScript|C++20|
|----|----|
|<pre>function ex01_function() {<br>  return Promise.resolve(1);<br>}<br><br>function example01() {<br>  ex01_function().then((result) =\> {<br>    console.log("ex01\: result=" + result);<br>  });<br>}<br><br>example01();|<pre>Promise\<int\> ex01_function() {<br>  return Promise\<int\>\:\:resolve(1);<br>}<br><br>void example01() {<br>  ex01_function().Then([](int& result) {<br>    cout \<\< "ex01\: result=" \<\< result \<\< "\n";<br>  });<br>}<br><br>void main() { example01(); }<br>|
<!-- END_MDAUTOGEN -->

### Ex 02: Return initializer-resolved Promise from a *regular function* and handle it with "Then"

<!-- BEGIN_MDAUTOGEN: code_table_body('JavaScript', 'C++20', '../Examples_JavaScript/example02.js', '../Examples_C++20/example02.hpp') -->
|JavaScript|C++20|
|----|----|
|<pre>function ex02_function() {<br>    return new Promise((resolve, reject) =\> {<br>        resolve(2);<br>    });<br>}<br><br>function example02() {<br>  ex02_function().then((result) =\> {<br>    console.log("ex02\: result=" + result);<br>  });<br>}<br><br>example02();<br>|<pre>Promise\<int\> ex02_function() {<br>  return Promise\<int\>([](shared_ptr\<PromiseState\<int\>\> promiseState) {<br>    promiseState-\>resolve(2);<br>  });<br>}<br><br>void example02() {<br>  ex02_function().Then([](int& result) {<br>    cout \<\< "ex02\: result=" \<\< result \<\< "\n";<br>  });<br>}<br><br>void main() { example02(); }|
<!-- END_MDAUTOGEN -->

### Ex 02a: Return initializer-resolved Promise from a *coroutine* and handle it with "Then"

<!-- BEGIN_MDAUTOGEN: code_table_body('JavaScript', 'C++20', '../Examples_JavaScript/example02a.js', '../Examples_C++20/example02a.hpp') -->
|JavaScript|C++20|
|----|----|
|<pre>async function ex02a_function() {<br>    return new Promise((resolve, reject) =\> {<br>        resolve(2);<br>    });<br>}<br><br>function example02a() {<br>  ex02a_function().then((result) =\> {<br>    console.log("ex02\: result=" + result);<br>  });<br>}<br><br>example02a();<br>|<pre>Promise\<int\> ex02a_function() {<br>  co_return Promise\<int\>([](shared_ptr\<PromiseState\<int\>\> promiseState) {<br>    promiseState-\>resolve(2);<br>  });<br>}<br><br>void example02a() {<br>  ex02a_function().Then([](int &result) {<br>    cout \<\< "ex02a\: result=" \<\< result \<\< "\n";<br>  });<br>}<br><br>void main() { example02a(); }|
<!-- END_MDAUTOGEN -->

### Ex 03: Return implicitly-resolved Promise from a *coroutine* and handle it with "Then"

<!-- BEGIN_MDAUTOGEN: code_table_body('JavaScript', 'C++20', '../Examples_JavaScript/example03.js', '../Examples_C++20/example03.hpp') -->
|JavaScript|C++20|
|----|----|
|<pre>async function ex03_async() {<br>  return 3;<br>}<br><br>function example03() {<br>  ex03_async().then((result) =\> {<br>      console.log("ex03\: result=" + result);<br>  });<br>}<br><br>example03();|<pre>Promise\<int\> ex03_coroutine() {<br>  co_return 3;<br>}<br><br>void example03() {<br>  ex03_coroutine().Then([](int& result) {<br>    cout \<\< "ex03\: result=" \<\< result \<\< "\n";<br>  });<br>}<br><br>void main() { example03(); }|
<!-- END_MDAUTOGEN -->

### Ex 04: *co_await* a Promise returned by a *regular function*

<!-- BEGIN_MDAUTOGEN: code_table_body('JavaScript', 'C++20', '../Examples_JavaScript/example04.js', '../Examples_C++20/example04.hpp') -->
|JavaScript|C++20|
|----|----|
|<pre>let example04_resolve;<br><br>function ex04_resolveAfter1Sec() {<br>  return new Promise((resolve) =\> {<br>    example04_resolve = resolve;<br>  });<br>}<br><br>async function ex04_coroutine() {<br>  let result = await ex04_resolveAfter1Sec();<br>  return result;<br>}<br><br>function example04() {<br>  ex04_coroutine().then((result) =\> {<br>    console.log("ex04\: result after 1sec=" + result);<br>  });<br><br>  // Wait 1sec before resolving the Promise to the value 4.<br>  setTimeout(example04_resolve, 1000, 4);<br>}<br><br>example04();|<pre>shared_ptr\<PromiseState\<int\>\> example04_promiseState;<br><br>Promise\<int\> ex04_resolveAfter1Sec() {<br>  return Promise\<int\>([](shared_ptr\<PromiseState\<int\>\> promiseState) {<br>    example04_promiseState = promiseState;<br>  });<br>}<br><br>Promise\<int\> ex04_coroutine() {<br>  int result = co_await ex04_resolveAfter1Sec();<br>  co_return result;<br>}<br><br>void example04() {<br>  ex04_coroutine().Then([](int& result) {<br>    cout \<\< "ex04\: result after 1sec=" \<\< result \<\< "\n";<br>  });<br><br>  // Wait 1sec before resolving the Promise to the value 4.<br>  this_thread\:\:sleep_for(1000ms);<br>  example04_promiseState-\>resolve(4);<br>}<br><br>void main() { example04(); }|
<!-- END_MDAUTOGEN -->

### Ex 05: *co_await* a Promise returned by a *coroutine* that *co_await*s a Promise returned by a *regular function*

<!-- BEGIN_MDAUTOGEN: code_table_body('JavaScript', 'C++20', '../Examples_JavaScript/example05.js', '../Examples_C++20/example05.hpp') -->
|JavaScript|C++20|
|----|----|
|<pre>let example05_resolve;<br><br>function ex5_resolveAfter1Sec() {<br>  return new Promise((resolve) =\> {<br>    example05_resolve = resolve;<br>  });<br>}<br><br>async function ex5_coroutine1() {<br>  let result = await ex5_resolveAfter1Sec();<br>  return result;<br>}<br><br>async function ex5_coroutine2() {<br>  let result = await ex5_coroutine1();<br>  return result;<br>}<br><br>function example05() {<br>  ex5_coroutine2().then((result) =\> {<br>    console.log("ex05\: result after 1sec=" + result);<br>  });<br><br>  // Wait 1sec before resolving the Promise to the value 5.<br>  setTimeout(example05_resolve, 1000, 5);<br>}<br><br>example05();|<pre>shared_ptr\<PromiseState\<int\>\> example05_promiseState;<br><br>Promise\<int\> ex5_resolveAfter1Sec() {<br>  return Promise\<int\>([](shared_ptr\<PromiseState\<int\>\> promiseState)<br>    {<br>      example05_promiseState = promiseState;<br>    });<br>}<br><br>Promise\<int\> ex5_coroutine1() {<br>  int result = co_await ex5_resolveAfter1Sec();<br>  co_return result;<br>}<br><br>Promise\<int\> ex5_coroutine2() {<br>  int result = co_await ex5_coroutine1();<br>  co_return result;<br>}<br><br>void example05() {<br>  ex5_coroutine2().Then([](int& result)<br>    {<br>      cout \<\< "ex05\: result after 1sec=" \<\< result \<\< "\n";<br>    });<br><br>  // Wait 1sec before resolving the Promise to the value 5.<br>  this_thread\:\:sleep_for(1000ms);<br>  example05_promiseState-\>resolve(5);<br>}<br><br>void main() { example05(); }|
<!-- END_MDAUTOGEN -->
