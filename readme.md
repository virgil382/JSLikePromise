# JavaScript-like Promise for C++20

A [Javacript Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise) is an object that represents the ventual completion (or failure) of an asynchronous operation and its resulting value.

## Header Files
This library enables you to write C++ that looks and feels like JavaScript code.

Add this to your <code>.cpp</code> file:

<pre>
#include "JSLikeVoidPromise.hpp"
#include "JSLikePromise.hpp"
#include "JSLikePromiseAny.hpp"
#include "JSLikePromiseAll.hpp"

using namespace JSLike;
</pre>

## Examples

### Ex 01: Implicit Resolve and "Then"

| JavaScript      | C++20|
| ----------- | ----------- |
| <pre>async function implicitPromise() {<br>    return 1;<br>}<br><br>function example01() {<br>    implicitPromise().then((result) =><br>    {<br>        console.log("result=" + result);<br>    });<br>}| <pre>Promise&lt;int&gt; functionImplicitResolve() {<br>  return 1;<br>}<br><br>void example01() {<br>  functionImplicitResolve().Then([](int result)<br>    {<br>      std::cout << "result=" << result << "\n";<br>    });<br>}<br>       |

