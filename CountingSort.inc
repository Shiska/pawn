stock bool: CountingSort(array[], size = sizeof array) {
    new
        i,
        val,
        low = array[0],
        high = low;

    for(i = size; --i >= 1; ) {
        val = array[i];

        if(val < low) low = val;
        else if(val > high) high = val;
    }
    val = high - low;

    #emit stack 0 // alt = stack
    #emit lctrl 2 // pri = heap
    #emit sub.alt
    #emit shr.c.pri 2
    #emit stor.s.pri high

    if(val <= high) { // should work by default up to a range of 4000, could be increased with #pragma dynamic
        #emit lctrl 1 // dat
        #emit move.alt
        #emit lctrl 0 // cod
        #emit sub
        #emit move.alt
        #emit lctrl 6 // cip
        #emit add
        #emit add.c 80
        #emit stor.s.pri high
        #emit load.s.pri val
        #emit add.c 1
        #emit shl.c.pri 2
        #emit sref.s.pri high
        #emit lctrl 2 // heap
        #emit sctrl 4 // stack
        #emit move.alt
        #emit zero.pri
        #emit fill 0 // fills stack with zeros
        #emit load.s.pri low
        #emit shl.c.pri 2
        #emit sub.alt // heap - low, so the lowest value is at position 0
        #emit stor.s.pri high // new high[] = {0, ...}

        for(i = size; --i >= 0; ) {
            #emit load.s.pri i
            #emit load.s.alt array
            #emit idxaddr
            #emit load.i
            #emit load.s.alt high
            #emit idxaddr
            #emit inc.i // high[array[i]]++
        }
        for(i = 0; val-- >= 0; ++low) {
            #emit load.s.alt high
            #emit load.s.pri low
            #emit idxaddr
            #emit load.i
            #emit stor.s.pri size // size = high[low]

            while(size--) {
                array[i++] = low;
            }
        } {}
        #emit lctrl 5 // frame
        #emit sctrl 4 // stack
        #emit retn // frame is never 0 so it represents bool true
    }
    return false; // can't be done
}