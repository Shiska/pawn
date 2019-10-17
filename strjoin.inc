#include <string>

stock strjoin(dest[], const entries[][], const seperator[] = "", const buffer = sizeof dest, count = sizeof entries) {
    /// <summary>Joins the string in lines together seperated by seperator</summary>
    /// <param name="dest">Dest array</param>
    /// <param name="entries">Source array</param>
    /// <param name="seperator">Seperator to join the strings from entries</param>
    /// <param name="buffer">By default size of dest but can be set it to cellmax, so it will overwrite adjacent memory, see example</param>
    /// <param name="count">Entry count, by default sizeof entries</param>
    /// <example>
    /// // To join constant global strings together and overwrite the source array use it like that
    /// new dest[] = {}; // empty array (0 cells), increase the size if strjoin fails due to insufficient memory
    /// new entries[][] = { // entries needs to be right after dest, so it gets overwritten
    ///     "a", "b", "c"
    /// };
    /// strjoin(dest, entries, ", ", cellmax); // strjoin will return the amout of insufficient memory as negative value
    ///</example>
    /// <example>
    /// // Otherwise the normal usage
    /// new dest[8]; // doesn't need to be right after but you need to increase the buffer to match the output
    /// new entries[][] = {
    ///     "a", "b", "c"
    /// };
    /// strjoin(dest, entries, ", "); // strjoin will return the amout of insufficient memory as negative value
    ///</example>
    /// <returns>Excess memory space in cells, 0 = perfect match, negative values are insufficient cells</returns>

    new memory;

    if(buffer == cellmax) { // lets try to overwrite the entries array
        #emit load.s.pri entries
        #emit load.s.alt dest
        #emit sub
        #emit stor.s.pri memory

        new sepLength = strlen(seperator); // this calculation is wrong if someone specifies the second dimension, aka wasting memory
        memory = (memory >> 2) + (count << 1) - sepLength * (count - 1) - 1;

        if(memory < 0) { // insufficient memory, increase buffer
            dest[0] = EOS;
        } else {
            new length = count;
            // push entries last to first
            while(length--) { // because we will overwrite the indices
                #emit load.s.alt entries
                #emit load.s.pri length
                #emit idxaddr
                #emit move.alt
                #emit load.i
                #emit add // dest
                #emit push.pri
            }
            sepLength <<= 2;

            #emit load.s.pri seperator
            #emit stor.s.pri entries

            while(--count) { // pop entries, frist to last
                #emit pop.pri
                #emit stor.s.pri seperator

                length = strlen(seperator) << 2;
                // memcpy fails with cellmax as maxlength so I push length twice
                memcpy(dest, seperator, 0, length, length);

                #emit load.s.pri dest
                #emit load.s.alt length
                #emit add
                #emit stor.s.pri dest
                #emit load.s.pri entries
                #emit stor.s.pri seperator

                memcpy(dest, seperator, 0, sepLength, sepLength);

                #emit load.s.pri dest
                #emit load.s.alt sepLength
                #emit add
                #emit stor.s.pri dest
            }
            if(memory == 0) { // perfect match, last line is already at the correct position
                #emit pop.pri // just free entry
            } else { // imperfect match, shift last line forward but creates unused cells
                #emit pop.pri // lower allocation if possible (only if you used 3 or more character as seperator)
                #emit stor.s.pri seperator

                dest[0] = EOS;
                // strcat because we don't know the overall size, also to finish the string with EOS
                strcat(dest, seperator, cellmax);
            }
        }
    } else { // put everything into dest
        new added;

        dest[0] = EOS;
        memory = count;

        while(memory--) { // because we will overwrite the indices
            #emit load.s.alt entries
            #emit load.s.pri memory
            #emit idxaddr
            #emit move.alt
            #emit load.i
            #emit add // dest
            #emit push.pri
            #emit push.s seperator
        }
        #emit pop.pri // ignore first seperator

        memory = buffer - 1; // 1 cell is taken by EOS
        count = count * 2 - 1;

        while(count--) {
            #emit pop.pri
            #emit stor.s.pri seperator

            memory -= (added = strcat(dest, seperator, buffer));

            if(memory == 0) { // no more memory available
                memory = added - strlen(seperator); // subtract missing characters

                while(count--) { // count missing space
                    #emit pop.pri
                    #emit stor.s.pri seperator

                    memory -= strlen(seperator);
                }
                break;
            }
        }
    }
    return memory;
}