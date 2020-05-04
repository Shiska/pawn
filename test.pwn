#define RUN_TESTS

#include <a_samp>
#include <YSI_Core\y_testing>

#include "bigint"

TestInit:Run() { // call all operators at least once
    // assignment
    new cBigInt: int1 = 5; // BigInt: operator=(oper)
    new cBigInt: int2 = int1; // BigInt: operator=(BigInt: oper)
    // BigInt: operator=(BigIntResult: oper) will be called by any calculation
    // addition
    // BigIntResult: operator+(BigInt: oper1, oper2)                        - int + 10
    // BigIntResult: operator+(BigInt: oper1, BigInt: oper2)                - int + int
    // BigIntResult: operator+(BigInt: oper1, BigIntResult: oper2)          - result + int
    // BigIntResult: operator+(BigIntResult: oper1, oper2)                  - result + 5
    // BigIntResult: operator+(BigIntResult: oper1, BigIntResult: oper2)    - result + result
    new cBigInt: int3 = (int1 + 10) + (int1 + int2 + int2) + 5; // 35
    // neg
    // BigIntResult: operator-(BigInt: oper)                                - (-int)
    // BigIntResult: operator-(BigIntResult: oper)                          - (-result)
    new cBigInt: int4 = -(-int3);
    // subtraction
    // BigIntResult: operator-(BigInt: oper1, oper2)                        - int - 10
    // BigIntResult: operator-(BigInt: oper1, BigInt: oper2)                - int - int
    // BigIntResult: operator-(BigInt: oper1, BigIntResult: oper2)          - int - result
    // BigIntResult: operator-(BigIntResult: oper1, BigInt: oper2)          - result - int
    // BigIntResult: operator-(BigIntResult: oper1, BigIntResult: oper2)    - result - result
    // BigIntResult: operator-(oper1, BigInt: oper2)                        - 20 - int
    // BigIntResult: operator-(BigIntResult: oper1, oper2)                  - result - 30
    // BigIntResult: operator-(oper1, BigIntResult: oper2)                  - 40 - result
    new cBigInt: int5 = (int3 - (int1 - 10)) - ((int1 - int2) - int4) - (40 - ((20 - int3) - 30));
    // multiplication
    // BigIntResult: operator*(BigInt: oper1, oper2)                        - int * 10
    // BigIntResult: operator*(BigInt: oper1, BigInt: oper2)                - int * int
    // BigIntResult: operator*(BigInt: oper1, BigIntResult: oper2)          - result * int
    // BigIntResult: operator*(BigIntResult: oper1, oper2)                  - result * 5
    // BigIntResult: operator*(BigIntResult: oper1, BigIntResult: oper2)    - result * result
    new cBigInt: int6 = (int1 * 10) * (int1 * int2 * int2) * 5; // 31250
    // destructor
    // operator~(BigInt: oper[], size)
}

Test:Run() {
    new cBigInt: int1 = -2147483647;
    new cBigInt: int2 = -(int1 + int1 + int1 + int1); // 8589934588‬
    new cBigInt: int3 = -int2; // -8589934588‬

    new data[2];

    BigIntGetValues(int2, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);

    BigIntGetValues(int3, data);

    ASSERT(data[0] == 4);
    ASSERT(data[1] == -2);

    new cBigInt: int4 = int3 - int1 - int1 - int1 - int1 - int1; // 2147483647

    BigIntGetValues(int4, data);

    ASSERT(data[0] == 2147483647);

    new cBigInt: int5 = int1 * 4; // -8589934588‬

    BigIntGetValues(int5, data);

    ASSERT(data[0] == 4);
    ASSERT(data[1] == -2);

    new cBigInt: int6 = int1 * -4; // 8589934588‬

    BigIntGetValues(int6, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);
}