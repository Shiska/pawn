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

## Notices

Allocates the variables into the heap, could result in incompatibilities with other includes.

## Usage

The same as normal variables except you need to convert them first to BigInt, either by tagging the variable with *cBigInt*.

```pawn
new cBigInt: int1 = 2147483647; // gets converted to BigInt
new cBigInt: int2 = int1 + int1 + int1 + int1;

new data[16];

BigIntToDecimal(int2, data); // "8589934588"
```

Or by using BigInt() but in that case you need to be wary of the execution order.

```pawn
// correct result
new cBigInt: int3 = BigInt(2147483647) + 2147483647 + 2147483647 + 2147483647;
// false result because the first two numbers used the normal addition resulting in an overflow
new cBigInt: int4 = 2147483647 + 2147483647 + BigInt(2147483647) + 2147483647;
```

If you want to use non-const variables with the tag *BigInt* you need to ensure that every variable you use gets freed!

```pawn
new BigInt: int5 = 7;

// int5 = 9; // Never reassign a variable! otherwise it could result in a memory leak

int5++; // 8

new BigInt: int6;

int6 = 9: // Valid because int6 wasn't assigned to anything
```

## Testing

To test, simply run the package:

```bash
sampctl package ensure
sampctl package build
sampctl package run
```