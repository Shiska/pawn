# BigInt

[![sampctl](https://img.shields.io/badge/sampctl-bigint-2f2f2f.svg?style=for-the-badge)](https://github.com/Shiska/pawn/tree/bigint)

## Installation

Simply install to your project:

```bash
sampctl package install Shiska/pawn@bigint
```

Include in your code and begin using the library:

```pawn
#include <bigint>
```

## Usage

```pawn
new cBigInt: int1 = 2147483647;
new cBigInt: int2 = int1 + int1 + int1 + int1;

new data[2];

BigIntGetValues(int2, data);

// data[0] = 11111111111111111111111111111100
// data[1] = 00000000000000000000000000000001
```

## Testing

To test, simply run the package:

```bash
sampctl package ensure
sampctl package build
sampctl package run
```