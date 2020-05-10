#define RUN_TESTS

// #include <a_samp>
#include <a_samp>
#include <YSI_Core\y_testing>

#include "bigint"

#define SPEEDTEST(%0,%1)\
	for(new _wait = -tickcount(), _count = 1; _count; )\
        for(; ; ++_count)\
            if(%1 < _count) {\
                _wait += tickcount();\
                printf("%32s %10d times / %10d ms = %8.2f times/ms", %0, %1, _wait, floatdiv(%1, _wait));\
                _count = 0;\
                break;\
            } else

Test:Operator() { // calls all operators at least once
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
    // shifting
    BigIntShiftLeft(_: int6, 2225); // TODO: test
    BigIntShiftRight(_: int6, 2225);
    // equal
    if(-int1 == -int2) {} // bool: operator==(BigIntResult: oper1, BigIntResult: oper2)
    if(int3 == -int4) {} // bool: operator==(BigInt: oper1, BigIntResult: oper2)
    if(int5 == int6) {}  // bool: operator==(BigInt: oper1, BigInt: oper2)
    if(-int1 == 5) {} // bool: operator==(BigIntResult: oper1, oper2)
    if(int3 == -5) {} // bool: operator==(BigInt: oper1, oper2)
    // not equal
    if(-int1 != -int2) {} // bool: operator!=(BigIntResult: oper1, BigIntResult: oper2)
    if(int3 != -int4) {} // bool: operator!=(BigInt: oper1, BigIntResult: oper2)
    if(int5 != int6) {}  // bool: operator!=(BigInt: oper1, BigInt: oper2)
    if(-int1 != 5) {} // bool: operator!=(BigIntResult: oper1, oper2)
    if(int3 != -5) {} // bool: operator!=(BigInt: oper1, oper2)
    // less
    if(-int1 < -int2) {} // bool: operator<(BigIntResult: oper1, BigIntResult: oper2)
    if(-int3 < int4) {} // bool: operator<(BigIntResult: oper1, BigInt: oper2)
    if(int4 < -int3) {} // bool: operator<(BigInt: oper1, BigIntResult: oper2)
    if(int5 < int6) {} // bool: operator<(BigInt: oper1, BigInt: oper2)
    if(-int1 < 5) {} // bool: operator<(BigIntResult: oper1, oper2)
    if(5 < -int1) {} // bool: operator<(oper1, BigIntResult: oper2)
    if(int3 < -5) {} // bool: operator<(BigInt: oper1, oper2)
    if(-5 < int3) {} // bool: operator<(oper1, BigInt: oper2)
    // greater
    if(-int1 > -int2) {} // bool: operator>(BigIntResult: oper1, BigIntResult: oper2)
    if(-int3 > int4) {} // bool: operator>(BigIntResult: oper1, BigInt: oper2)
    if(int4 > -int3) {} // bool: operator>(BigInt: oper1, BigIntResult: oper2)
    if(int5 > int6) {} // bool: operator>(BigInt: oper1, BigInt: oper2)
    if(-int1 > 5) {} // bool: operator>(BigIntResult: oper1, oper2)
    if(5 > -int1) {} // bool: operator>(oper1, BigIntResult: oper2)
    if(int3 > -5) {} // bool: operator>(BigInt: oper1, oper2)
    if(-5 > int3) {} // bool: operator>(oper1, BigInt: oper2)
    // less equal
    if(-int1 <= -int2) {} // bool: operator<=(BigIntResult: oper1, BigIntResult: oper2)
    if(-int3 <= int4) {} // bool: operator<=(BigIntResult: oper1, BigInt: oper2)
    if(int4 <= -int3) {} // bool: operator<=(BigInt: oper1, BigIntResult: oper2)
    if(int5 <= int6) {} // bool: operator<=(BigInt: oper1, BigInt: oper2)
    if(-int1 <= 5) {} // bool: operator<=(BigIntResult: oper1, oper2)
    if(5 <= -int1) {} // bool: operator<=(oper1, BigIntResult: oper2)
    if(int3 <= -5) {} // bool: operator<=(BigInt: oper1, oper2)
    if(-5 <= int3) {} // bool: operator<=(oper1, BigInt: oper2)
    // greater equal
    if(-int1 >= -int2) {} // bool: operator>=(BigIntResult: oper1, BigIntResult: oper2)
    if(-int3 >= int4) {} // bool: operator>=(BigIntResult: oper1, BigInt: oper2)
    if(int4 >= -int3) {} // bool: operator>=(BigInt: oper1, BigIntResult: oper2)
    if(int5 >= int6) {} // bool: operator>=(BigInt: oper1, BigInt: oper2)
    if(-int1 >= 5) {} // bool: operator>=(BigIntResult: oper1, oper2)
    if(5 >= -int1) {} // bool: operator>=(oper1, BigIntResult: oper2)
    if(int3 >= -5) {} // bool: operator>=(BigInt: oper1, oper2)
    if(-5 >= int3) {} // bool: operator>=(oper1, BigInt: oper2)
    // not
    if(!-int1) {} // bool: operator!(BigIntResult: oper)
    if(!int1) {} // bool: operator!(BigInt: oper)
    // destructor
    // operator~(BigInt: oper[], size)
}

Test:AddSub() {
    new cBigInt: int1 = -2147483647;
    new cBigInt: int2 = -(int1 + int1 + int1 + int1); // 8589934588‬
    new cBigInt: int3 = -int2; // -8589934588‬

    new data[8];

    BigIntGetValues(int2, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);
    ASSERT(data[2] == 0);

    BigIntGetValues(int3, data);

    ASSERT(data[0] == 4);
    ASSERT(data[1] == -2);
    ASSERT(data[2] == -1);

    ASSERT(int3 == -int2);

    new cBigInt: int4 = int3 - int1 - int1 - int1 - int1 - int1; // 2147483647

    BigIntGetValues(int4, data);

    ASSERT(data[0] == 2147483647);
}

Test:Mul() {
    new cBigInt: int1 = -2147483647;
    new cBigInt: int2 = int1 * 4; // -8589934588‬

    new data[8];

    BigIntGetValues(int2, data);

    ASSERT(data[0] == 4);
    ASSERT(data[1] == -2);
    ASSERT(data[2] == -1);

    new cBigInt: int3 = int1 * -4; // 8589934588‬

    BigIntGetValues(int3, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);
    ASSERT(data[2] == 0);

    new cBigInt: int4 = int1 * int1;

    BigIntGetValues(int4, data);

    ASSERT(data[0] == 1);
    ASSERT(data[1] == 1073741823);
    ASSERT(data[2] == 0);

    new cBigInt: int5 = int2 * int2; // 73786976226118729744

    BigIntGetValues(int5, data);

    ASSERT(data[0] == 0x00000010); // 0x00000010
    ASSERT(data[1] == 0xfffffff0); // 0xfffffff0
    ASSERT(data[2] == 0x3); // 0x3

    new hex[32];

    BigIntGetHex(int5, hex);

    ASSERT(strcmp(hex, "3FFFFFFF000000010") == 0);
}

Test:Karatsuba() {
    // some huge number
    new cBigInt: int1 = cellmax - 1;
    new cBigInt: int2 = int1 * int1;
    new cBigInt: int3 = int2 * int2;
    new cBigInt: int4 = int3 * int3;
    new cBigInt: int5 = int5 * int5; // 2147483646^16 - 0xFFFFFFC00000077FFFFF740000071BFFFFBBC00001F47FFFF4D400003245FFFF4D400001F47FFFFBBC0000071BFFFFF7400000077FFFFFFC000000010000

    new cBigInt: f1 = -int5;
    new cBigInt: f2 = f1;

    new cBigInt: int7 = f1 * f1;
    new cBigInt: int8 = f1 * f2;
    new cBigInt: int9 = BigIntResult: BigIntKaratsuba(_: f1, _: f1);

    ASSERT(int7 == int8);
    ASSERT(int8 == int9);

    // BigIntSquare                         100000 times /      17491 ms =        5.71 times/ms
    // BigIntMul                            100000 times /      19989 ms =        5.00 times/ms
    // BigIntKaratsuba                       10000 times /      40615 ms =        0.24 times/ms
    // BigIntSquare                         100000 times /      17889 ms =        5.59 times/ms
    // BigIntMul                            100000 times /      21731 ms =        4.60 times/ms
    // BigIntKaratsuba                       10000 times /      50038 ms =        0.19 times/ms
    // BigIntSquare                         100000 times /      18557 ms =        5.38 times/ms
    // BigIntMul                            100000 times /      23788 ms =        4.20 times/ms
    // BigIntKaratsuba                       10000 times /      40724 ms =        0.24 times/ms

    // for(new i; i < 3; ++i) {
    //     SPEEDTEST("BigIntSquare", 100000) {
    //         new cBigInt: tint = f1 * f1;
    //     }
    //     SPEEDTEST("BigIntMul", 100000) {
    //         new cBigInt: tint = f1 * f2;
    //     }
    //     SPEEDTEST("BigIntKaratsuba", 10000) {
    //         new cBigInt: tint = BigIntResult: BigIntKaratsuba(_: f1, _: f1);
    //     }
    // }
}

Test:Pow() {
    new cBigInt: int1 = cellmax;
    new cBigInt: int2 = int1 * int1;
    new cBigInt: int3 = int2 * int2;
    new cBigInt: int4 = BigIntResult: BigIntPow(_: int1, 4);

    ASSERT(int3 == int4);
}