#if 0
    #define debug printf
#else
    #define debug(%0);
    #define BigIntPrint(%0);
#endif

// always use cBigInt for variables otherwise you could overwrite an initialized variable
// resulting that the deconstructor, for the overwritten variable, isn't called anymore, thus creating a leak in the heap
#define cBigInt: const BigInt:

stock raw_memcpy(dest, const source, index, numbytes, maxlength) {
    #pragma unused dest, source, index, numbytes, maxlength

    debug("raw_memcpy(%d, %d, %d, %d, %d)", dest, source, index, numbytes, maxlength);

    #emit stack 8
    #emit sysreq.c memcpy
    #emit stack -8
    #emit retn

    return 0;
}

stock raw_memset(address, numbytes, value) { 
    debug("raw_memset(%d, %d, %d)", address, numbytes, value);
    // the "fill" opcode only fills to the next smaller multiple of the cell size, fill the remaining bytes manually with strb.i
    switch(numbytes & 3) {
        case 1: {
            #emit load.s.pri address
            #emit load.s.alt numbytes
            #emit add
            #emit add.c -1
            #emit move.alt
            #emit load.s.pri value
            #emit strb.i 1
        }
        case 2: {
            #emit load.s.pri address
            #emit load.s.alt numbytes
            #emit add
            #emit add.c -2
            #emit move.alt
            #emit load.s.pri value
            #emit strb.i 2
        }
        case 3: {
            #emit load.s.pri address
            #emit load.s.alt numbytes
            #emit add
            #emit add.c -3
            #emit move.alt
            #emit load.s.pri value
            #emit strb.i 2 // strb.i can only write 1/2 or 4 bytes
            #emit xchg
            #emit add.c 2
            #emit xchg
            #emit shr.c.pri 16
            #emit strb.i 1
        }
    } {}
    #emit lctrl 6
    #emit move.alt
    #emit lctrl 0
    #emit add
    #emit move.alt
    #emit lctrl 1
    #emit sub.alt
    #emit add.c 84 // 21 * 4
    #emit load.s.alt address
    #emit stor.s.pri address
    #emit load.s.pri numbytes
    #emit sref.s.pri address
    #emit load.s.pri value
    #emit fill 0
    #emit retn

    return 0;
}

stock raw_memcmp(address1, address2, numbytes) { // returns -1, 0, 1
    debug("raw_memcmp(%d, %d, %d)", address1, address2, numbytes);

    #emit lctrl 6
    #emit move.alt
    #emit lctrl 0
    #emit add
    #emit move.alt
    #emit lctrl 1
    #emit sub.alt
    #emit add.c 84 // 21 * 4
    #emit load.s.alt address1
    #emit stor.s.pri address1
    #emit load.s.pri numbytes
    #emit sref.s.pri address1
    #emit load.s.pri address2
    #emit cmps 0
    #emit retn

    return 0;
}

stock HeapAlloc(bytes) {
    debug("HeapAlloc(%d)", bytes);

    #emit lctrl 2
    #emit move.alt
    #emit load.s.pri bytes
    #emit add
    #emit sctrl 2
    #emit move.pri
    #emit retn

    return 0;
}

stock HeapFree(bytes) {
    debug("HeapFree(%d)", bytes);

    #emit lctrl 2
    #emit load.s.alt bytes
    #emit sub
    #emit sctrl 2
    #emit retn

    return 0;
}

stock BigIntAlloc(size) {
    debug("BigIntAlloc(%d)", size);

    if(size > 0) {
        new bytes = (size + 1) << 1;
        new address = HeapAlloc(bytes);

        raw_memset(address, bytes, 0);
        BigIntSetSize(address, size);

        return address;
    }
    return 0;
}

// Note: Only works on the last created BigInt
stock BigIntRealloc(address, newsize) {
    debug("BigIntRealloc(%d, %d)", address, newsize);

    if(address && newsize > 0) {
        new size = BigIntGetSize(address);
        new offset = newsize - size;

        if(offset) {
            if(offset < 0) {
                HeapFree((-offset) << 1);
            } else {
                raw_memset(HeapAlloc(offset << 1), offset << 1, 0);
            }
            return BigIntSetSize(address, newsize);
        }
    }
    return 0;
}

stock BigIntCopy(address, alloc = 0) {
    debug("BigIntCopy(%d, %d)", address, alloc);

    if(address) {
        new size = BigIntGetSize(address);
        new bytes = ((alloc += size) + 1) << 1;
        new copy = HeapAlloc(bytes);

        raw_memset(copy, bytes, 0);
        raw_memcpy(copy, address, 0, (size + 1) << 1, size);

        BigIntSetSize(copy, alloc);

        return copy;
    }
    return 0;
}

stock BigInt(integer, bool: unsigned = false) { // TODO : finish chaing size to 16 bit words from cells
    debug("BigInt(%d, %d)", integer, unsigned);

    new size, int;

    if(unsigned) {
        int = BigIntAlloc((size = (integer & 0xFFFF0000) ? 2 : 1));
    } else {
        new bool: isneg = bool: (integer >>> 31);

        if(isneg) {
            integer *= -1;
        }
        int = BigIntAlloc((size = (integer & 0xFFFF0000) ? 2 : 1));

        if(isneg) {
            BigIntNeg(int);
        }
    } {}
    #emit load.s.pri int
    #emit add.c 2
    #emit move.alt
    #emit load.s.pri integer
    #emit strb.i 2

    if(size == 2) {
        #emit load.s.pri int
        #emit add.c 4
        #emit move.alt
        #emit load.s.pri integer
        #emit shr.c.pri 16
        #emit strb.i 2
    }
    BigIntPrint(int);

    return int;
}

stock BigIntFree(address) {
    debug("BigIntFree(%d)", address);

    if(address) {
        #emit load.s.pri address
        #emit sctrl 2
        #emit retn
    }
    return false;
}
// Note: Moves bigint to dest and frees everything behind it!!!
// Only use this function with BigIntResult
stock BigIntMov(dest, src) {
    debug("BigIntMov(%d, %d)", dest, src);

    if(dest && src) {
        new size = BigIntGetSize(src);
        new bytes = (size + 1) << 1;

        raw_memcpy(dest, src, 0, bytes, size);

        #emit load.s.pri dest
        #emit load.s.alt bytes
        #emit add
        #emit sctrl 2 // heap

        return dest;
    }
    return 0;
}

stock BigIntZero(& zero) { // uses a local variable as storage
    debug("BigIntZero(%d)", zero);

    new addr;

    zero = 0;

    #emit load.s.pri zero
    #emit stor.s.pri addr

    BigIntSetSize(addr, 1);

    return addr;
}

enum BigIntHidden {
    bool: eBigIntNeg, // Negative
    bool: eBigIntInv, // Inverse
}

stock BigIntSetSize(address, cells) { // size = number of 16 bit words
    debug("BigIntSetSize(%d, %d)", address, cells);

    if(address) {
        const mask = (1 << _: BigIntHidden) - 1;

        #emit load.s.pri address
        #emit lodb.i 2
        #emit const.alt mask
        #emit and
        #emit move.alt
        #emit load.s.pri cells
        #emit shl.c.pri BigIntHidden
        #emit or
        #emit load.s.alt address
        #emit strb.i 2
        #emit move.pri
        #emit retn
    }
    return 0;
}

stock BigIntGetSize(address) { // size = number of 16 bit words
    debug("BigIntGetSize(%d)", address);

    if(address) {
        #emit load.s.pri address
        #emit lodb.i 2
        #emit shr.c.pri BigIntHidden
        #emit retn
    }
    return 0;
}

stock BigIntNeg(address) {
    debug("BigIntNeg(%d)", address);

    if(address) {
        const bit = 1 << _: eBigIntNeg;

        #emit load.s.pri address
        #emit lodb.i 2
        #emit const.alt bit
        #emit xor
        #emit load.s.alt address
        #emit strb.i 2
        #emit move.pri
        #emit retn
    }
    return 0;
}

stock BigIntSetNeg(address, bool: neg) {
    debug("BigIntSetNeg(%d, %d)", address, neg);

    if(address) {
        const bit = 1 << _: eBigIntNeg;
        const mask = ~bit;

        if(neg) {
            #emit load.s.pri address
            #emit lodb.i 2
            #emit const.alt bit
            #emit or
            #emit load.s.alt address
            #emit strb.i 2
            #emit move.pri
            #emit retn
        } {}
        #emit load.s.pri address
        #emit lodb.i 2
        #emit const.alt mask
        #emit and
        #emit load.s.alt address
        #emit strb.i 2
        #emit move.pri
        #emit retn
    }
    return 0;
}

stock bool: BigIntIsNeg(address) {
    debug("BigIntIsNeg(%d)", address);

    if(address) {
        const bit = 1 << _: eBigIntNeg;

        #emit load.s.pri address
        #emit lodb.i 2
        #emit const.alt bit
        #emit and
        #emit retn
    }
    return false;
}

stock BigIntInv(address) {
    debug("BigIntInv(%d)", address);

    if(address) {
        const bit = 1 << _: eBigIntInv;

        #emit load.s.pri address
        #emit lodb.i 2
        #emit const.alt bit
        #emit xor
        #emit load.s.alt address
        #emit strb.i 2
        #emit move.pri
        #emit retn
    }
    return 0;
}

stock BigIntSetInv(address, bool: inv) {
    debug("BigIntSetInv(%d, %d)", address, inv);

    if(address) {
        const bit = 1 << _: eBigIntInv;
        const mask = ~bit;

        if(inv) {
            #emit load.s.pri address
            #emit lodb.i 2
            #emit const.alt bit
            #emit or
            #emit load.s.alt address
            #emit strb.i 2
            #emit move.pri
            #emit retn
        } {}
        #emit load.s.pri address
        #emit lodb.i 2
        #emit const.alt mask
        #emit and
        #emit load.s.alt address
        #emit strb.i 2
        #emit move.pri
        #emit retn
    }
    return 0;
}

stock bool: BigIntIsInv(address) {
    debug("BigIntIsInv(%d)", address);

    if(address) {
        const bit = 1 << _: eBigIntInv;

        #emit load.s.pri address
        #emit lodb.i 2
        #emit const.alt bit
        #emit and
        #emit retn
    }
    return false;
}

// removes empty cells at the end
// Warning: only use it on the last created BigInt otherwise this could free other data
stock BigIntTrim(address) {
    debug("BigIntTrim(%d)", address);

    if(address) {
        new size = BigIntGetSize(address), value, trim;
        new addr = address + ((size + 1) << 1);
        // check for empty cells
        do {
            #emit load.s.pri addr
            #emit add.c -2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit stor.s.pri value

            debug("    addr %d value %d", addr, value);
        } while(!value && ++trim < size);

        if(trim) {
            return BigIntRealloc(address, size - trim);
        }
        return address;
    }
    return 0;
}

stock BigIntPrint(address) {
    debug("BigIntPrint(%d)", address);

    if(address) {
        new size = BigIntGetSize(address);

        printf("    size %d isneg %d isinv %d", size, BigIntIsNeg(address), BigIntIsInv(address));

        new addr = address + ((size + 1) << 1), value;

        while(size--) {
            #emit load.s.pri addr
            #emit add.c -2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit stor.s.pri value

            printf("    value %5d %016b", value, value);
        }
        return address;
    }
    return 0;
}

stock BigIntNegInternal(oper) { // inplace
    debug("BigIntNegInternal(%d)", oper);

    if(oper) {
        new size = BigIntGetSize(oper);
        new addr = oper;
        new value;

        BigIntPrint(oper);
        // negative until value is not 0
        while(!value && size) {
            #emit load.s.pri addr
            #emit add.c 2
            #emit stor.s.pri addr
            #emit move.alt
            #emit lodb.i 2
            #emit neg
            #emit strb.i 2
            #emit stor.s.pri value
            #emit dec.s size
        }
        // invert remaining bytes
        while(size) {
            #emit load.s.pri addr
            #emit add.c 2
            #emit stor.s.pri addr
            #emit move.alt
            #emit lodb.i 2
            #emit invert
            #emit strb.i 2
            #emit dec.s size
        }
        BigIntPrint(oper);

        return oper;
    }
    return 0;
}

stock BigIntAdd(oper1, oper2) {
    debug("BigIntAdd(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new bool: isneg1 = BigIntIsNeg(oper1);
        new bool: isneg2 = BigIntIsNeg(oper2);

        if(isneg1 ^ isneg2) { // one operator is negative
            if(isneg1) { // switch to sub because we store everything as unsigned
                return BigIntSubInternal(oper2, oper1);
            }
            return BigIntSubInternal(oper1, oper2);
        }
        return BigIntAddInternal(oper1, oper2);
    }
    return 0;
}

stock BigIntSub(oper1, oper2) {
    debug("BigIntSub(%d, %d)", oper1, oper2);

    if(oper1 && oper2) { // same as BigIntAdd, except checks are inverted because isneg2 is mentally inverted due to subbing
        new bool: isneg1 = BigIntIsNeg(oper1);
        new bool: isneg2 = BigIntIsNeg(oper2);

        if(isneg1 ^ isneg2) { // two positive or two negative
            return BigIntAddInternal(oper1, oper2); // works because result tag depends on oper1
        }
        if(isneg1) {
            return BigIntSubInternal(oper2, oper1);
        }
        return BigIntSubInternal(oper1, oper2);
    }
    return 0;
}

// gets called for two positive or two negative numbers
stock BigIntAddInternal(oper1, oper2) {
    debug("BigIntAddInternal(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);

        new bool: isneg = BigIntIsNeg(oper1); // get it before swapping
        // switch operators if oper1 is smaller than oper2
        if(size1 < size2) {
            // switch sizes
            #emit load.s.pri size1
            #emit load.s.alt size2
            #emit stor.s.pri size2
            #emit stor.s.alt size1
            // switch operators
            #emit load.s.pri oper1
            #emit load.s.alt oper2
            #emit stor.s.pri oper2
            #emit stor.s.alt oper1
        }
        new result = BigIntAlloc(size1);
        new addr = result + 2;
        new carry;
        new i;

        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);
        // add both values together
        while(i < size2) {
            #emit load.s.pri oper1
            #emit add.c 2
            #emit stor.s.pri oper1
            #emit lodb.i 2
            #emit move.alt
            #emit load.s.pri oper2
            #emit add.c 2
            #emit stor.s.pri oper2
            #emit lodb.i 2
            #emit add
            #emit load.s.alt carry
            #emit add
            #emit load.s.alt addr
            #emit strb.i 2
            #emit shr.c.pri 16
            #emit stor.s.pri carry
            #emit const.pri 2
            #emit add
            #emit stor.s.pri addr
            #emit inc.s i
        }
        // add missing bytes from bigger operator
        while(i < size1) {
            #emit load.s.pri oper1
            #emit add.c 2
            #emit stor.s.pri oper1
            #emit lodb.i 2
            #emit load.s.alt carry
            #emit add
            #emit load.s.alt addr
            #emit strb.i 2
            #emit shr.c.pri 16
            #emit stor.s.pri carry
            #emit const.pri 2
            #emit add
            #emit stor.s.pri addr
            #emit inc.s i
        }
        if(carry) { // allocate additional cell
            debug("Allocating additional cell - %d", carry);

            BigIntRealloc(result, size1 + 1);

            #emit load.s.pri carry
            #emit load.s.alt addr
            #emit strb.i 2
        }
        if(isneg) { // if any oper is negative result is negative
            BigIntNeg(result);
        }
        BigIntPrint(result);

        return result;
    }
    return 0;
}

// gets called for one positive (oper1) and one negative (oper2) number
stock BigIntSubInternal(oper1, oper2) {
    debug("BigIntSubInternal(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);
        new size, addr, result, carry;
        // copy smaller operator into result to match sizes
        if(size1 < size2) {
            addr = oper1 = result = BigIntCopy(oper1, (size = size2) - size1);
        } else {
            addr = oper2 = result = BigIntCopy(oper2, (size = size1) - size2);
        }
        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);
        // substract cells
        while(size) {
            #emit load.s.pri oper1
            #emit add.c 2
            #emit stor.s.pri oper1
            #emit lodb.i 2
            #emit move.alt
            #emit load.s.pri oper2
            #emit add.c 2
            #emit stor.s.pri oper2
            #emit lodb.i 2
            #emit sub.alt
            #emit load.s.alt carry
            #emit sub
            #emit move.alt
            #emit load.s.pri addr
            #emit add.c 2
            #emit stor.s.pri addr
            #emit xchg
            #emit strb.i 2
            #emit shr.c.pri 31
            #emit stor.s.pri carry
            #emit dec.s size

            debug("    subbing addresses (%d + %d)", oper1, oper2);
            BigIntPrint(result);
        }
        if(carry) {
            BigIntNegInternal(result);
            BigIntSetNeg(result, true);
        } else {
            BigIntSetNeg(result, false);
        }
        BigIntPrint(result);
        BigIntTrim(result);
        BigIntPrint(result);

        return result;
    }
    return 0;
}

stock BigIntInc(oper) {
    debug("BigIntInc(%d)", oper);

    if(oper) {
        if(BigIntIsNeg(oper)) {
            return BigIntDecInternal(oper);
        }
        return BigIntIncInternal(oper);
    }
    return 0;
}

stock BigIntDec(oper) {
    debug("BigIntDec(%d)", oper);

    if(oper) {
        if(BigIntIsNeg(oper)) {
            return BigIntIncInternal(oper);
        }
        return BigIntDecInternal(oper);
    }
    return 0;
}

stock BigIntIncInternal(oper) { // inplace
    debug("BigIntIncInternal(%d)", oper);

    if(oper) {
        new size = BigIntGetSize(oper);
        new addr = oper;
        new carry = 1;

        while(carry && size) {
            #emit load.s.pri addr
            #emit add.c 2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit load.s.alt carry
            #emit add
            #emit load.s.alt addr
            #emit strb.i 2
            #emit shr.c.pri 16
            #emit stor.s.pri carry
            #emit dec.s size
        }
        return oper;
    }
    return 0;
}

stock BigIntDecInternal(oper) { // inplace
    debug("BigIntDecInternal(%d)", oper);

    if(oper) {
        new size = BigIntGetSize(oper);
        new addr = oper;
        new carry = 1;

        while(carry && size) {
            #emit load.s.pri addr
            #emit add.c 2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit load.s.alt carry
            #emit sub
            #emit load.s.alt addr
            #emit strb.i 2
            #emit shr.c.pri 31
            #emit stor.s.pri carry
            #emit dec.s size
        }
        if(carry) {
            BigIntNegInternal(oper);
            BigIntNeg(oper);
        }
        return oper;
    }
    return 0;
}

// https://lemire.me/blog/2017/09/18/computing-the-inverse-of-odd-integers/
stock BigIntInvInternal(oper) { // module inverse - only works on odd numbers because the number needs to be coprime to the modulo
    debug("BigIntInvInternal(%d)", oper); // unused

    if(oper) {
        new size = BigIntGetSize(oper);
        new bits = size << 4;
        new cbits = 3;

        new y = BigIntCopy(oper);
        new square = BigIntAlloc(size);

        new yaddr = y + 2;
        new xaddr = oper + 2;
        new saddr = square + 2;

        new laddr, raddr, daddr;
        new sum, carry, mod;
        new left, right;
        new i, j;

        BigIntPrint(oper);

        while(cbits < bits) {
            // y * y
            laddr = raddr = yaddr;
            daddr = saddr;

            for(sum = i = 0; i != size; ++i) {
                right = raddr;
                left = laddr;
                carry = 0;

                while(left < right) {
                    #emit load.s.pri left
                    #emit lodb.i 2
                    #emit move.alt
                    #emit load.s.pri right
                    #emit lodb.i 2
                    #emit umul
                    #emit addr.alt mod
                    #emit strb.i 2
                    #emit shr.c.pri 16
                    #emit load.s.alt carry
                    #emit add
                    #emit stor.s.pri carry

                    sum += mod << 1;

                    #emit dec.s right
                    #emit dec.s right
                    #emit inc.s left
                    #emit inc.s left
                }
                carry <<= 1;

                if(left == right) {
                    #emit load.s.pri left
                    #emit lodb.i 2
                    #emit move.alt
                    #emit umul
                    #emit addr.alt mod
                    #emit strb.i 2
                    #emit shr.c.pri 16
                    #emit load.s.alt carry
                    #emit add
                    #emit stor.s.pri carry

                    sum += mod;
                } {}
                #emit load.s.pri sum
                #emit load.s.alt daddr
                #emit strb.i 2
                #emit shr.c.pri 16
                #emit load.s.alt carry
                #emit add
                #emit stor.s.pri sum

                daddr += 2;
                raddr += 2;
            }
            BigIntPrint(square);
            // (y << 1) - (y * y * x)
            laddr = saddr;
            raddr = xaddr;
            daddr = yaddr;
            
            for(sum = i = 0; i++ != size; ) {
                right = raddr;
                left = laddr;
                carry = 0;
                j = i;
                // sum + carry = y * y * x
                while(j--) {
                    #emit load.s.pri left
                    #emit lodb.i 2
                    #emit move.alt
                    #emit load.s.pri right
                    #emit lodb.i 2
                    #emit umul
                    #emit addr.alt mod
                    #emit strb.i 2
                    #emit shr.c.pri 16
                    #emit load.s.alt carry
                    #emit add
                    #emit stor.s.pri carry

                    sum += mod;

                    #emit dec.s right
                    #emit dec.s right
                    #emit inc.s left
                    #emit inc.s left
                }
                // (y << 1) - sum
                #emit load.s.pri daddr
                #emit lodb.i 2
                #emit shl.c.pri 1
                #emit load.s.alt sum
                #emit sub
                #emit load.s.alt daddr
                #emit strb.i 2
                #emit shr.c.pri 31
                #emit load.s.alt carry
                #emit sub.alt
                #emit stor.s.pri sum

                daddr += 2;
                raddr += 2;
            }
            BigIntPrint(y);
 
            cbits <<= 1;
        }
        BigIntFree(square);

        return y;
    }
    return 0;
}

// squaring by reusing duplicates terms
// it seems to be slower than BigIntMul if the number is smaller than ~128 bits
stock BigIntSquare(oper) {
    debug("BigIntSquare(%d)", oper);

    if(oper) {
        new size = BigIntGetSize(oper);
        // special case for size 1
        if(size == 1) {
            #emit load.s.pri oper
            #emit add.c 2
            #emit lodb.i 2
            #emit move.alt
            #emit umul
            #emit stor.s.pri size

            return BigInt(size, true);
        }
        new result = BigIntAlloc((size <<= 1));

        BigIntPrint(_: oper);

        new maddr = oper + size; // last cell
        new daddr = result + 2; // dest addr
        new laddr = oper + 2; // left addr
        new raddr = laddr; // right addr
        new carry, sum, left, right, mod;

        while(--size) {
            right = raddr;
            left = laddr;
            carry = 0;
            sum = 0;

            while(left < right) {
                #emit load.s.pri left
                #emit lodb.i 2
                #emit move.alt
                #emit load.s.pri right
                #emit lodb.i 2
                #emit umul
                #emit addr.alt mod
                #emit strb.i 2
                #emit shr.c.pri 16
                #emit load.s.alt carry
                #emit add
                #emit stor.s.pri carry

                sum += mod;

                #emit dec.s right
                #emit dec.s right
                #emit inc.s left
                #emit inc.s left
            }
            // dublicate the terms
            carry <<= 1;
            sum <<= 1;

            if(left == right) {
                #emit load.s.pri left
                #emit lodb.i 2
                #emit move.alt
                #emit umul
                #emit addr.alt mod
                #emit strb.i 2
                #emit shr.c.pri 16
                #emit load.s.alt carry
                #emit add
                #emit stor.s.pri carry

                sum += mod;
            } {}
            #emit load.s.pri daddr
            #emit lodb.i 2
            #emit load.s.alt sum
            #emit add
            #emit load.s.alt daddr
            #emit strb.i 2
            #emit shr.c.pri 16
            #emit load.s.alt carry
            #emit add
            #emit stor.s.pri carry

            daddr += 2;

            if(carry) {
                #emit load.s.pri daddr
                #emit lodb.i 2
                #emit load.s.alt carry
                #emit add
                #emit load.s.alt daddr
                #emit strb.i 2
                #emit shr.c.pri 16
                #emit stor.s.pri carry

                if(carry) { // if there is still a carry add it to the next higher cell
                    #emit load.s.pri daddr
                    #emit add.c 2
                    #emit move.alt
                    #emit load.s.pri carry
                    #emit strb.i 2
                }
            }
            if(raddr == maddr) {
                laddr += 2;
            } else {
                raddr += 2;
            }
        }
        BigIntPrint(result);
        BigIntTrim(result);
        BigIntPrint(result);

        return result;
    }
    return 0;
}

stock BigIntMul(oper1, oper2) {
    debug("BigIntMul(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        if(oper1 == oper2) { // only checking if address match, not if the value is the same
            #emit const.pri BigIntSquare // unlucky if two different variables have the same value, they need to take the long way
            #emit add.c 4
            #emit sctrl 6
        }
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);
        // special case for size 1
        if(size1 == 1 && size2 == 1) {
            #emit load.s.pri oper1
            #emit add.c 2
            #emit lodb.i 2
            #emit move.alt
            #emit load.s.pri oper2
            #emit add.c 2
            #emit lodb.i 2
            #emit umul
            #emit stor.s.pri size1

            return BigInt(size1, true);
        }
        // switch operators if oper1 is smaller than oper2
        if(size1 < size2) {
            // switch sizes
            #emit load.s.pri size1
            #emit load.s.alt size2
            #emit stor.s.pri size2
            #emit stor.s.alt size1
            // switch operators
            #emit load.s.pri oper1
            #emit load.s.alt oper2
            #emit stor.s.pri oper2
            #emit stor.s.alt oper1
        }
        new result = BigIntAlloc(size1 + size2);

        if(BigIntIsNeg(oper1) ^ BigIntIsNeg(oper2)) {
            BigIntNeg(result);
        }
        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);
        // long multiplication
        new carry, o2value;
        new raddr = result;
        new o1addr, o2addr = oper2;
        new i1, i2;

        BigIntPrint(result);

        for(i2 = 0; i2 != size2; ++i2) {
            #emit load.s.pri o2addr
            #emit add.c 2
            #emit stor.s.pri o2addr
            #emit lodb.i 2
            #emit stor.s.pri o2value

            carry = 0;
            o1addr = oper1;

            for(i1 = 0; i1 != size1; ++i1) { // result += carry + oper1 + oper2
                #emit load.s.pri o1addr
                #emit add.c 2
                #emit stor.s.pri o1addr
                #emit lodb.i 2
                #emit load.s.alt o2value
                #emit umul
                #emit move.alt
                #emit load.s.pri raddr
                #emit add.c 2
                #emit stor.s.pri raddr
                #emit lodb.i 2
                #emit add
                #emit load.s.alt carry
                #emit add
                #emit load.s.alt raddr
                #emit strb.i 2
                #emit shr.c.pri 16
                #emit stor.s.pri carry
            } {}
            #emit load.s.pri raddr
            #emit add.c 2
            #emit stor.s.pri raddr
            #emit lodb.i 2
            #emit load.s.alt carry
            #emit add
            #emit load.s.alt raddr
            #emit strb.i 2
            #emit load.s.pri size1
            #emit shl.c.pri 1
            #emit sub.alt
            #emit stor.s.pri raddr

            BigIntPrint(result);
        }
        BigIntPrint(result);
        BigIntTrim(result);
        BigIntPrint(result);

        return result;
    }
    return 0;
}
// due to the huge overheat this is quite slow compared to the basic implementation
// the main function isn't used in any operators but the internal as speical case for size 1
stock BigIntKaratsuba(oper1, oper2) {
    debug("BigIntKaratsuba(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        BigIntPrint(oper1);
        BigIntPrint(oper2);

        new result;

        #emit lctrl 2 // heap
        #emit stor.s.pri result

        // free everything at once instead at the end of each recursive call
        BigIntMov(result, BigIntKaratsubaInternal(oper1, oper2));

        if(BigIntIsNeg(oper1) ^ BigIntIsNeg(oper2)) {
            BigIntNeg(result);
        }
        BigIntPrint(result);
        BigIntTrim(result);
        BigIntPrint(result);

        return result;
    }
    return 0;
}

stock BigIntKaratsubaInternal(oper1, oper2) {
    debug("BigIntKaratsubaInternal(%d, %d)", oper1, oper2);

    new size1 = BigIntGetSize(oper1);
    new size2 = BigIntGetSize(oper2);

    oper1 += 2;
    oper2 += 2;

    if(size1 == 1 && size2 == 1) {
        #emit load.s.pri oper1
        #emit lodb.i 2
        #emit move.alt
        #emit load.s.pri oper2
        #emit lodb.i 2
        #emit umul
        #emit stor.s.pri size1

        return BigInt(size1, true);
    }
    new size = max(size1, size2) >>> 1;
    new bytes = size << 1;

    new osize1 = size1 - size;
    new osize2 = size2 - size;
    // split the integers into two parts
    new low1 = BigIntAlloc(size);
    new low2 = BigIntAlloc(size);
    new high1 = BigIntAlloc(osize1);
    new high2 = BigIntAlloc(osize2);

    raw_memcpy(low1, oper1, 2, bytes, bytes);
    raw_memcpy(low2, oper2, 2, bytes, bytes);
    raw_memcpy(high1, oper1 + bytes, 2, osize1 << 1, osize1);
    raw_memcpy(high2, oper2 + bytes, 2, osize2 << 1, osize2);

    new lh1 = BigIntSubInternal(low1, high1);
    new lh2 = BigIntSubInternal(high2, low2);
    // do it
    new z0 = BigIntKaratsubaInternal(low1, low2);
    new z1 = BigIntKaratsubaInternal(lh1, lh2);
    new z2 = BigIntKaratsubaInternal(high1, high2);
    // (z2 << (2 * lshift)) + ((z0 + z1 + z2) << lshift) + z0
    new lshift = size << 4; // shift result to the correct position
    // readd sign to unsigned multiplication
    if(BigIntIsNeg(lh1) ^ BigIntIsNeg(lh2)) {
        BigIntNeg(z1);
    }
    z1 = BigIntAdd(BigIntAdd(z0, z1), z2);
    z2 = BigIntCopy(z2, size);

    BigIntShiftLeft(z2, lshift);
    BigIntTrim(z2);

    z1 = BigIntAdd(z1, z2);

    BigIntRealloc(z1, BigIntGetSize(z1) + size);
    BigIntShiftLeft(z1, lshift);
    BigIntTrim(z1);

    return BigIntMov(low1, BigIntAdd(z0, z1));
}

stock BigIntPow(oper, pow) {
    debug("BigIntPow(%d, %d)", oper, pow);

    if(oper && pow > 0) {
        // if(pow < 0) {
        //     // oper = BigIntInv(oper);
        //     pow *= -1;
        // }
        if(pow == 0) {
            return 1;
        }
        BigIntPrint(oper);

        new dest;

        #emit lctrl 2
        #emit stor.s.pri dest
        // skip even numbers
        while(!(pow & 1)) {
            oper = BigIntSquare(oper);
            pow >>>= 1;
        }
        new result = oper;

        while((pow >>>= 1)) {
            oper = BigIntSquare(oper);

            if(pow & 1) {
                result = BigIntMul(oper, result);
            }
        }
        BigIntPrint(result);

        if(dest != result) {
            return BigIntMov(dest, result);
        }
        return dest;
    }
    return 0;
}

stock BigIntDiv(dividend, divisor, & quotient, & remainder) {
    debug("BigIntDiv(%d, %d, %d, %d)", dividend, divisor, quotient, remainder);

    if(dividend && divisor) {
        new sizeN = BigIntGetSize(dividend);
        new sizeD = BigIntGetSize(divisor);

        BigIntPrint(dividend);
        BigIntPrint(divisor);

        if((sizeN < sizeD) || ((sizeN == sizeD) && BigIntCmp(dividend, divisor) == -1)) {
            quotient = BigIntAlloc(1); // allocate empty quotient
            remainder = BigIntCopy(dividend);
        } else {
            new size = sizeN << 1;
            new raddr, daddr, carry, s;

            quotient = BigIntAlloc(sizeN);
            remainder = BigIntAlloc(size);
            // copy dividend without flags
            raw_memcpy(remainder, dividend + 2, 2, size, size);

            new div = BigIntAlloc(size);
            // copy divisor to shifted position
            raw_memcpy(div, divisor + 2, size + 2, sizeD << 1, size);

            debug("divisor");
            BigIntPrint(div);
            // long division
            for(new i = sizeN << 4; i != 0; --i) {
                BigIntShiftLeft(remainder, 1);
                BigIntShiftLeft(quotient, 1);

                if(BigIntCmp(remainder, div) != -1) {
                    raddr = remainder;
                    daddr = div;
                    carry = 0;
                    s = size;
                    // substract divisor from remainder inplace
                    while(s) {
                        #emit load.s.pri raddr
                        #emit add.c 2
                        #emit stor.s.pri raddr
                        #emit lodb.i 2
                        #emit move.alt
                        #emit load.s.pri daddr
                        #emit add.c 2
                        #emit stor.s.pri daddr
                        #emit lodb.i 2
                        #emit sub.alt
                        #emit load.s.alt carry
                        #emit sub
                        #emit load.s.alt raddr
                        #emit strb.i 2
                        #emit shr.c.pri 31
                        #emit stor.s.pri carry
                        #emit dec.s s
                    }
                    BigIntIncInternal(quotient);
                }
            }
            raddr = quotient + size + 2;
            // check for trimable cells in quotient
            do {
                #emit load.s.pri raddr
                #emit add.c -2
                #emit stor.s.pri raddr
                #emit lodb.i 2
                #emit stor.s.pri carry
            } while(!carry);
            // move remainder
            remainder = BigIntMov(raddr + 2, remainder);
            // correct size of quotient
            BigIntSetSize(quotient, (raddr - quotient) >> 1);
            // shift remainder downwards
            raw_memcpy(remainder, remainder + size + 2, 2, size, size);
            // correct size and free heap and trim
            BigIntSetSize(remainder, sizeN);
            BigIntTrim(remainder);
            HeapFree(size);
        }
        if(BigIntIsNeg(dividend) ^ BigIntIsNeg(divisor)) {
            BigIntNeg(quotient);
        }
        BigIntPrint(quotient);
        BigIntPrint(remainder);

        return quotient;
    }
    return false;
}

stock BigIntShiftLeft(oper, lshift) { // inplace
    debug("BigIntShiftLeft(%d, %d)", oper, lshift);

    if(oper && lshift > 0) {
        new offset = lshift >>> 4;
        new size = BigIntGetSize(oper);
        new addr = oper + ((size + 1) << 1);
        new rshift = 16 - (lshift &= 0xF);
        new tmp;

        BigIntPrint(oper);

        size -= offset;
        offset <<= 1;

        if(size >= 1) {
            while(--size) { // oper[i] = (oper[i - offset] << lshift) | (oper[i - offset - 1] >>> rshift);
                #emit load.s.pri addr
                #emit add.c -2
                #emit stor.s.pri addr
                #emit load.s.alt offset
                #emit sub
                #emit stor.s.pri tmp // first cell
                #emit lodb.i 2
                #emit load.s.alt lshift
                #emit shl
                #emit move.alt
                #emit load.s.pri tmp
                #emit stor.s.alt tmp // first result
                #emit add.c -2 // second cell
                #emit lodb.i 2
                #emit load.s.alt rshift
                #emit shr // second result
                #emit load.s.alt tmp
                #emit or
                #emit load.s.alt addr
                #emit strb.i 2
            } {}
            // last affected cell
            #emit load.s.pri addr
            #emit add.c -2
            #emit stor.s.pri addr
            #emit load.s.alt offset
            #emit sub
            #emit lodb.i 2
            #emit load.s.alt lshift
            #emit shl
            #emit load.s.alt addr
            #emit strb.i 2
        }
        if(offset) { // clear all remaining cells
            size += offset >>> 1;

            while(size--) {
                #emit load.s.pri addr
                #emit add.c -2
                #emit stor.s.pri addr
                #emit move.alt
                #emit zero.pri
                #emit strb.i 2
            }
        }
        BigIntPrint(oper);

        return oper;
    }
    return 0;
}

stock BigIntShiftRight(oper, rshift) { // inplace
    debug("BigIntShiftRight(%d, %d)", oper, rshift);

    if(oper && rshift > 0) {
        new offset = rshift >>> 4;
        new size = BigIntGetSize(oper);
        new lshift = 16 - (rshift &= 0xF);
        new addr = oper;
        new tmp;

        BigIntPrint(oper);

        size -= offset;
        offset <<= 1;

        if(size >= 1) {
            while(--size) { // oper[i] = (oper[i + offset] << lshift) | (oper[i + offset + 1] >>> rshift);
                #emit load.s.pri addr
                #emit add.c 2
                #emit stor.s.pri addr
                #emit load.s.alt offset
                #emit add
                #emit stor.s.pri tmp // first cell
                #emit lodb.i 2
                #emit load.s.alt rshift
                #emit shr
                #emit move.alt
                #emit load.s.pri tmp
                #emit stor.s.alt tmp // first result
                #emit add.c 2 // second cell
                #emit lodb.i 2
                #emit load.s.alt lshift
                #emit shl // second result
                #emit load.s.alt tmp
                #emit or
                #emit load.s.alt addr
                #emit strb.i 2
            } {}
            // last affected cell
            #emit load.s.pri addr
            #emit add.c 2
            #emit stor.s.pri addr
            #emit load.s.alt offset
            #emit add
            #emit lodb.i 2
            #emit load.s.alt rshift
            #emit shr
            #emit load.s.alt addr
            #emit strb.i 2
        }
        if(offset) { // clear all remaining cells
            size += offset >>> 1;

            while(size--) {
                #emit load.s.pri addr
                #emit add.c 2
                #emit stor.s.pri addr
                #emit move.alt
                #emit zero.pri
                #emit strb.i 2
            }
        }
        BigIntPrint(oper);

        return oper;
    }
    return 0;
}

stock BigIntCmp(oper1, oper2) { // -1, 0, 1 - oper1 is less than, equal or greater than
    debug("BigIntCmp(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new bool: isneg1 = BigIntIsNeg(oper1);
        new bool: isneg2 = BigIntIsNeg(oper2);
        // check sign
        if(isneg1 != isneg2) {
            return _: isneg2 - _: isneg1; // -1, 1
        }
        // check size if both negative or both positive
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);

        if(size1 != size2) { // clamp to ensure -1 and 1 instead of any negative / positive value
            return clamp(size1 - size2, -1, 1); // checking for -1 and 1 produces less opcode than checking for positive / negative
        }
        // compare bytes if size match
        if(isneg1) { // if both negative switch operators
            #emit load.s.pri oper1
            #emit load.s.alt oper2
            #emit stor.s.pri oper2
            #emit stor.s.alt oper1
        }
        // compare from highest to lowest
        size1 = (size1 + 1) << 1;

        oper1 += size1;
        oper2 += size1;

        size1 = 0;

        while(!size1 && size2) {
            #emit load.s.pri oper1
            #emit add.c -2
            #emit stor.s.pri oper1
            #emit lodb.i 2
            #emit move.alt
            #emit load.s.pri oper2
            #emit add.c -2
            #emit stor.s.pri oper2
            #emit lodb.i 2
            #emit sub.alt
            #emit stor.s.pri size1
            #emit dec.s size2
        }
        return clamp(size1, -1, 1);
    }
    return 0;
}

stock BigIntCmpR(oper1, BigIntResult: oper2) {
    new result = BigIntCmp(oper1, _: oper2);

    BigIntFree(_: oper2);

    return result;
}

stock BigIntCmpRR(BigIntResult: oper1, BigIntResult: oper2) {
    new result = BigIntCmp(_: oper1, _: oper2);

    BigIntFree(min(_: oper1, _: oper2));

    return result;
}

stock operator~(const BigInt: oper[], size) {
    #pragma unused size

    debug("operator~(BigInt: %d, %d)", _: oper[0], size);

    return BigIntFree(_: oper[0]);
}

stock BigInt: operator=(oper) {
    debug("BigInt: operator=(%d)", oper);

    return BigInt: BigInt(oper);
}

stock BigInt: operator=(BigIntResult: oper) {
    debug("BigInt: operator=(BigIntResult: %d)", _: oper);
    // pass through, just to avoid tag mistake
    return BigInt: oper;
}

stock BigInt: operator=(BigInt: oper) {
    debug("BigInt: operator=(BigInt: %d)", _: oper);

    BigIntCopy(_: oper);
    // avoid destructor of BigInt
    #emit retn

    return BigInt: 0;
}

stock BigIntResult: operator+(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator+(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), BigIntAdd(_: oper1, _: oper2));
}

stock BigIntResult: operator+(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator+(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper2, BigIntAdd(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigInt: oper1, BigInt: oper2) {
    debug("BigIntResult: operator+(BigInt: %d BigInt: %d)", _: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit const.pri BigIntAdd
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigInt: oper1, oper2) {
    debug("BigIntResult: operator+(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = BigInt(oper2);

    BigIntMov(oper2, BigIntAdd(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator+(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntAdd(_: oper1, BigInt(_: oper2)));
}

stock BigIntResult: operator-(BigIntResult: oper) {
    #pragma unused oper

    debug("BigIntResult: operator-(BigIntResult: %d)", _: oper);
    // faster than calling the function
    #emit const.pri BigIntNeg
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigInt: oper) {
    debug("BigIntResult: operator-(BigInt: %d)", _: oper);

    (_: oper) = BigIntCopy(_: oper);
    // avoid destructor of BigInt
    #emit const.pri BigIntNeg
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator-(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), BigIntSub(_: oper1, _: oper2));
}

stock BigIntResult: operator-(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator-(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper2, BigIntSub(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigIntResult: oper1, BigInt: oper2) {
    debug("BigIntResult: operator-(BigIntResult: %d BigInt: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper1, BigIntSub(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigInt: oper1, BigInt: oper2) {
    debug("BigIntResult: operator-(BigInt: %d BigInt: %d)", _: oper1, _: oper2);

    #emit const.pri BigIntSub
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigInt: oper1, oper2) {
    debug("BigIntResult: operator-(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = BigInt(oper2);

    BigIntMov(oper2, BigIntSub(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(oper1, BigInt: oper2) {
    debug("BigIntResult: operator-(%d, BigInt: %d)", _: oper1, _: oper2);

    oper1 = BigInt(oper1);

    BigIntMov(oper1, BigIntSub(oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator-(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntSub(_: oper1, BigInt(oper2)));
}

stock BigIntResult: operator-(oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator-(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper2, BigIntSub(BigInt(oper1), _: oper2));
}

stock BigIntResult: operator*(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator*(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), BigIntMul(_: oper1, _: oper2));
}

stock BigIntResult: operator*(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator*(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper2, BigIntMul(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator*(BigInt: oper1, BigInt: oper2) {
    debug("BigIntResult: operator*(BigInt: %d BigInt: %d)", _: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit const.pri BigIntMul
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator*(BigInt: oper1, oper2) {
    debug("BigIntResult: operator*(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = BigInt(oper2);

    BigIntMov(oper2, BigIntMul(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator*(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator*(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntMul(_: oper1, BigInt(oper2)));
}

stock BigIntResult: operator/(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator/(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, _: oper2, quotient, remainder);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), quotient);
}

stock BigIntResult: operator/(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator/(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, _: oper2, quotient, remainder);
    BigIntMov(_: oper2, quotient);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator/(BigIntResult: oper1, BigInt: oper2) {
    debug("BigIntResult: operator/(BigIntResult: %d BigInt: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, _: oper2, quotient, remainder);
    BigIntMov(_: oper1, quotient);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator/(BigInt: oper1, BigInt: oper2) {
    debug("BigIntResult: operator/(BigInt: %d BigInt: %d)", _: oper1, _: oper2);

    BigIntDiv(_: oper1, _: oper2, _: oper1, _: oper2);
    BigIntFree(_: oper2);
    // avoid destructor of BigInt
    #emit load.s.pri oper1
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator/(BigInt: oper1, oper2) {
    debug("BigIntResult: operator/(BigInt: %d, %d)", _: oper1, _: oper2);

    new quotient, remainder;

    oper2 = BigInt(oper2);

    BigIntDiv(_: oper1, oper2, quotient, remainder);
    BigIntMov(oper2, quotient);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator/(oper1, BigInt: oper2) {
    debug("BigIntResult: operator/(%d, BigInt: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    oper1 = BigInt(oper1);

    BigIntDiv(oper1, _: oper2, quotient, remainder);
    BigIntMov(oper1, quotient);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator/(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator/(BigIntResult: %d, %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, BigInt(oper2), quotient, remainder);

    return BigIntResult: BigIntMov(_: oper1, quotient);
}

stock BigIntResult: operator/(oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator/(%d, BigIntResult: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(BigInt(oper1), _: oper2, quotient, remainder);

    return BigIntResult: BigIntMov(_: oper2, quotient);
}

stock BigIntResult: operator%(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator%%(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, _: oper2, quotient, remainder);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), remainder);
}

stock BigIntResult: operator%(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator%%(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, _: oper2, quotient, remainder);
    BigIntMov(_: oper2, remainder);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator%(BigIntResult: oper1, BigInt: oper2) {
    debug("BigIntResult: operator%%(BigIntResult: %d BigInt: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, _: oper2, quotient, remainder);
    BigIntMov(_: oper1, remainder);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator%(BigInt: oper1, BigInt: oper2) {
    debug("BigIntResult: operator%%(BigInt: %d BigInt: %d)", _: oper1, _: oper2);

    BigIntDiv(_: oper1, _: oper2, _: oper1, _: oper2);
    BigIntMov(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator%(BigInt: oper1, oper2) {
    debug("BigIntResult: operator%%(BigInt: %d, %d)", _: oper1, _: oper2);

    new quotient, remainder;

    oper2 = BigInt(oper2);

    BigIntDiv(_: oper1, oper2, quotient, remainder);
    BigIntMov(oper2, remainder);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator%(oper1, BigInt: oper2) {
    debug("BigIntResult: operator%%(%d, BigInt: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    oper1 = BigInt(oper1);

    BigIntDiv(oper1, _: oper2, quotient, remainder);
    BigIntMov(oper1, remainder);
    // avoid destructor of BigInt
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator%(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator%%(BigIntResult: %d, %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(_: oper1, BigInt(oper2), quotient, remainder);

    return BigIntResult: BigIntMov(_: oper1, remainder);
}

stock BigIntResult: operator%(oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator%%(%d, BigIntResult: %d)", _: oper1, _: oper2);

    new quotient, remainder;

    BigIntDiv(BigInt(oper1), _: oper2, quotient, remainder);

    return BigIntResult: BigIntMov(_: oper2, remainder);
}

stock bool: operator==(BigIntResult: oper1, BigIntResult: oper2) {
    debug("bool: operator==(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return !BigIntCmpRR(oper1, oper2);
}

stock bool: operator==(BigInt: oper1, BigIntResult: oper2) {
    debug("bool: operator==(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit not
    #emit retn

    return false;
}

stock bool: operator==(BigInt: oper1, BigInt: oper2) {
    debug("bool: operator==(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit not
    #emit retn

    return false;
}

stock bool: operator==(BigIntResult: oper1, oper2) {
    debug("bool: operator==(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return !BigIntCmpRR(oper1, BigIntResult: BigInt(oper2));
}

stock bool: operator==(BigInt: oper1, oper2) {
    debug("bool: operator==(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigInt(oper2));
    // avoid destructor of BigInt
    #emit not
    #emit retn

    return false;
}

stock bool: operator!=(BigIntResult: oper1, BigIntResult: oper2) {
    debug("bool: operator!=(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return !!BigIntCmpRR(oper1, oper2);
}

stock bool: operator!=(BigInt: oper1, BigIntResult: oper2) {
    debug("bool: operator!=(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit not
    #emit not
    #emit retn

    return false;
}

stock bool: operator!=(BigInt: oper1, BigInt: oper2) {
    debug("bool: operator!=(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit not
    #emit not
    #emit retn

    return false;
}

stock bool: operator!=(BigIntResult: oper1, oper2) {
    debug("bool: operator!=(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return !!BigIntCmpRR(oper1, BigIntResult: BigInt(oper2));
}

stock bool: operator!=(BigInt: oper1, oper2) {
    debug("bool: operator!=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigInt(oper2));
    // avoid destructor of BigInt
    #emit not
    #emit not
    #emit retn

    return false;
}

stock bool: operator<(BigIntResult: oper1, BigIntResult: oper2) {
    debug("bool: operator<(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) == -1;
}

stock bool: operator<(BigIntResult: oper1, BigInt: oper2) {
    debug("bool: operator<(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri 1 // operator order swapped
    #emit retn

    return false;
}

stock bool: operator<(BigInt: oper1, BigIntResult: oper2) {
    debug("bool: operator<(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<(BigInt: oper1, BigInt: oper2) {
    debug("bool: operator<(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<(BigIntResult: oper1, oper2) {
    debug("bool: operator<(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigInt(oper2)) == -1;
}

stock bool: operator<(oper1, BigIntResult: oper2) {
    debug("bool: operator<(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigInt(oper1), oper2) == -1;
}

stock bool: operator<(BigInt: oper1, oper2) {
    debug("bool: operator<(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<(oper1, BigInt: oper2) {
    debug("bool: operator<(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigInt(oper1));
    // avoid destructor of BigInt
    #emit eq.c.pri 1 // operators swapped
    #emit retn

    return false;
}

stock bool: operator>(BigIntResult: oper1, BigIntResult: oper2) {
    debug("bool: operator>(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) == 1;
}

stock bool: operator>(BigIntResult: oper1, BigInt: oper2) {
    debug("bool: operator>(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator>(BigInt: oper1, BigIntResult: oper2) {
    debug("bool: operator>(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit retn

    return false;
}

stock bool: operator>(BigInt: oper1, BigInt: oper2) {
    debug("bool: operator>(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit retn

    return false;
}

stock bool: operator>(BigIntResult: oper1, oper2) {
    debug("bool: operator>(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigInt(oper2)) == 1;
}

stock bool: operator>(oper1, BigIntResult: oper2) {
    debug("bool: operator>(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigInt(oper1), oper2) == 1;
}

stock bool: operator>(BigInt: oper1, oper2) {
    debug("bool: operator>(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit retn

    return false;
}

stock bool: operator>(oper1, BigInt: oper2) {
    debug("bool: operator>(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigInt(oper1));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<=(BigIntResult: oper1, BigIntResult: oper2) {
    debug("bool: operator<=(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) != 1;
}

stock bool: operator<=(BigIntResult: oper1, BigInt: oper2) {
    debug("bool: operator<=(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri -1 // operators swapped
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(BigInt: oper1, BigIntResult: oper2) {
    debug("bool: operator<=(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(BigInt: oper1, BigInt: oper2) {
    debug("bool: operator<=(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(BigIntResult: oper1, oper2) {
    debug("bool: operator<=(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigInt(oper2)) != 1;
}

stock bool: operator<=(oper1, BigIntResult: oper2) {
    debug("bool: operator<=(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigInt(oper1), oper2) != 1;
}

stock bool: operator<=(BigInt: oper1, oper2) {
    debug("bool: operator<=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(oper1, BigInt: oper2) {
    debug("bool: operator<=(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigInt(oper1));
    // avoid destructor of BigInt
    #emit eq.c.pri -1 // operators swapped
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigIntResult: oper1, BigIntResult: oper2) {
    debug("bool: operator>=(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) != -1;
}

stock bool: operator>=(BigIntResult: oper1, BigInt: oper2) {
    debug("bool: operator>=(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigInt: oper1, BigIntResult: oper2) {
    debug("bool: operator>=(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigInt: oper1, BigInt: oper2) {
    debug("bool: operator>=(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigIntResult: oper1, oper2) {
    debug("bool: operator>=(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigInt(oper2)) != -1;
}

stock bool: operator>=(oper1, BigIntResult: oper2) {
    debug("bool: operator>=(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigInt(oper1), oper2) != -1;
}

stock bool: operator>=(BigInt: oper1, oper2) {
    debug("bool: operator>=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(oper1, BigInt: oper2) {
    debug("bool: operator>=(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigInt(oper1));
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator!(BigIntResult: oper) {
    new zero;

    return !BigIntCmpR(BigIntZero(zero), oper);
}

stock bool: operator!(BigInt: oper) {
    new zero;

    BigIntCmp(BigIntZero(zero), _: oper);
    // avoid destructor of BigInt
    #emit stack 4
    #emit not
    #emit retn

    return false;
}

// pass BigInt object as reference otherwise it would call the copy constructor
stock BigIntToBinary(const & BigInt: address, dest[], const size = sizeof dest) {
    debug("BigIntToBinary(%d, %d)", _: address, size);

    if(_: address) {
        new destaddr;

        #emit load.s.pri dest
        #emit stor.s.pri destaddr

        BigIntPrint(_: address);

        if(BigIntIsNeg(_: address)) { // BigInts are always stored as absolute value / positive
            new BigInt: int = BigIntResult: BigIntNegInternal(BigIntCopy(_: address)); // make a copy and negate it

            BigIntPrint(_: int);

            raw_memset(destaddr, size << 2, -1); // fill dest array with 1s

            return raw_memcpy(destaddr, _: int + 2, 0, BigIntGetSize(_: int) << 1, size);
        }
        raw_memset(destaddr, size << 2, 0); // clear dest array

        return raw_memcpy(destaddr, _: address + 2, 0, BigIntGetSize(_: address) << 1, size);
    }
    return 0;
}

stock BigIntToHexInternal(address, dest[], const size = sizeof dest) {
    debug("BigIntToHexInternal(%d, %d)", address, size);

    if(address && size > 0) {
        new idx, addr = address + ((BigIntGetSize(address) + 1) << 1), j, h, hex;

        address += 2;

        while(addr != address) {
            #emit load.s.pri addr
            #emit add.c -2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit stor.s.pri hex

            for(j = 0; j != 4 && idx < size; ++j) {
                h = (hex >>> 12) & 0xF;
                hex <<= 4;

                if(!idx && !h) { // skip leading zeros
                    continue;
                }
                dest[idx++] = (h < 10) ? (h + '0') : (h + ('A' - 10));
            }
        }
        if(idx == size) {
            idx--;
        }
        dest[idx] = EOS;

        return idx;
    }
    return 0;
}

stock BigIntToHex(const & BigInt: address, dest[], const size = sizeof dest) {
    debug("BigIntToHex(%d, %d)", _: address, size);

    if(_: address) {
        BigIntPrint(_: address);

        if(BigIntIsNeg(_: address)) { // BigInts are always stored as absolute value / positive
            new BigInt: int = BigIntResult: BigIntNegInternal(BigIntCopy(_: address)); // make a copy and negate it

            BigIntPrint(_: int);

            return BigIntToHexInternal(_: int, dest, size);
        }
        return BigIntToHexInternal(_: address, dest, size);
    }
    return 0;
}