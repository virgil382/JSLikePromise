# JavaScript-like Promise for C++20

A [Javacript Promise](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise) is an object that represents the ventual completion (or failure) of an asynchronous operation and its resulting value.

## Header Files
This library enables you to write C++ that looks and feels like JavaScript code.

Add this to your <code>.cpp</code> file:

<pre>
#include "JSLikeBasePromise.hpp"
#include "JSLikePromise.hpp"
#include "JSLikePromiseAny.hpp"
#include "JSLikePromiseAll.hpp"

using namespace JSLike;
</pre>

## Examples

### Ex 01: Return explicitly resolved Promise and handle it with "Then"

<!-- BEGIN_MDAUTOGEN: code_table_body('JavaScript', 'C++20', '../Examples_JavaScript/example01.js', '../Examples_C++20/example01.hpp') -->
|JavaScript|C++20|
|----|----|
|<pre>function ex01_function() {<br>    return Promise.resolve(1);<br>}<br><br>function example01() {<br>    ex01_function().then((result) =\> {<br>        console.log("ex01\: result=" + result);<br>    });<br>}<br><br>example01();|<pre>Promise\<int\> ex01_function() {<br>  return Promise\<int\>\:\:resolve(1);<br>}<br><br>void example01() {<br>  ex01_function().Then([](int& result)<br>    {<br>      std\:\:cout \<\< "ex01\: result=" \<\< result \<\< "\n";<br>    });<br>}|
<!-- END_MDAUTOGEN -->

