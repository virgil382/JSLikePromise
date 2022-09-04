

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
