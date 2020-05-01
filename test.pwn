#define RUN_TESTS

#include <a_samp>
#include <YSI_Core\y_testing>

#include "bigint"

Test:Run() {
    new cBigInt: int1 = 2147483647;
    new cBigInt: int2 = -(int1 + int1 + int1 + int1); // -8589934588‬
    new cBigInt: int3 = -int2; // 8589934588‬

    new data[2];

    BigIntGetValues(int2, data);

    ASSERT(data[0] == 4);
    ASSERT(data[1] == 65534);

    BigIntGetValues(int3, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);
}