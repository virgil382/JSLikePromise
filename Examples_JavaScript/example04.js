let example04_resolve;

function ex04_resolveAfter1Sec() {
  return new Promise((resolve) => {
    example04_resolve = resolve;
  });
}

async function ex04_coroutine() {
  let result = await ex04_resolveAfter1Sec();
  return result;
}

function example04() {
  ex04_coroutine().then((result) => {
    console.log("ex04: result after 1sec=" + result);
  });

  // Wait 1sec before resolving the Promise to the value 4.
  setTimeout(example04_resolve, 1000, 4);
}

example04();