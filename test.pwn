#define RUN_TESTS

#include <a_samp> // YSI doesn't include all neccessary includes...
#include <YSI_Core\y_testing>

#include "strjoin"

new global[] = {}; // empty array (0 cells)
new globalEntries[][3] = {
    "a", "bc", "d"
};

Test:Run() {
    new def[16]; // default behavior, just put everything into dest
    new localEntries[][] = {
        "a", "bc", "d"
    }; // oder must be reversed, entries first dest second
    new local[1]; // local array need to have at least 1 cell

    ASSERT(strjoin(def, localEntries, ", ") == 7); // 7 unused cells remaining
    ASSERT(strcmp(def, "a, bc, d", false) == 0);

    ASSERT(strjoin(local, localEntries, ", ", cellmax) == 2); // 2 unused cells remaining
    ASSERT(strcmp(local, "a, bc, d", false) == 0);

    ASSERT(strjoin(global, globalEntries, ", ", cellmax) == 1); // 1 unused cell remaining
    ASSERT(strcmp(global, "a, bc, d", false) == 0);
}