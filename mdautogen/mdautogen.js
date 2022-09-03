// This utility helps to auto-generate (and auto-replace) portions of .md files that would otherwise be tedious to do
// manually by offloading the work to a built-in function.  For example, suppose you have an .md file in which you
// would like to show side-by-side JavaScript and C++20 code in a table.  The following section of the .md would do the
// trick:
// ...
// | JavaScript      | C++20|
// | ----------- | ----------- |
// <!-- BEGIN_MDAUTOGEN: code_table('example01.js', 'example01.hpp') -->
// | <pre>async function implicitPromise() {<br>    return 1;<br>}<br><br>function example01() {<br>    implicitPromise().then((result) =><br>    {<br>        console.log("result=" + result);<br>    });<br>}| <pre>Promise&lt;int&gt; implicitResolve() {<br>  return 1;<br>}<br><br>void example01() {<br>  implicitResolve().Then([](int result)<br>    {<br>      std::cout << "result=" << result << "\n";<br>    });<br>}<br>       |
// <!-- END_MDAUTOGEN -->
// ...
//
// Piping the .md file into this script's stdin would cause the script to:
// 1) Echo to stdout all the text from stdin before "<!-- BEGIN_MDAUTOGEN: code_table('example01.js', 'example01.hpp') -->"
// 2) Discard the rest of the text form stdin before "<!-- END_MDAUTOGEN -->"
// 3) Execute the built-in function "code_table(example01.js, example01.hpp)" and send its output to stdout.
// 4) Echo to stdout all the text from stdin after "<!-- END_MDAUTOGEN -->"
//
// NOTE that the "<!-- BEGIN_MDAUTOGEN: ... -->" and "<!-- END_MDAUTOGEN -->" must each be in a separate text line
// with no preceding or succeeding characters.

let readline = require('readline');
const fs = require('fs');

let isEchoToStdoutEnabled = true;

function doAutogen(line) {
    const re = /<!-- BEGIN_MDAUTOGEN:(.+) -->/;
    const match = line.match(re);
    if(!match || !match[1]) return;

    let command = match[1];
    eval(command);
}

function code_table_body(header1, header2, codeFilePath1, codeFilePath2) {
    let output = "|" + header1 + "|" + header2 + "|\n"
     + "|----|----|\n"
    + "|" + textFileToHtml(codeFilePath1) + "|" + textFileToHtml(codeFilePath2) + "|";

    console.log(output);
}

function textFileToHtml(filePath) {
    //console.log("textFileToHtml(): " + filePath);

    let output = "";

    try {
        let nLines = 0;
        const allFileContents = fs.readFileSync(filePath, 'utf-8');
        allFileContents.split(/\r?\n/).forEach(line =>  {
            if(line === null) return;
            line = escapeMdCharacters(line);
            if(1 === ++nLines) {
                output = "<pre>" + line;
            } else {
                output += "<br>" + line;
            }
        });
    } catch(err) {
        output = "<pre>" + err.toString();
    }

    // TODO: Remove all trailing "<br>"

    return output;
}

function escapeMdCharacters(line) {
    line = replaceCharacter(line, ':', '\\:');
    line = replaceCharacter(line, '<', '\\<');
    line = replaceCharacter(line, '>', '\\>');
    return line;
}

function replaceCharacter(line, search, replaceWith) {
    const searchRegExp = new RegExp(search, 'g'); // Throws SyntaxError
    return line.replace(searchRegExp, replaceWith);
}

//===================================================================================

let rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
    terminal: false
});

// Read from stdin and look for lines that start with:
//   "<!-- BEGIN_MDAUTOGEN:" and
//   "<!-- END_MDAUTOGEN"
rl.on('line', function(line){
    if(line.lastIndexOf("<!-- BEGIN_MDAUTOGEN:") !== -1) {
        isEchoToStdoutEnabled = false;
        console.log(line);
        doAutogen(line);
        return;
    }

    if(line.lastIndexOf("<!-- END_MDAUTOGEN") !== -1) {
        isEchoToStdoutEnabled = true;
        console.log(line);
        return;
    }

    if(isEchoToStdoutEnabled === false) return;

    console.log(line);
})
