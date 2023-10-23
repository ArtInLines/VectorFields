# VectorFields

A native desktop app visualizing vector fields. A vector field basically just associates each point in a space with some vector. In this case, the vector represents the velocity of the given point. Or in other words, the app simulates and visualizes where each point would go if it followed said vector.

Each point's vector is computed via a function, that you can change yourself.

## Demo

Disclaimer: The colors have changed by now, but I didn't bother to re-record yet. The functioning of the app is still the same as in the recording shown below.

![Demo Gif](./docs/changing-func-demo.gif)

## Installing

There are pre-built binaries for Windows x86_64, which you can find in the release tab on the side. The app should also run on most other operating systems, but you will have to build it yourself then (see below for instructions).

Simply download and extract the zip file and run the `VectorFields.exe`.

If you want to use the app as your screen-saver, you must follow the following steps:
1.  Download and extract the zip as usual. You can store the files wherever you want
2.  Rename `VectorFields.exe` to `VectorFields.scr`
3.  Right-Click on `VectorFields.scr` and click `install`. The app might start, but you can just close it. There should also be a window letting you decide after how many minutes you want the screen saver to appear.
4.  You need to copy the `assets` folder to `C:/Windows/System32` so Windows can find the correct font. Otherwise the font will look very weird.
5.  Enjoy your screen-saver. You can interact with the app as usual. You exit the screen-saver by exiting the application (via Escape)

## Building

To build the executable, you need both `gcc` and `make` installed and in your path.

After you downloaded the source code, simply navigate to its root folder and run `build.bat`/`build.sh`. You should then find the executable under `bin/VectorFields`.

Alternatively you can also just run `run.bat`/`run.sh` to both build and run the executable.

All dependencies are packaged in the `deps/` folder and are built along with the executable, so no prior setup should be required.

## Dependencies

-   [raylib](https://github.com/raysan5/raylib) to draw shapes onto the screen in a cross-platform way
-   [ail](https://gthub.com/ArtInLines/ail) for ui and common utilities that I wrote myself

## Usage

To exit the application, press `Escape`.

By scrolling or by pinching in/out on your touchpad, you can zoom in/out of the Vector Field.

With `Tab` you can automatically change to function to a random function.

You can also manually change the function, that is used to create the Vector Field. You will see it applied in real time.

Your function needs to be written in a paranthesized [prefix notation](https://en.wikipedia.org/wiki/Polish_notation) (similar to [Lisp](<https://en.wikipedia.org/wiki/Lisp_(programming_language)>)).

The variables `x` and `y` correspond to the x- and y-coordinates of the points in the field. They are normalized to between -1 and 1, with (0, 0) being the center-point of the window.

The result needs to be a 2D vector (constructed with `vec2`), which describes the direction by which the given point moves.

The length of the resulting vector influences the color with which it is drawn. The longer the vector, the more blue and saturated it becomes.

When changing the function, you have currently access to the following functions:

-   `vec2`: Constructs a 2D vector. This should only be used as the outermost function
-   `+`: Adds as many operands as provided
-   `-`: Subtracts as many operands as provided from the first operand. If only one operand was provided, it negates said operand instead
-   `*`: Multiplies as many operands as provided
-   `/`: Divides as many operands as provided from the first operand. If only one operand was provided, it's multiplicative inverse is created instead
-   `**`: Raises the first operand by the power of the next operands. As of now, `**` is treated like a left-associative operation instead of a right-associative one as usual in maths
-   `sin`: Takes one argument and computes the sine of that value
-   `cos`: Takes one argument and computes the cosine of that value
-   `tan`: Takes one argument and computes the tangent of that value
-   `max`: Takes the maximum of two arguments
-   `min`: Takes the minimum of two arguments
-   `clamp`: Clamps the value `x` by the given `min` and `max` values. Arguments should be provided in the following order: `x min max`
-   `lerp`: Linearly interpolates between `min` and `max` by the value `t`. Arguments should be provided in the following order: `t min max`
-   `sqrt`: Returns the square root of the argument
-   `abs`: Returns the absolute value of the argument
-   `log`: Returns the natural logarithm of the argument

Further, you have access to the following variables:

-   `x`: The point's x-coordinate
-   `y`: The point's y-coordinate
-   `xn`: The absolute value of the point's x-coordinate
-   `yn`: The absolute value of the point's y-coordinate
-   `e`: Euler's number e
-   `pi`: Pi

## Inspiration

I got the idea for this project after seeing the [anvaka](https://github.com/anvaka) and [LowByteProductions](https://github.com/lowbyteproductions) create similar projects (albeit for the web):

-   anvaka's "Fieldplay": [Website](https://anvaka.github.io/fieldplay/); [Source](https://github.com/anvaka/fieldplay)
-   LowByteProduction's "Flow-Fields": [YouTube Video](https://www.youtube.com/watch?v=M_SUcX66SDA&t); [Source](https://github.com/lowbyteproductions/flow-fields)
