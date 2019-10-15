# CountingSort

[![sampctl](https://shields.southcla.ws/badge/sampctl-CountingSort-2f2f2f.svg?style=for-the-badge)](https://github.com/Shiska/pawn/tree/countingsort)

## Installation

Simply install to your project:

```bash
sampctl package install Shiska/pawn@countingsort
```

Include in your code and begin using the library:

```pawn
#include <countingsort>
```

## Usage

Countingsort is limited to pure integer arrays.  
But it is usually faster than quicksort depending on the range of values.

```pawn
countingsort(array);
```

## Testing

To test, simply run the package:

```bash
sampctl package ensure
sampctl package build
sampctl package run
```
