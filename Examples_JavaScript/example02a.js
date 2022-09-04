async function ex02a_function() {
    return new Promise((resolve, reject) => {
        resolve(2);
    });
}

function example02a() {
  ex02a_function().then((result) => {
    console.log("ex02: result=" + result);
  });
}

example02a();
