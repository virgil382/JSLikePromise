copy ..\readme.md ..\readme.md.bak
type ..\readme.md | node mdautogen.js > ..\readme2.md
del ..\readme.md
ren ..\readme2.md readme.md