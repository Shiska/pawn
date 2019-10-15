#include <string>

// Joins line array together seperated by 'seperator' and moves everything to base address
// Note: Everything between the base address and the lines array will be deleted and used as buffer
//
// Therefor use have to define it like that if your seperator uses 2 or less characters
// new base[] = {}; // empty array (0 cells)
// new lines[][] = {
//     "a", "b", "c"
// };
// strjoin(base, lines, ", ")
//
// If three or more characters you need to increase the base buffer to match the memory usage returned by strjoin
// new base[NEGATIVE_RETURN_VALUE_OF_STRJOIN]; // in this case with 3 charcters as seperator and three lines strjoin would return -1, so you need at least 1 additional cell
// Note: Local arrays can't be empty and must have at least 1 cell, also the order must be reversed, see test.pwn for an example
stock strjoin(base[], lines[][], const seperator[] = "", size = sizeof lines) {
    // returns excess memory space in cells, 0 = perfect match
    // if negative the function failed due to lack of memory, increase base buffer
    // if positive decrease base buffer (only possible with 3 or more characters as seperator otherwise buffer should already be 0)

    new memory;

    #emit load.s.pri lines
    #emit load.s.alt base
    #emit sub // memory offset
    #emit stor.s.pri memory

    new sepLength = strlen(seperator);

    memory = (memory >> 2) + (size << 1) - sepLength * (size - 1) - 1;

    if(memory < 0) { // insufficient memory, increase buffer
        base[0] = EOS;
    } else {
        new length;
        new i = size;
        // push entries last to first
        while(i--) { // because we will overwrite the indices
            #emit load.s.alt lines
            #emit load.s.pri i
            #emit idxaddr
            #emit move.alt
            #emit load.i
            #emit add // dest
            #emit push.pri
        }
        sepLength <<= 2;

        #emit load.s.pri seperator
        #emit stor.s.pri lines

        while(--size) { // pop entries, frist to last
            #emit pop.pri
            #emit stor.s.pri seperator
            #emit pop.alt
            #emit sub.alt
            #emit add.c 0xFFFFFFFC // -4
            #emit stor.s.pri length
            #emit push.alt
            // memcpy fails with cellmax as maxlength so I push length twice
            memcpy(base, seperator, 0, length, length);

            #emit load.s.pri base
            #emit load.s.alt length
            #emit add
            #emit stor.s.pri base
            #emit load.s.pri lines
            #emit stor.s.pri seperator

            memcpy(base, seperator, 0, sepLength, sepLength);

            #emit load.s.pri base
            #emit load.s.alt sepLength
            #emit add
            #emit stor.s.pri base
        }
        if(memory == 0) { // perfect match, last line is already at the correct position
            #emit pop.pri // just free entry
        } else { // imperfect match, shift last line forward but creates unused cells
            #emit pop.pri // lower allocation if possible (only if you used 3 or more character as seperator)
            #emit stor.s.pri seperator

            base[0] = EOS;
            // strcat because we don't know the overall size, also to finish the string with EOS
            strcat(base, seperator, cellmax);
        }
    }
    return memory;
}