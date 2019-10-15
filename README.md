# pawn

[![sampctl](https://shields.southcla.ws/badge/sampctl-strjoin-2f2f2f.svg?style=for-the-badge)](https://github.com/Shiska/pawn/tree/strjoin)

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

Global arrays, first the **_base_**, second the **_lines_**

```pawn
new global[] = {}; // empty array (0 cells), increase if strjoin fails due to insufficient memory
new globalLines[][] = {
    "a", "b", "c"
};

// some function
strjoin(global, globalLines, ", "); // global = "a, b, c"
```

Local arrays, reversed order, first the **_lines_**, second the **_base_**

```pawn
new localLines[][] = {
    "a", "b", "c"
};
new local[1] = {}; // local array need to have at least 1 cell, increase if strjoin fails due to insufficient memory

strjoin(local, localLines, ", "); // local = "a, b, c"
```

Note: Don't put anything between **_base_** and **_lines_**  
Note: If strjoins fails increase **_base_** array by the negative returned value  
Note: Overwrites / reuses **_lines_** array, it will be unuseable afterwards

## Testing

To test, simply run the package:

```bash
sampctl package ensure
sampctl package build
sampctl package run
```