let example05_resolve;

function ex5_resolveAfter1Sec() {
  return new Promise((resolve) => {
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
  ex5_coroutine2().then((result) => {
    console.log("ex05: result after 1sec=" + result);
  });

  // Wait 1sec before resolving the Promise to the value 5.
  setTimeout(example05_resolve, 1000, 5);
}

example05();