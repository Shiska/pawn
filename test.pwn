#define RUN_TESTS

#include <YSI_Core\y_testing>

#include "CountingSort"

Swap({Float, _}: ...) {
    #emit lref.s.pri 12
    #emit lref.s.alt 16
    #emit sref.s.alt 12
    #emit sref.s.pri 16
}

ShuffleArray(array[], size = sizeof array) {
    while(--size >= 1) {
        Swap(array[size], array[random(size)]);
    }
}

bool: IsSorted(const array[], size = sizeof array) { // checks if array is sorted ascending
    new cmp = array[--size];

    while(--size >= 0) {
        if(array[size] > cmp) {
            return false;
        }
        cmp = array[size];
    }
    return true;
}

Test:Run() {
    new array[50] = {1, 4, ...};

    ShuffleArray(array);

    ASSERT(!IsSorted(array));
    ASSERT(CountingSort(array));
    ASSERT(IsSorted(array));
}