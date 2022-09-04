function ex02_function() {
    return new Promise((resolve, reject) => {
        resolve(2);
    });
}

function example02() {
  ex02_function().then((result) => {
    console.log("ex02: result=" + result);
  });
}

example02();
