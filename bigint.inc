#if 0
    #define debug printf
#else
    #define debug(%0);
    #define BigIntPrint(%0);
#endif

#pragma unused SAMP_INCLUDES_VERSION // generates warning with the default compiler

// always use cBigInt for variables otherwise you could overwrite an initialized variable
// resulting that the deconstructor, for the overwritten variable, isn't called anymore, thus creating a leak in the heap
#define cBigInt: const BigInt:

static stock raw_memcpy(dest, const source, index, numbytes, maxlength) {
    #pragma unused dest, source, index, numbytes, maxlength

    debug("raw_memcpy(%d, %d, %d, %d, %d)", dest, source, index, numbytes, maxlength);

    #emit stack 8
    #emit sysreq.c memcpy
    #emit stack 0xFFFFFFF8 // -8
    #emit retn

    return 0;
}

static stock raw_memset(address, numbytes, value) {
    debug("raw_memset(%d, %d, %d)", address, numbytes, value);
    // the "fill" opcode only fills to the next smaller multiple of the cell size, fill the remaining bytes manually with strb.i
    switch(numbytes & 3) {
        case 1: {
            #emit load.s.pri address
            #emit load.s.alt numbytes
            #emit add
            #emit add.c 0xFFFFFFFF // -1
            #emit move.alt
            #emit load.s.pri value
            #emit strb.i 1
        }
        case 2: {
            #emit load.s.pri address
            #emit load.s.alt numbytes
            #emit add
            #emit add.c 0xFFFFFFFE // -2
            #emit move.alt
            #emit load.s.pri value
            #emit strb.i 2
        }
        case 3: {
            #emit load.s.pri address
            #emit load.s.alt numbytes
            #emit add
            #emit add.c 0xFFFFFFFD // -3
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

static stock raw_memcmp(address1, address2, numbytes) { // returns -1, 0, 1, unused
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

static stock HeapAlloc(bytes) {
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

static stock HeapFree(bytes) {
    debug("HeapFree(%d)", bytes);

    #emit lctrl 2
    #emit load.s.alt bytes
    #emit sub
    #emit sctrl 2
    #emit retn

    return 0;
}

static stock BigIntAlloc(size) {
    debug("BigIntAlloc(%d)", size);

    if(size >= 0) {
        new bytes = (size + 1) << 1;
        new address = HeapAlloc(bytes);

        raw_memset(address, bytes, 0);
        BigIntSetSize(address, size);

        return address;
    }
    return 0;
}

// Note: Only works on the last created BigInt
static stock BigIntRealloc(address, newsize) {
    debug("BigIntRealloc(%d, %d)", address, newsize);

    if(address && newsize >= 0) {
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
        return address;
    }
    return 0;
}

static stock BigIntCopy(address, alloc = 0) {
    debug("BigIntCopy(%d, %d)", address, alloc);

    if(address) {
        new size = BigIntGetSize(address);
        new bytes = ((alloc += size) + 1) << 1;
        new copy = HeapAlloc(bytes);

        raw_memset(copy, bytes, 0);
        raw_memcpy(copy, address, 0, (size + 1) << 1, bytes);

        BigIntSetSize(copy, alloc);

        return copy;
    }
    return 0;
}

stock BigIntResult: BigInt(integer, bool: unsigned = false) {
    debug("BigInt(%d, %d)", integer, unsigned);

    new int;

    if(!integer) {
        int = BigIntAlloc(0);
    } else {
        new size;

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
    }
    BigIntPrint(int);

    return BigIntResult: int;
}

static stock BigIntFree(address) {
    debug("BigIntFree(%d)", address);

    if(address) {
        // return HeapFree((BigIntGetSize(address) + 1) << 1);

        #emit load.s.pri address
        #emit sctrl 2
        #emit retn
    }
    return false;
}
// Note: Moves bigint to dest and frees everything behind it!!!, returns from the previous function
static stock BigIntRetn(dest, src) {
    debug("BigIntRetn(%d, %d)", dest, src);

    if(dest && src) {
        new size = BigIntGetSize(src);
        new bytes = (size + 1) << 1;

        raw_memcpy(dest, src, 0, bytes, bytes);
        // restore heap
        #emit load.s.pri bytes
        #emit load.s.alt dest
        #emit add
        #emit sctrl 2 // heap
        // restore previous frame
        #emit load.s.pri 0
        #emit sctrl 4 // stack
        #emit sctrl 5 // frame
        // return dest
        #emit move.pri
        #emit retn
    }
    return 0;
}

enum BigIntHidden {
    bool: eBigIntNeg, // Negative
    bool: eBigIntInv, // Inverse
}

static stock BigIntSetSize(address, cells) { // size = number of 16 bit words
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

static stock BigIntGetSize(address) { // size = number of 16 bit words
    debug("BigIntGetSize(%d)", address);

    if(address) {
        #emit load.s.pri address
        #emit lodb.i 2
        #emit shr.c.pri BigIntHidden
        #emit retn
    }
    return 0;
}

static stock BigIntNeg(address) {
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

static stock BigIntSetNeg(address, bool: neg) {
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

static stock bool: BigIntIsNeg(address) {
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

static stock BigIntInv(address) {
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

static stock BigIntSetInv(address, bool: inv) {
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

static stock bool: BigIntIsInv(address) {
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
static stock BigIntTrim(address) {
    debug("BigIntTrim(%d)", address);

    if(address) {
        new size = BigIntGetSize(address), value, trim;
        new addr = address + ((size + 1) << 1);
        // check for empty cells
        do {
            #emit load.s.pri addr
            #emit add.c 0xFFFFFFFE // -2
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

static stock BigIntPrint(address) {
    debug("BigIntPrint(%d)", address);

    if(address) {
        new size = BigIntGetSize(address);

        printf("    size %d isneg %d isinv %d", size, BigIntIsNeg(address), BigIntIsInv(address));

        new addr = address + ((size + 1) << 1), value;

        while(size--) {
            #emit load.s.pri addr
            #emit add.c 0xFFFFFFFE // -2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit stor.s.pri value

            printf("    value %5d %016b", value, value);
        }
        return address;
    }
    return 0;
}

static stock BigIntNegInternal(oper) { // inplace
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

static stock BigIntAdd(oper1, oper2) {
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

static stock BigIntSub(oper1, oper2) {
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
static stock BigIntAddInternal(oper1, oper2) {
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
static stock BigIntSubInternal(oper1, oper2) {
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
        return BigIntTrim(result);
    }
    return 0;
}

static stock BigIntInc(oper) {
    debug("BigIntInc(%d)", oper);

    if(oper) {
        if(BigIntIsNeg(oper)) {
            return BigIntDecInternal(oper);
        }
        return BigIntIncInternal(oper);
    }
    return 0;
}

static stock BigIntDec(oper) {
    debug("BigIntDec(%d)", oper);

    if(oper) {
        if(BigIntIsNeg(oper)) {
            return BigIntIncInternal(oper);
        }
        return BigIntDecInternal(oper);
    }
    return 0;
}

static stock BigIntIncInternal(oper) { // inplace
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

static stock BigIntDecInternal(oper) { // inplace
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
static stock BigIntSquare(oper) {
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

            return _: BigInt(size, true);
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
        return BigIntTrim(result);
    }
    return 0;
}

static stock BigIntMul(oper1, oper2) {
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

            return _: BigInt(size1, true);
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
        return BigIntTrim(result);
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

        new result = BigIntKaratsubaInternal(oper1, oper2);

        if(BigIntIsNeg(oper1) ^ BigIntIsNeg(oper2)) {
            BigIntNeg(result);
        }
        return BigIntTrim(result);
    }
    return 0;
}

static stock BigIntKaratsubaInternal(oper1, oper2) {
    debug("BigIntKaratsubaInternal(%d, %d)", oper1, oper2);

    new size1 = BigIntGetSize(oper1);
    new size2 = BigIntGetSize(oper2);

    if(size1 == 0 || size2 == 0) {
        return _: BigInt(0);
    }
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

        return _: BigInt(size1, true);
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

    return BigIntRetn(low1, BigIntAdd(z0, z1));
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
            return BigIntRetn(dest, result);
        }
        return dest;
    }
    return 0;
}

stock BigIntDivMod(dividend, divisor, & quotient, & remainder) { // unused
    debug("BigIntDivMod(%d, %d, %d, %d)", dividend, divisor, quotient, remainder);

    if(dividend && divisor) {
        new sizeD = BigIntGetSize(divisor);

        if(!sizeD) { // zero
            return 1 / sizeD; // simulate division by zero
        }
        new sizeN = BigIntGetSize(dividend);

        BigIntPrint(dividend);
        BigIntPrint(divisor);

        if((sizeN < sizeD) || ((sizeN == sizeD) && BigIntCmp(dividend, divisor) == -1)) { // dividend < divisor
            quotient = _: BigInt(0); // quotient = 0
            remainder = BigIntCopy(dividend); // remainder = dividend
        } else if(sizeD == 1) { // we can use the div opcode if divisor is small enough
            quotient = BigIntAlloc(sizeN);

            new div = (sizeN + 1) << 1;
            new daddr = dividend + div;
            new qaddr = quotient + div;
            new mod = 0;
            // load divisor value
            #emit load.s.pri divisor
            #emit add.c 2
            #emit lodb.i 2
            #emit stor.s.pri div
            // divide cells
            while(sizeN) {
                #emit load.s.pri daddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri daddr
                #emit lodb.i 2
                #emit load.s.alt mod
                #emit shl.c.alt 16
                #emit or
                #emit load.s.alt div
                #emit udiv
                #emit stor.s.alt mod
                #emit move.alt
                #emit load.s.pri qaddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri qaddr
                #emit xchg
                #emit strb.i 2

                sizeN--;
            }
            BigIntTrim(quotient);

            remainder = _: BigInt(mod);
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

            new msbD = BigIntMsb(div), shift;
            // long division with skipping zeros by shifting
            for(sizeN <<= 4; ; sizeN -= shift) {
                shift = msbD - BigIntMsb(remainder);

                if(sizeN < shift) { // shift remaining size
                    BigIntShiftLeft(remainder, sizeN);
                    BigIntShiftLeft(quotient, sizeN);
                    break;
                } else if(shift == 0) { // no shift but we know that the number is smaller than div so shift by 1
                    if(shift == sizeN) { // we can't shift more
                        break; // so we are done
                    }
                    BigIntShiftLeft(remainder, 1);
                    shift++;
                } else {
                    BigIntShiftLeft(remainder, shift);

                    if(BigIntCmp(remainder, div) == -1) { // remainder < div
                        if(shift == sizeN) { // we can't shift more
                            BigIntShiftLeft(quotient, shift); // shift quotient, remainder is already shifted
                            break;
                        }
                        BigIntShiftLeft(remainder, 1);
                        shift++;
                    }
                }
                // substract divisor from remainder inplace
                raddr = remainder;
                daddr = div;
                carry = 0;
                s = size;

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
                // move and increase quotient
                BigIntShiftLeft(quotient, shift);
                BigIntIncInternal(quotient);
            }
            raddr = quotient + size + 2;
            // check for trimable cells in quotient
            do {
                #emit load.s.pri raddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri raddr
                #emit lodb.i 2
                #emit stor.s.pri carry
            } while(!carry);
            // correct size of quotient
            BigIntSetSize(quotient, (raddr - quotient) >> 1);
            // move hidden / first word of remainder
            #emit load.s.pri raddr
            #emit add.c 2
            #emit stor.s.pri raddr
            #emit move.alt
            #emit load.s.pri remainder
            #emit lodb.i 2
            #emit strb.i 2
            // shift remainder downwards
            raw_memcpy(raddr, remainder + size + 2, 2, size, size);
            // check for trimable cells in remainder
            raddr = (remainder = raddr) + size + 2;

            do {
                #emit load.s.pri raddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri raddr
                #emit lodb.i 2
                #emit stor.s.pri carry
            } while(!carry);
            // correct size
            BigIntSetSize(remainder, (raddr - remainder) >> 1);
            // free heap
            #emit load.s.pri raddr
            #emit add.c 2
            #emit sctrl 2 // heap
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

static stock BigIntDiv(dividend, divisor) { // same as BigIntDivMod without everything related to remainder
    debug("BigIntDiv(%d, %d)", dividend, divisor);

    if(dividend && divisor) {
        new sizeD = BigIntGetSize(divisor);

        if(!sizeD) { // zero
            return 1 / sizeD; // simulate division by zero
        }
        new sizeN = BigIntGetSize(dividend);
        new quotient;

        BigIntPrint(dividend);
        BigIntPrint(divisor);

        if((sizeN < sizeD) || ((sizeN == sizeD) && BigIntCmp(dividend, divisor) == -1)) { // dividend < divisor
            quotient = _: BigInt(0); // quotient = 0
        } else if(sizeD == 1) { // we can use the div opcode if divisor is small enough
            quotient = BigIntAlloc(sizeN);

            new div = (sizeN + 1) << 1;
            new daddr = dividend + div;
            new qaddr = quotient + div;
            new mod = 0;
            // load divisor value
            #emit load.s.pri divisor
            #emit add.c 2
            #emit lodb.i 2
            #emit stor.s.pri div
            // divide cells
            while(sizeN) {
                #emit load.s.pri daddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri daddr
                #emit lodb.i 2
                #emit load.s.alt mod
                #emit shl.c.alt 16
                #emit or
                #emit load.s.alt div
                #emit udiv
                #emit stor.s.alt mod
                #emit move.alt
                #emit load.s.pri qaddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri qaddr
                #emit xchg
                #emit strb.i 2

                sizeN--;
            }
            BigIntTrim(quotient);
        } else {
            quotient = BigIntAlloc(sizeN);

            new size = sizeN << 1;
            new raddr, daddr, carry, s;
            new remainder = BigIntAlloc(size);

            // copy dividend without flags
            raw_memcpy(remainder, dividend + 2, 2, size, size);

            new div = BigIntAlloc(size);
            // copy divisor to shifted position
            raw_memcpy(div, divisor + 2, size + 2, sizeD << 1, size);

            new msbD = BigIntMsb(div), shift;
            // long division with skipping zeros by shifting
            for(sizeN <<= 4; ; sizeN -= shift) {
                shift = msbD - BigIntMsb(remainder);

                if(sizeN < shift) { // shift remaining size
                    BigIntShiftLeft(quotient, sizeN);
                    break;
                } else if(shift == 0) { // no shift but we know that the number is smaller than div so shift by 1
                    if(shift == sizeN) { // we can't shift more
                        break; // so we are done
                    }
                    BigIntShiftLeft(remainder, 1);
                    shift++;
                } else {
                    BigIntShiftLeft(remainder, shift);

                    if(BigIntCmp(remainder, div) == -1) { // remainder < div
                        if(shift == sizeN) { // we can't shift more
                            BigIntShiftLeft(quotient, shift); // shift quotient, remainder is already shifted
                            break;
                        }
                        BigIntShiftLeft(remainder, 1);
                        shift++;
                    }
                }
                // substract divisor from remainder inplace
                raddr = remainder;
                daddr = div;
                carry = 0;
                s = size;

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
                // move and increase quotient
                BigIntShiftLeft(quotient, shift);
                BigIntIncInternal(quotient);
            }
            raddr = quotient + size + 2;
            // check for trimable cells in quotient
            do {
                #emit load.s.pri raddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri raddr
                #emit lodb.i 2
                #emit stor.s.pri carry
            } while(!carry);
            // correct size of quotient
            BigIntSetSize(quotient, (raddr - quotient) >> 1);
            // free heap
            #emit load.s.pri raddr
            #emit add.c 2
            #emit sctrl 2 // heap
        }
        if(BigIntIsNeg(dividend) ^ BigIntIsNeg(divisor)) {
            BigIntNeg(quotient);
        }
        BigIntPrint(quotient);

        return quotient;
    }
    return false;
}

static stock BigIntMod(dividend, divisor) { // same as BigIntDivMod without everything related to quotient
    debug("BigIntMod(%d, %d)", dividend, divisor);

    if(dividend && divisor) {
        new sizeD = BigIntGetSize(divisor);

        if(!sizeD) { // zero
            return 1 / sizeD; // simulate division by zero
        }
        new sizeN = BigIntGetSize(dividend);
        new remainder;

        BigIntPrint(dividend);
        BigIntPrint(divisor);

        if((sizeN < sizeD) || ((sizeN == sizeD) && BigIntCmp(dividend, divisor) == -1)) { // dividend < divisor
            remainder = BigIntCopy(dividend); // remainder = dividend
        } else if(sizeD == 1) { // we can use the div opcode if divisor is small enough
            new div = (sizeN + 1) << 1;
            new daddr = dividend + div;
            new mod = 0;
            // load divisor value
            #emit load.s.pri divisor
            #emit add.c 2
            #emit lodb.i 2
            #emit stor.s.pri div
            // divide cells
            while(sizeN) {
                #emit load.s.pri daddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri daddr
                #emit lodb.i 2
                #emit load.s.alt mod
                #emit shl.c.alt 16
                #emit or
                #emit load.s.alt div
                #emit udiv
                #emit stor.s.alt mod

                sizeN--;
            }
            remainder = _: BigInt(mod);
        } else {
            new size = sizeN << 1;
            new raddr, daddr, carry, s;

            remainder = BigIntAlloc(size);
            // copy dividend without flags
            raw_memcpy(remainder, dividend + 2, 2, size, size);

            new div = BigIntAlloc(size);
            // copy divisor to shifted position
            raw_memcpy(div, divisor + 2, size + 2, sizeD << 1, size);

            new msbD = BigIntMsb(div), shift;
            // long division with skipping zeros by shifting
            for(sizeN <<= 4; ; sizeN -= shift) {
                shift = msbD - BigIntMsb(remainder);

                if(sizeN < shift) { // shift remaining size
                    BigIntShiftLeft(remainder, sizeN);
                    break;
                } else if(shift == 0) { // no shift but we know that the number is smaller than div so shift by 1
                    if(shift == sizeN) { // we can't shift more
                        break; // so we are done
                    }
                    BigIntShiftLeft(remainder, 1);
                    shift++;
                } else {
                    BigIntShiftLeft(remainder, shift);

                    if(BigIntCmp(remainder, div) == -1) { // remainder < div
                        if(shift == sizeN) { // we can't shift more
                            break;
                        }
                        BigIntShiftLeft(remainder, 1);
                        shift++;
                    }
                }
                // substract divisor from remainder inplace
                raddr = remainder;
                daddr = div;
                carry = 0;
                s = size;

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
            }
            // shift remainder downwards
            raw_memcpy(remainder, remainder + size + 2, 2, size, size);
            // check for trimable cells in remainder
            raddr = remainder + size + 2;

            do {
                #emit load.s.pri raddr
                #emit add.c 0xFFFFFFFE // -2
                #emit stor.s.pri raddr
                #emit lodb.i 2
                #emit stor.s.pri carry
            } while(!carry);
            // correct size
            BigIntSetSize(remainder, (raddr - remainder) >> 1);
            // free heap
            #emit load.s.pri raddr
            #emit add.c 2
            #emit sctrl 2 // heap
        }
        return remainder;
    }
    return false;
}

stock BigIntMsb(oper) { // returns the most significant bit position, starting at 1 instead of 0, 0 will be returned if no bit is set or if invalid operator
    debug("BigIntMsb(%d)", oper);

    if(oper) {
        new value;
        new size = BigIntGetSize(oper);
        new addr = oper + ((size + 1) << 1);

        while(size) {
            size--;

            #emit load.s.pri addr
            #emit add.c 0xFFFFFFFE // -2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit stor.s.pri value

            if(value) {
                new msb = (size << 4) + 1;

                if(value & 0xFF00) {
                    value >>= 8;
                    msb += 8;
                }
                if(value & 0xF0) {
                    value >>= 4;
                    msb += 4;
                }
                if(value & 0b1100) {
                    value >>= 2;
                    msb += 2;
                }
                if(value & 0b10) {
                    msb++;
                }
                return msb;
            }
        }
    }
    return 0;
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
                #emit add.c 0xFFFFFFFE // -2
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
                #emit add.c 0xFFFFFFFE // -2 // second cell
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
            #emit add.c 0xFFFFFFFE // -2
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
                #emit add.c 0xFFFFFFFE // -2
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

static stock BigIntCmp(oper1, oper2) { // -1, 0, 1 - oper1 is less than, equal or greater than
    debug("BigIntCmp(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new bool: isneg1 = BigIntIsNeg(oper1);
        new bool: isneg2 = BigIntIsNeg(oper2);
        // check sign
        if(isneg1 != isneg2) {
            new size, value;
            // check for zero
            for(size = BigIntGetSize(oper1); size; --size) {
                #emit load.s.pri oper1
                #emit add.c 2
                #emit stor.s.pri oper1
                #emit lodb.i 2
                #emit stor.s.pri value

                if(value) {
                    return _: isneg2 - _: isneg1; // -1, 1
                }
            }
            for(size = BigIntGetSize(oper2); size; --size) {
                #emit load.s.pri oper2
                #emit add.c 2
                #emit stor.s.pri oper2
                #emit lodb.i 2
                #emit stor.s.pri value

                if(value) {
                    return _: isneg2 - _: isneg1; // -1, 1
                }
            }
            return 0;
        }
        if(isneg1) { // if both negative switch operators to reverse result
            #emit load.s.pri oper1
            #emit load.s.alt oper2
            #emit stor.s.pri oper2
            #emit stor.s.alt oper1
        }
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);

        if(size1 != size2) { // if size don't match check if the bigger int got only zeros
            new diff, addr, value, retn;

            if(size1 < size2) {
                retn = -1;
                diff = size2 - size1;
                addr = oper2 + ((size2 = size1) << 1);
            } else {
                retn = 1;
                diff = size1 - size2;
                addr = oper1 + ((size1 = size2) << 1);
            }
            while(diff) {
                #emit load.s.pri addr
                #emit add.c 2
                #emit stor.s.pri addr
                #emit lodb.i 2
                #emit stor.s.pri value

                if(value) {
                    return retn;
                }
                diff--;
            }
        }
        // compare bytes if size match from highest to lowest
        size1 = (size1 + 1) << 1;

        oper1 += size1;
        oper2 += size1;

        size1 = 0;

        while(!size1 && size2) {
            #emit load.s.pri oper1
            #emit add.c 0xFFFFFFFE // -2
            #emit stor.s.pri oper1
            #emit lodb.i 2
            #emit move.alt
            #emit load.s.pri oper2
            #emit add.c 0xFFFFFFFE // -2
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

stock operator~(const BigInt: oper[], size) {
    #pragma unused size

    debug("operator~(BigInt: %d, %d)", _: oper[0], size);

    return BigIntFree(_: oper[0]);
}

static stock __BigIntCmpRR(BigInt: oper1, BigInt: oper2) {
    return BigIntCmp(_: oper1, _: oper2); // oper2 gets free before oper1!
}

static stock BigIntCmpRR(BigIntResult: oper1, BigIntResult: oper2) {
    #pragma unused oper1, oper2

    #emit const.pri __BigIntCmpRR
    #emit add.c 4
    #emit sctrl 6

    return 0;
}

static stock BigIntCmpIR(const & BigInt: oper1, BigIntResult: oper2) {
    // set reference of oper1 to oper2 and overwrite oper1 with its value
    #emit lref.s.pri oper1
    #emit load.s.alt oper2
    #emit sref.s.alt oper1
    #emit stor.s.pri oper1
    // jump to main function
    #emit const.pri BigIntCmp
    #emit add.c 4
    #emit sctrl 6

    return 0;
}

static stock BigIntCmpII(const & BigInt: oper1, const & BigInt: oper2) {
    // set both reference to 0 and overwrite both with the values
    #emit zero.alt
    #emit lref.s.pri oper1
    #emit sref.s.alt oper1
    #emit stor.s.pri oper1
    #emit lref.s.pri oper2
    #emit sref.s.alt oper2
    #emit stor.s.pri oper2
    // jump to main function
    #emit const.pri BigIntCmp
    #emit add.c 4
    #emit sctrl 6

    return 0;
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

    return BigIntResult: BigIntRetn(min(_: oper1, _: oper2), BigIntAdd(_: oper1, _: oper2));
}

stock BigIntResult: operator+(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator+(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntAdd(_: oper1, _: oper2));
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

    oper2 = _: BigInt(oper2);

    return BigIntResult: BigIntRetn(oper2, BigIntAdd(_: oper1, oper2));
}

stock BigIntResult: operator+(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator+(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntAdd(_: oper1, _: BigInt(_: oper2)));
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

    return BigIntResult: BigIntRetn(min(_: oper1, _: oper2), BigIntSub(_: oper1, _: oper2));
}

stock BigIntResult: operator-(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator-(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntSub(_: oper1, _: oper2));
}

stock BigIntResult: operator-(BigIntResult: oper1, BigInt: oper2) {
    debug("BigIntResult: operator-(BigIntResult: %d BigInt: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntSub(_: oper1, _: oper2));
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

    oper2 = _: BigInt(oper2);

    return BigIntResult: BigIntRetn(oper2, BigIntSub(_: oper1, oper2));
}

stock BigIntResult: operator-(oper1, BigInt: oper2) {
    debug("BigIntResult: operator-(%d, BigInt: %d)", _: oper1, _: oper2);

    oper1 = _: BigInt(oper1);

    return BigIntResult: BigIntRetn(oper1, BigIntSub(oper1, _: oper2));
}

stock BigIntResult: operator++(BigIntResult: oper) {
    #emit const.pri BigIntInc
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigInt: operator++(BigInt: oper) {
    #emit const.pri BigIntInc
    #emit add.c 4
    #emit sctrl 6

    return BigInt: 0;
}

stock BigIntResult: operator--(BigIntResult: oper) {
    #emit const.pri BigIntDec
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigInt: operator--(BigInt: oper) {
    #emit const.pri BigIntDec
    #emit add.c 4
    #emit sctrl 6

    return BigInt: 0;
}

stock BigIntResult: operator-(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator-(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntSub(_: oper1, _: BigInt(oper2)));
}

stock BigIntResult: operator-(oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator-(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntSub(_: BigInt(oper1), _: oper2));
}

stock BigIntResult: operator*(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator*(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(min(_: oper1, _: oper2), BigIntMul(_: oper1, _: oper2));
}

stock BigIntResult: operator*(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator*(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntMul(_: oper1, _: oper2));
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

    oper2 = _: BigInt(oper2);

    return BigIntResult: BigIntRetn(oper2, BigIntMul(_: oper1, oper2));
}

stock BigIntResult: operator*(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator*(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntMul(_: oper1, _: BigInt(oper2)));
}

stock BigIntResult: operator/(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator/(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(min(_: oper1, _: oper2), BigIntDiv(_: oper1, _: oper2));
}

stock BigIntResult: operator/(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator/(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntDiv(_: oper1, _: oper2));
}

stock BigIntResult: operator/(BigIntResult: oper1, BigInt: oper2) {
    debug("BigIntResult: operator/(BigIntResult: %d BigInt: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntDiv(_: oper1, _: oper2));
}

stock BigIntResult: operator/(BigInt: oper1, BigInt: oper2) {
    debug("BigIntResult: operator/(BigInt: %d BigInt: %d)", _: oper1, _: oper2);

    #emit const.pri BigIntDiv
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator/(BigInt: oper1, oper2) {
    debug("BigIntResult: operator/(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = _: BigInt(oper2);

    return BigIntResult: BigIntRetn(oper2, BigIntDiv(_: oper1, oper2));
}

stock BigIntResult: operator/(oper1, BigInt: oper2) {
    debug("BigIntResult: operator/(%d, BigInt: %d)", _: oper1, _: oper2);

    oper1 = _: BigInt(oper1);

    return BigIntResult: BigIntRetn(oper1, BigIntDiv(oper1, _: oper2));
}

stock BigIntResult: operator/(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator/(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntDiv(_: oper1, _: BigInt(oper2)));
}

stock BigIntResult: operator/(oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator/(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntDiv(_: BigInt(oper1), _: oper2));
}

stock BigIntResult: operator%(BigIntResult: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator%%(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(min(_: oper1, _: oper2), BigIntMod(_: oper1, _: oper2));
}

stock BigIntResult: operator%(BigInt: oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator%%(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntMod(_: oper1, _: oper2));
}

stock BigIntResult: operator%(BigIntResult: oper1, BigInt: oper2) {
    debug("BigIntResult: operator%%(BigIntResult: %d BigInt: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntMod(_: oper1, _: oper2));
}

stock BigIntResult: operator%(BigInt: oper1, BigInt: oper2) {
    debug("BigIntResult: operator%%(BigInt: %d BigInt: %d)", _: oper1, _: oper2);

    #emit const.pri BigIntMod
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator%(BigInt: oper1, oper2) {
    debug("BigIntResult: operator%%(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = _: BigInt(oper2);

    return BigIntResult: BigIntRetn(oper2, BigIntMod(_: oper1, oper2));
}

stock BigIntResult: operator%(oper1, BigInt: oper2) {
    debug("BigIntResult: operator%%(%d, BigInt: %d)", _: oper1, _: oper2);

    oper1 = _: BigInt(oper1);

    return BigIntResult: BigIntRetn(oper1, BigIntMod(oper1, _: oper2));
}

stock BigIntResult: operator%(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator%%(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper1, BigIntMod(_: oper1, _: BigInt(oper2)));
}

stock BigIntResult: operator%(oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator%%(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntRetn(_: oper2, BigIntMod(_: BigInt(oper1), _: oper2));
}

stock bool: operator==(BigIntResult: oper1, BigIntResult: oper2) {
    return !BigIntCmpRR(oper1, oper2);
}

stock bool: operator==(BigInt: oper1, BigIntResult: oper2) {
    return !BigIntCmpIR(oper1, oper2);
}

stock bool: operator==(BigInt: oper1, BigInt: oper2) {
    return !BigIntCmpII(oper1, oper2);
}

stock bool: operator==(BigIntResult: oper1, oper2) {
    return !BigIntCmpRR(oper1, BigInt(oper2));
}

stock bool: operator==(BigInt: oper1, oper2) {
    return !BigIntCmpIR(oper1, BigInt(oper2));
}

stock bool: operator!=(BigIntResult: oper1, BigIntResult: oper2) {
    return !!BigIntCmpRR(oper1, oper2);
}

stock bool: operator!=(BigInt: oper1, BigIntResult: oper2) {
    return !!BigIntCmpIR(oper1, oper2);
}

stock bool: operator!=(BigInt: oper1, BigInt: oper2) {
    return !!BigIntCmpII(oper1, oper2);
}

stock bool: operator!=(BigIntResult: oper1, oper2) {
    return !!BigIntCmpRR(oper1, BigInt(oper2));
}

stock bool: operator!=(BigInt: oper1, oper2) {
    return !!BigIntCmpIR(oper1, BigInt(oper2));
}

stock bool: operator<(BigIntResult: oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper1, oper2) == -1;
}

stock bool: operator<(BigIntResult: oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, oper1) == 1;
}

stock bool: operator<(BigInt: oper1, BigIntResult: oper2) {
    return BigIntCmpIR(oper1, oper2) == -1;
}

stock bool: operator<(BigInt: oper1, BigInt: oper2) {
    return BigIntCmpII(oper1, oper2) == -1;
}

stock bool: operator<(BigIntResult: oper1, oper2) {
    return BigIntCmpRR(oper1, BigInt(oper2)) == -1;
}

stock bool: operator<(oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper2, BigInt(oper1)) == 1;
}

stock bool: operator<(BigInt: oper1, oper2) {
    return BigIntCmpIR(oper1, BigInt(oper2)) == -1;
}

stock bool: operator<(oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, BigInt(oper1)) == 1;
}

stock bool: operator>(BigIntResult: oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper1, oper2) == 1;
}

stock bool: operator>(BigIntResult: oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, oper1) == -1;
}

stock bool: operator>(BigInt: oper1, BigIntResult: oper2) {
    return BigIntCmpIR(oper1, oper2) == 1;
}

stock bool: operator>(BigInt: oper1, BigInt: oper2) {
    return BigIntCmpII(oper1, oper2) == 1;
}

stock bool: operator>(BigIntResult: oper1, oper2) {
    return BigIntCmpRR(oper1, BigInt(oper2)) == 1;
}

stock bool: operator>(oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper2, BigInt(oper1)) == -1;
}

stock bool: operator>(BigInt: oper1, oper2) {
    return BigIntCmpIR(oper1, BigInt(oper2)) == 1;
}

stock bool: operator>(oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, BigInt(oper1)) == -1;
}

stock bool: operator<=(BigIntResult: oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper1, oper2) != 1;
}

stock bool: operator<=(BigIntResult: oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, oper1) != -1;
}

stock bool: operator<=(BigInt: oper1, BigIntResult: oper2) {
    return BigIntCmpIR(oper1, oper2) != 1;
}

stock bool: operator<=(BigInt: oper1, BigInt: oper2) {
    return BigIntCmpII(oper1, oper2) != 1;
}

stock bool: operator<=(BigIntResult: oper1, oper2) {
    return BigIntCmpRR(oper1, BigInt(oper2)) != 1;
}

stock bool: operator<=(oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper2, BigInt(oper1)) != -1;
}

stock bool: operator<=(BigInt: oper1, oper2) {
    return BigIntCmpIR(oper1, BigInt(oper2)) != 1;
}

stock bool: operator<=(oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, BigInt(oper1)) != -1;
}

stock bool: operator>=(BigIntResult: oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper1, oper2) != -1;
}

stock bool: operator>=(BigIntResult: oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, oper1) != 1;
}

stock bool: operator>=(BigInt: oper1, BigIntResult: oper2) {
    return BigIntCmpIR(oper1, oper2) != -1;
}

stock bool: operator>=(BigInt: oper1, BigInt: oper2) {
    return BigIntCmpII(oper1, oper2) != -1;
}

stock bool: operator>=(BigIntResult: oper1, oper2) {
    return BigIntCmpRR(oper1, BigInt(oper2)) != -1;
}

stock bool: operator>=(oper1, BigIntResult: oper2) {
    return BigIntCmpRR(oper2, BigInt(oper1)) != 1;
}

stock bool: operator>=(BigInt: oper1, oper2) {
    return BigIntCmpIR(oper1, BigInt(oper2)) != -1;
}

stock bool: operator>=(oper1, BigInt: oper2) {
    return BigIntCmpIR(oper2, BigInt(oper1)) != 1;
}

stock bool: operator!(BigIntResult: oper) {
    return !BigIntCmpRR(oper, BigInt(0));
}

stock bool: operator!(BigInt: oper) {
    return !BigIntCmpIR(oper, BigInt(0));
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

static stock BigIntToHexInternal(address, dest[], const size = sizeof dest) {
    debug("BigIntToHexInternal(%d, %d)", address, size);

    if(address && size > 0) {
        new idx, addr = address + ((BigIntGetSize(address) + 1) << 1), j, h, hex;

        address += 2;

        while(addr != address) {
            #emit load.s.pri addr
            #emit add.c 0xFFFFFFFE // -2
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

stock BigIntToDecimal(const & BigInt: address, dest[], size = sizeof dest) {
    debug("BigIntToDecimal(%d, %d)", _: address, size);

    if(_: address) {
        BigIntPrint(_: address);

        new addr, isize = BigIntGetSize(_: address);

        #emit load.s.pri dest
        #emit stor.s.pri addr
        // clear dest array
        raw_memset(addr, size << 2, 0);

        addr = _: address + ((isize + 1) << 1);

        new carry, d, msd = --size; // last cell

        while(isize) {
            #emit load.s.pri addr
            #emit add.c 0xFFFFFFFE // -2
            #emit stor.s.pri addr
            #emit lodb.i 2
            #emit stor.s.pri carry
            // shift every cell by 16 and split it into each digit until highst digit (msd) reached and carry is 0
            for(d = size; (d >= msd) || (carry && d >= 0); --d) {
                // dest[d] = (dest[d] << 16) + carry, carry = dest[d] / 10, dest[d] %= 10;
                // emit just to speed things / only one div opcode
                #emit load.s.alt dest
                #emit load.s.pri d
                #emit idxaddr
                #emit push.pri
                #emit load.i
                #emit shl.c.pri 16
                #emit load.s.alt carry
                #emit add
                #emit const.alt 10
                #emit sdiv
                #emit stor.s.pri carry
                #emit move.pri
                #emit pop.alt
                #emit stor.i
            }
            msd = d + 1;
            isize--;
        }
        if(msd) { // move string to correct position and convert it to characters
            d = 0;

            if(BigIntIsNeg(_: address)) {
                if(msd == 1) { // to few cells available, we need 1 for eos and 1 for minus sign
                    size = 0; // copy nothing
                } else {
                    dest[0] = '-';
                    d++;
                }
            }
            while(msd <= size) {
                dest[d++] = dest[msd++] + '0';
            }
            dest[d] = EOS;

            return d;
        }
    }
    return 0; // returns 0 for failure or number of characters
}