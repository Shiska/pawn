# strjoin

[![sampctl](https://img.shields.io/badge/sampctl-strjoin-2f2f2f.svg?style=for-the-badge)](https://github.com/Shiska/pawn/tree/strjoin)

## Installation

Simply install to your project:

```bash
sampctl package install Shiska/pawn@strjoin
```

Include in your code and begin using the library:

```pawn
#include <strjoin>
```

## Usage

Joins the entries together, separated by seperator, into **_dest_**.

```pawn
// returns excess (positive value) / insufficient (negative value) memory in cells
strjoin(dest[], const entries[][], const seperator[] = "", const buffer = sizeof dest, count = sizeof entries);
```

But there is a special case if buffer is set to **_cellmax_** it will overwrite the adjacent memory  
The intended use is with global constant data but it also works with local constant data

```pawn
new global[] = {}; // dest array, increase if strjoin fails due to insufficient memory (add the negative return value to the array size)
new globalEntries[][] = { // will be overwritten and can't be used anymore
    "a", "b", "c"
};
// some function
strjoin(global, globalLines, ", ", cellmax); // global = "a, b, c" 
```

## Testing

To test, simply run the package:

```bash
sampctl package ensure
sampctl package build
sampctl package run
```
