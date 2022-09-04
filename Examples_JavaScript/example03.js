async function ex03_async() {
  return 3;
}

function example03() {
  ex03_async().then((result) => {
      console.log("ex03: result=" + result);
  });
}

example03();