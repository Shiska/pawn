# CountingSort

[![sampctl](https://shields.southcla.ws/badge/sampctl-CountingSort-2f2f2f.svg?style=for-the-badge)](https://github.com/Shiska/CountingSort)

## Installation

Simply install to your project:

```bash
sampctl package install Shiska/CountingSort
```

Include in your code and begin using the library:

```pawn
#include <CountingSort>
```

## Usage

CountingSort is limited to pure integer arrays.  
But it is usually faster than quicksort depending on the range of values.

```pawn
CountingSort(array);
```

## Testing

To test, simply run the package:

```bash
sampctl package ensure
sampctl package build
sampctl package run
```
