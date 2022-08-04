
//================================================================================
// Function returns implicitly-resolved Promise.
// Caller with "then".
function ex01_function() {
    return Promise.resolve(1);
}

function example01() {
    ex01_function().then((result) =>
    {
        console.log("ex01: result=" + result);
    });
}

example01();

//================================================================================
// Function returns initializer-resolved Promise.
// Caller with "then".

function ex02_function() {
    return new Promise((resolve, reject) =>
    {
        resolve(2);
    })
}

function example02() {
    ex02_function().then((result) => {
        console.log("ex02: result=" + result);
    });
}

example02();

//================================================================================
// Async/coroutine returns initializer-resolved Promise.
// Caller with "then".

async function ex02a_function() {
    return new Promise((resolve, reject) => {
        resolve(2);
    })
}

function example02a() {
    ex02a_function().then((result) => {
        console.log("ex02: result=" + result);
    });
}

example02a();

//================================================================================
// Async/coroutine returns a implicitly-resolved Promise.
// Caller with "then".

async function ex03_async() {
    return 3;
}

function example03() {
    ex03_async().then((result) => {
        console.log("ex03: result=" + result);
    });
}

example03();

//================================================================================
// Async/Coroutine awaits on a Promise before resuming.
let example04_resolve;

function ex04_resolveAfter1Sec() {
    return new Promise((resolve) =>
    {
        example04_resolve = resolve;
    });
}


async function ex04_coroutine() {
    let result = await ex04_resolveAfter1Sec();
    return result;
}

function example04() {
    ex04_coroutine().then((result) =>
    {
        console.log("ex04: result after 1sec=" + result);
    });

    // Wait 1sec before resolving the Promise to the value 4.
    setTimeout(example04_resolve, 1000, 4);
}

example04();

//================================================================================
// Async/Coroutine awaits on another async/coroutine that get resolved after 1sec.
let example05_resolve;

function ex5_resolveAfter1Sec() {
    return new Promise((resolve) =>
    {
        example05_resolve = resolve;
    });
}

async function ex5_coroutine1() {
    let result = await ex5_resolveAfter1Sec();
    return result;
}

async function ex5_coroutine2() {
    let result = await ex5_coroutine1();
    return result;
}

function example05() {
    ex5_coroutine2().then((result) =>
    {
        console.log("ex05: result after 1sec=" + result);
    });

    // Wait 1sec before resolving the Promise to the value 5.
    setTimeout(example05_resolve, 1000, 5);
}

example05();
//================================================================================
// Use Promise.all() to wait for 3 promises to all get resolved to different value types after 1sec
let example06_resolvers = [];

function ex6_resolveAfter1Sec() {
    return new Promise((resolve) => {
        example06_resolvers.push(resolve);
    });
}

function example06() {
    let p0 = ex6_resolveAfter1Sec();
    let p1 = ex6_resolveAfter1Sec();
    let p2 = ex6_resolveAfter1Sec();

    let ex6_promiseAll = Promise.all([p0, p1, p2]).then((results) =>
    {
        console.log("ex06: result0 after 1sec=" + results[0]);
        console.log("ex06: result1 after 1sec=" + results[1]);
        console.log("ex06: result2 after 1sec=" + results[2]);
    });

    // Wait 1sec before resolving all 3 Promises.
    setTimeout(example06_resolvers[0], 1000, 6);
    setTimeout(example06_resolvers[1], 1000, "six");
    setTimeout(example06_resolvers[2], 1000, 6.6);
}

example06();
//================================================================================
// Use Promise.any() to wait for any one of 3 promises of different value types to get resolved after 1sec
let example07_resolvers = [];

function ex7_resolveAfter1Sec() {
    return new Promise((resolve) => {
        example07_resolvers.push(resolve);
    });
}

function example07() {
    let p0 = ex7_resolveAfter1Sec();
    let p1 = ex7_resolveAfter1Sec();
    let p2 = ex7_resolveAfter1Sec();

    let ex7_promiseAny = Promise.any([p0, p1, p2]).then((result) => {
        // Note that we don't know which Promise was resolved.
        // Note that it's also not trivial to get the type of result.
        console.log("ex07: result after 1sec=" + result);
    });

    // Wait 1sec before resolving just one of the 3 Promise.
    setTimeout(example07_resolvers[1], 1000, "seven");
}

example07();
