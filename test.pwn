#define RUN_TESTS

#include <a_samp> // YSI doesn't include all neccessary includes...
#include <YSI_Core\y_testing>

#include "strjoin"

new global[] = {}; // empty array (0 cells)
new globalEntries[][] = {
    "a", "b", "c"
};

Test:Run() {
    ASSERT(strjoin(global, globalEntries, ", ") == 1); // 1 unused cell remaining
    ASSERT(strcmp(global, "a, b, c", false) == 0);

    new localEntries[][] = {
        "a", "b", "c"
    }; // oder must be reversed, lines first base second
    new local[1] = {}; // local array need to have at least 1 cell

    ASSERT(strjoin(local, localEntries, ", ") == 2); // 2 unused cells remaining
    ASSERT(strcmp(local, "a, b, c", false) == 0);
}