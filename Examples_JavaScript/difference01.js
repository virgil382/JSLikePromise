async function difference01_function() {
  // JavaScript async functions can return Promises
  // that resolve to void.
  return new Promise(
    (resolve, reject) => {
      resolve();
    });
}


function difference01() {
  let p = difference01_function().then((result) => {
    console.log("dif01: resolved");
  });
}

difference01();