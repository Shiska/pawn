#define RUN_TESTS

#include <a_samp>
#include <YSI_Core\y_testing>

#include "bigint"

Test:Run() {
    new cBigInt: int1 = 2147483647;
    new cBigInt: int2 = int1 + int1 + int1 + int1;

    new data[2];

    BigIntGetValues(int2, data);

    ASSERT(data[0] == -4);
    ASSERT(data[1] == 1);

    for(new i; i < sizeof data; ++i) {
        printf("data[%02d] %032b %d", i, data[i], data[i]);
    }
}