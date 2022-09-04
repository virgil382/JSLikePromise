function ex01_function() {
  return Promise.resolve(1);
}

function example01() {
  ex01_function().then((result) => {
    console.log("ex01: result=" + result);
  });
}

example01();