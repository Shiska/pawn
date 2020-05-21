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

    ASSERT(int1 == int2);
    ASSERT(int2 == 5);
    // BigInt: operator=(BigIntResult: oper) will be called by any calculation
    // addition
    // BigIntResult: operator+(BigInt: oper1, oper2)                        - int + 10
    // BigIntResult: operator+(BigInt: oper1, BigInt: oper2)                - int + int
    // BigIntResult: operator+(BigInt: oper1, BigIntResult: oper2)          - result + int
    // BigIntResult: operator+(BigIntResult: oper1, oper2)                  - result + 5
    // BigIntResult: operator+(BigIntResult: oper1, BigIntResult: oper2)    - result + result
    new cBigInt: int3 = (int1 + 10) + (int1 + int2 + int2) + 5;

    ASSERT(int3 == 35);
    // neg
    // BigIntResult: operator-(BigInt: oper)                                - (-int)
    // BigIntResult: operator-(BigIntResult: oper)                          - (-result)
    new cBigInt: int4 = -(-int3);

    ASSERT(int4 == 35);
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

    ASSERT(int5 == -10);
    // multiplication
    // BigIntResult: operator*(BigInt: oper1, oper2)                        - int * 10
    // BigIntResult: operator*(BigInt: oper1, BigInt: oper2)                - int * int
    // BigIntResult: operator*(BigInt: oper1, BigIntResult: oper2)          - result * int
    // BigIntResult: operator*(BigIntResult: oper1, oper2)                  - result * 5
    // BigIntResult: operator*(BigIntResult: oper1, BigIntResult: oper2)    - result * result
    new cBigInt: int6 = (int1 * 10) * (int1 * int2 * int2) * 5;

    ASSERT(int6 == 31250);
    // division
    // BigIntResult: operator/(BigInt: oper1, oper2)                        - int / 10
    // BigIntResult: operator/(BigInt: oper1, BigInt: oper2)                - int / int
    // BigIntResult: operator/(BigInt: oper1, BigIntResult: oper2)          - int / result
    // BigIntResult: operator/(BigIntResult: oper1, BigInt: oper2)          - result / int
    // BigIntResult: operator/(BigIntResult: oper1, BigIntResult: oper2)    - result / result
    // BigIntResult: operator/(oper1, BigInt: oper2)                        - 20 / int
    // BigIntResult: operator/(BigIntResult: oper1, oper2)                  - result / 30
    // BigIntResult: operator/(oper1, BigIntResult: oper2)                  - 40 / result
    new cBigInt: int7 = ((int6 / int2) / int1) / (int3 / (int1 / 2)) / (33 / ((200 / int3) / 3));

    ASSERT(int7 == 2);
    // modulo
    // BigIntResult: operator%(BigInt: oper1, oper2)                        - int % 10
    // BigIntResult: operator%(BigInt: oper1, BigInt: oper2)                - int % int
    // BigIntResult: operator%(BigInt: oper1, BigIntResult: oper2)          - int % result
    // BigIntResult: operator%(BigIntResult: oper1, BigInt: oper2)          - result % int
    // BigIntResult: operator%(BigIntResult: oper1, BigIntResult: oper2)    - result % result
    // BigIntResult: operator%(oper1, BigInt: oper2)                        - 20 % int
    // BigIntResult: operator%(BigIntResult: oper1, oper2)                  - result % 30
    // BigIntResult: operator%(oper1, BigIntResult: oper2)                  - 40 % result
    new cBigInt: int8 = ((int6 % int3) % int6) % (int3 % (int3 % 9)) % (33 % ((200 % int3) % 15));

    ASSERT(int8 == 0);
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

Test:Comparision() {
    new cBigInt: int1 = 5;
    new cBigInt: int2 = -int1;

    ASSERT(int1 == -int2);
    ASSERT(-int1 == int2);

    ASSERT(int1 != int2);
    ASSERT(-int1 != -int2);

    ASSERT(int2 < int1);
    ASSERT(-int2 > -int1);

    ASSERT(int1 > int2);
    ASSERT(-int1 < -int2);

    ASSERT(int2 <= int1);
    ASSERT(-int2 >= -int1);

    ASSERT(int2 <= -int1);
    ASSERT(-int2 >= int1);

    ASSERT(int1 >= int2);
    ASSERT(-int1 <= -int2);

    ASSERT(int1 >= -int2);
    ASSERT(-int1 <= int2);

    new cBigInt: int3 = 3;

    ASSERT(int1 != int3);
    ASSERT(-int1 != -int3);

    ASSERT(int2 != int3);
    ASSERT(-int2 != -int3);

    ASSERT(int2 < int3);
    ASSERT(-int2 > -int3);

    ASSERT(int1 > int3);
    ASSERT(-int1 < -int3);

    ASSERT(int2 <= int1);
    ASSERT(-int2 >= -int1);
}

Test:AddSub() {
    new cBigInt: int1 = -2147483647;
    new cBigInt: int2 = -(int1 + int1 + int1 + int1); // 8589934588‬
    new cBigInt: int3 = -int2; // -8589934588‬

    new data[8];

    BigIntToBinary(int2, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);
    ASSERT(data[2] == 0);

    BigIntToBinary(int3, data);

    ASSERT(data[0] == 4);
    ASSERT(data[1] == -2);
    ASSERT(data[2] == -1);

    ASSERT(int3 == -int2);

    new cBigInt: int4 = int3 - int1 - int1 - int1 - int1 - int1; // 2147483647

    BigIntToBinary(int4, data);

    ASSERT(data[0] == 2147483647);
}

Test:Mul() {
    new cBigInt: int1 = -2147483647;
    new cBigInt: int2 = int1 * 4; // -8589934588‬

    new data[32];

    BigIntToBinary(int2, data);

    ASSERT(data[0] == 4);
    ASSERT(data[1] == -2);
    ASSERT(data[2] == -1);

    new cBigInt: int3 = int1 * -4; // 8589934588‬

    BigIntToBinary(int3, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);
    ASSERT(data[2] == 0);

    new cBigInt: int4 = int1 * int1;

    BigIntToBinary(int4, data);

    ASSERT(data[0] == 1);
    ASSERT(data[1] == 1073741823);
    ASSERT(data[2] == 0);

    new cBigInt: int5 = int2 * int2; // 73786976226118729744

    BigIntToBinary(int5, data);

    ASSERT(data[0] == 0x00000010); // 0x00000010
    ASSERT(data[1] == 0xfffffff0); // 0xfffffff0
    ASSERT(data[2] == 0x3); // 0x3

    BigIntToHex(int5, data);

    ASSERT(!strcmp(data, "3FFFFFFF000000010"));

    BigIntToDecimal(int5, data);

    ASSERT(!strcmp(data, "73786976226118729744"));
}

Test:Shift() {
    new cBigInt: int1 = 66666;

    BigIntShiftLeft(_: int1, 15);
    BigIntShiftRight(_: int1, 15);
}

Test:Karatsuba() {
    // some huge number
    new cBigInt: int1 = cellmax - 1;
    new cBigInt: int2 = int1 * int1;
    new cBigInt: int3 = int2 * int2;
    new cBigInt: int4 = int3 * int3;
    new cBigInt: int5 = int4 * int4;

    new cBigInt: f1 = -int5;
    new cBigInt: f2 = f1;

    new cBigInt: int7 = f1 * f1;
    new cBigInt: int8 = f1 * f2;
    new cBigInt: int9 = BigIntResult: BigIntKaratsuba(_: f1, _: f1);

    ASSERT(int7 == int8);
    ASSERT(int8 == int9);

    new hex[256];

    BigIntToHex(int5, hex);

    ASSERT(!strcmp(hex, "FFFFFFC00000077FFFFF740000071BFFFFBBC00001F47FFFF4D400003245FFFF4D400001F47FFFFBBC0000071BFFFFF7400000077FFFFFFC000000010000")); // 2147483646^16

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

Test:Div() {
    for(new i = 0; i < 100; ++i) {
        new int1 = random(cellmax);
        new int2 = random(499) + 1;
        new int3 = int2 << 16;

        new cBigInt: bint1 = int1;
        new cBigInt: bint2 = int2;
        new cBigInt: bint3 = int3;
        new cBigInt: bint4 = bint1 / bint2;
        new cBigInt: bint5 = bint1 % bint2;
        new cBigInt: bint6 = bint1 / bint3;
        new cBigInt: bint7 = bint1 % bint3;

        ASSERT(bint4 == (int1 / int2));
        ASSERT(bint5 == (int1 % int2));
        ASSERT(bint6 == (int1 / int3));
        ASSERT(bint7 == (int1 % int3));
    }
}

Test:Example() {
    new cBigInt: int1 = 2147483647;
    new cBigInt: int2 = int1 + int1 + int1 + int1;
    new cBigInt: int3 = BigInt(2147483647) + 2147483647 + 2147483647 + 2147483647;
    new cBigInt: int4 = 2147483647 + 2147483647 + BigInt(2147483647) + 2147483647;

    new data[16];

    BigIntToDecimal(int2, data);

    ASSERT(!strcmp(data, "8589934588"));
    ASSERT(int2 == int3);
    ASSERT(int3 != int4);

    new BigInt: int5 = 7;

    // int5 = 9; // Never reassign a variable! otherwise it will result in a memory leak

    ASSERT(++int5 == 8);
    ASSERT(--int5 == 7);

    new BigInt: int6;

    int6 = 9; // Valid because in6 wasn't assigned to anything
}