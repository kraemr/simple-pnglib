# simple-pnglib




## What is this for ? 

This library can read pixel data out of a png and store it in RGBA format and can write it back to a png file.
Later versions will include More Options, like writing image as greyscale,RGB,RGBA or reading values differently.
However this library is intended to be as simple as possible.
As a lot of png libraries have a lot of features that are just confusing.

## What to expect ?

Just to temper your expectations, this will not have any more advanced features like iCC profiles,gamma,sRGB and other things like that. Its for reading the pixelvalues doing something to them or not and write them back to a file ... or not. This Library was more of a learning experience for me.

## How to use:

`gcc yourfile.c pnglib.dll zlib.dll -lm`

or 

`gcc yourfile.c pnglib.c -lz -lm`

or 

`gcc yourfile.c pnglib.so zlib.so -lm`

there are some example on how to use it in a program in example.c

## Note

This Library works on Windows and Linux, however since this is still very early in development<br />
expect some inefficiencies,oddities,bugs and other growing pains.<br />
The Biggest deficiency is writing to a PNG file as i have not yet implemented any way 
<br />
This Library has precompiled .dll and .so files if you wanted to <br />you could also compile this by source and use your own zlib dll or so files<br />
<br />
Also if you want to write your own png-decoder or encoder a good place to start would be here: <br />
https://www.w3.org/TR/PNG/ 


<br />
<br />

## Feel Free to donate if you want:
https://www.buymeacoffee.com/rkraemer


