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
    // Therefore, properly handling the result is not trivial.
    console.log("ex07: result after 1sec=" + result);
  });

  // Wait 1sec before resolving just one of the 3 Promise.
  setTimeout(example07_resolvers[1], 1000, "seven");
}

example07();