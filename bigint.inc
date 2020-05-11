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

stock HeapAlloc(cells) {
    debug("HeapAlloc(%d)", cells);

    new heap, stack;

    #emit lctrl 2
    #emit stor.s.pri heap
    #emit lctrl 4
    #emit stor.s.pri stack

    debug("heap %d stack %d", heap, stack);

    #emit lctrl 2
    #emit move.alt
    #emit load.s.pri cells
    #emit shl.c.pri 2
    #emit add
    #emit sctrl 2
    #emit move.pri
    #emit stack 8
    #emit retn

    return 0;
}

stock HeapFree(cells) {
    debug("HeapFree(%d)", cells);

    #emit lctrl 2
    #emit load.s.alt cells
    #emit shl.c.alt 2
    #emit sub
    #emit sctrl 2
    #emit retn

    return 0;
}

stock BigIntAlloc(cells) {
    debug("BigIntAlloc(%d)", cells);

    if(cells > 0) {
        new address = HeapAlloc(cells);

        raw_memset(address, cells * 4, 0);
        BigIntSetSize(address, cells);

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
            BigIntSetSize(address, newsize);

            if(offset < 0) {
                HeapFree(-offset);
            } else {
                raw_memset(HeapAlloc(offset), offset << 2, 0);
            }
            return offset;
        }
    }
    return 0;
}

stock BigIntCopy(address, alloc = 0) {
    debug("BigIntCopy(%d, %d)", address, alloc);

    if(address) {
        new size = BigIntGetSize(address);
        new copy = HeapAlloc((alloc += size));

        raw_memset(copy, alloc << 2, 0);
        raw_memcpy(copy, address, 0, size << 2, size);

        BigIntSetSize(copy, alloc);

        return copy;
    }
    return 0;
}

stock BigIntFromInt(integer) {
    debug("BigIntFromInt(%d)", integer);

    new bool: isneg = bool: (integer >>> 31);

    if(isneg) {
        integer *= -1;
    }
    new cells = (integer & 0xFFFF0000) ? 2 : 1;
    new int = BigIntAlloc(cells);

    if(isneg) {
        BigIntNeg(int);
    } {}
    #emit load.s.pri int
    #emit add.c 2
    #emit move.alt
    #emit load.s.pri integer
    #emit strb.i 2

    if(cells == 2) {
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
        return HeapFree(BigIntGetSize(address));
    }
    return false;
}
// Note: Moves bigint to dest and frees everything behind it!!!
// Only use this function with BigIntResult
stock BigIntMov(dest, src) {
    debug("BigIntMov(%d, %d)", dest, src);

    if(dest && src) {
        new size = BigIntGetSize(src);
        new bytes = size << 2;

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

stock BigIntSetSize(address, cells) {
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

stock BigIntGetSize(address) {
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
        new size = BigIntGetSize(address), value, addr;

        #emit load.s.alt address
        #emit load.s.pri size
        #emit idxaddr
        #emit stor.s.pri addr

        new newsize = size;

        while(newsize) {
            addr -= 4;

            #emit lref.s.pri addr
            #emit stor.s.pri value

            debug("    addr %d value %d", addr, value);

            if(value) {
                break;
            }
            newsize--;
        }
        if(size != newsize) {
            BigIntRealloc(address, newsize);
        }
        return true;
    }
    return false;
}

stock BigIntPrint(address) {
    debug("BigIntPrint(%d)", address);

    if(address) {
        new size = BigIntGetSize(address), value;

        printf("    size %d hidden %016b", size, _: BigIntGetHidden(address)); // error line

        address += 2;

        for(new i = 1; i < size; ++i) {
            #emit lref.s.pri address
            #emit stor.s.pri value

            printf("    value %s%10d %032b", (value >= 0) ? "+" : "", value, value);

            address += 4;
        } {}
        #emit load.s.pri address
        #emit lodb.i 2
        #emit stor.s.pri value

        printf("    value %s%10d %032b", (value >= 0) ? "+" : "", value, value);
    }
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

        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);

        new carry = 0;
        new i, j = size2 * 2;
        new addr = result + 2;
        // add both values together
        while(++i != j) {
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
        }
        j = size1 * 2;

        if(i == j) { // size1 == size2
            if(carry) { // allocate additional cell
                debug("Allocating additional cell - %d", carry);

                #emit load.s.pri carry
                #emit heap 4
                #emit strb.i 2

                BigIntSetSize(result, size1 + 1);
            }
        } else { // add missing bytes from bigger operator
            // add carry
            #emit load.s.pri oper1
            #emit add.c 2
            #emit stor.s.pri oper1
            #emit lodb.i 2
            #emit load.s.alt carry
            #emit add
            #emit load.s.alt addr
            #emit strb.i 2

            while(++i != j) {
                #emit load.s.pri addr
                #emit add.c 2
                #emit stor.s.pri addr
                #emit move.alt
                #emit load.s.pri oper1
                #emit add.c 2
                #emit stor.s.pri oper1
                #emit lodb.i 2
                #emit strb.i 2
            }
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
stock BigIntSubInternal(oper1, oper2, bool: trim = true) {
    debug("BigIntSubInternal(%d, %d, %d)", oper1, oper2, trim);

    if(oper1 && oper2) {
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);
        new minsize, maxsize;

        if(size1 < size2) {
            minsize = size1;
            maxsize = size2;
        } else {
            minsize = size2;
            maxsize = size1;
        }
        new result = BigIntAlloc(maxsize);
        new addr = result + 2;
        new carry = 0;

        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);

        minsize = (minsize << 1) - 1;
        maxsize = (maxsize << 1) - 1 - minsize;
        // substract cells
        while(minsize--) {
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
            #emit load.s.alt addr
            #emit strb.i 2
            #emit shr.c.pri 31
            #emit stor.s.pri carry

            debug("    subbing addresses (%d + %d)", oper1, oper2);
            BigIntPrint(result);

            addr += 2;
        } // add missing bytes from bigger operator
        if(maxsize) {
            if(size1 < size2) {
                while(maxsize--) {
                    #emit load.s.pri oper2
                    #emit add.c 2
                    #emit stor.s.pri oper2
                    #emit lodb.i 2
                    #emit neg
                    #emit load.s.alt carry
                    #emit sub
                    #emit load.s.alt addr
                    #emit strb.i 2
                    #emit shr.c.pri 31
                    #emit stor.s.pri carry

                    debug("    subbing addresses (%d)", oper2);
                    BigIntPrint(result);

                    addr += 2;
                }
            } else {
                while(maxsize--) {
                    #emit load.s.pri oper1
                    #emit add.c 2
                    #emit stor.s.pri oper1
                    #emit lodb.i 2
                    #emit load.s.alt carry
                    #emit sub
                    #emit load.s.alt addr
                    #emit strb.i 2
                    #emit shr.c.pri 31
                    #emit stor.s.pri carry

                    debug("    subbing addresses (%d)", oper1);
                    BigIntPrint(result);

                    addr += 2;
                }
            }
        }
        if(carry) {
            BigIntNegInternal(result);
        }
        if(trim) {
            BigIntPrint(result);
            BigIntTrim(result);
            BigIntPrint(result);
        }
        return result;
    }
    return 0;
}

stock BigIntNegInternal(oper) { // inplace
    debug("BigIntNegInternal(%d)", oper);

    if(oper) {
        new size = BigIntGetSize(oper);
        new addr = oper + 2;

        BigIntPrint(oper);
        // invert every cell
        for(new i = 1; i != size; ++i) {
            #emit lref.s.pri addr
            #emit invert
            #emit sref.s.pri addr

            addr += 4;
        } {}
        #emit load.s.pri addr
        #emit move.alt
        #emit lodb.i 2
        #emit invert
        #emit strb.i 2
        // add one
        addr = oper + 2;

        for(new i = 1, value; value == 0; ++i, addr += 4) {
            if(i == size) {
                #emit load.s.pri addr
                #emit move.alt
                #emit lodb.i 2
                #emit add.c 1
                #emit strb.i 2

                break;
            } else {
                #emit lref.s.pri addr
                #emit add.c 1
                #emit sref.s.pri addr
                #emit stor.s.pri value
            }
        }
        BigIntNeg(oper);
        BigIntPrint(oper);

        return oper;
    }
    return 0;
}

stock BigIntSquare(oper) { // fast squaring by reusing duplicates terms
    debug("BigIntSquare(%d)", oper);

    if(oper) {
        new size = BigIntGetSize(oper);
        // special case for size 1
        if(size == 1) {
            return BigIntKaratsubaInternal(oper, 1, oper, 1);
        }
        new result = BigIntAlloc(size << 1);

        BigIntPrint(_: oper);

        new loop = (size << 2) - 2;
        new maddr = oper + loop; // last cell
        new daddr = result + 2; // dest addr
        new laddr = oper + 2; // left addr
        new raddr = laddr; // right addr
        new carry, mul, sum, left, right;

        while(--loop) {
            sum = carry;
            carry = 0;
            left = laddr;
            right = raddr;

            while(left < right) {
                #emit load.s.pri left
                #emit lodb.i 2
                #emit move.alt
                #emit load.s.pri right
                #emit lodb.i 2
                #emit umul
                #emit stor.s.pri mul

                sum += (mul << 1) & 0xFFFF;
                carry += mul >>> 15;
                right -= 2;
                left += 2;
            }
            if(left == right) {
                #emit load.s.pri left
                #emit lodb.i 2
                #emit move.alt
                #emit umul
                #emit stor.s.pri mul

                sum += mul & 0xFFFF;
                carry += mul >>> 16;
            } {}
            #emit load.s.pri sum
            #emit load.s.alt daddr
            #emit strb.i 2
            #emit shr.c.pri 16
            #emit load.s.alt carry
            #emit add
            #emit stor.s.pri carry

            if(raddr == maddr) {
                laddr += 2;
            } else {
                raddr += 2;
            }
            daddr += 2;
        }
        while(carry) {
            #emit load.s.pri carry
            #emit load.s.alt daddr
            #emit strb.i 2
            #emit shr.c.pri 16
            #emit stor.s.pri carry

            daddr += 2;
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
            return BigIntKaratsubaInternal(oper1, 1, oper2, 1);
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
        new result = BigIntAlloc(size1 + size2 + 1);

        if(BigIntIsNeg(oper1) ^ BigIntIsNeg(oper2)) {
            BigIntNeg(result);
        }
        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);
        // long multiplication
        new carry, value2;
        new raddr, raddrbase = result;
        new o1addr, o2addr = oper2 + 2;
        new i1, j1 = size1 * 2, i2, j2 = size2 * 2;

        for(i2 = 1; i2 != j2; ++i2) {
            #emit load.s.pri o2addr
            #emit lodb.i 2
            #emit stor.s.pri value2

            carry = 0;
            o2addr += 2;
            o1addr = oper1 + 2;
            raddr = (raddrbase += 2);

            for(i1 = 1; i1 != j1; ++i1) { // result += carry + oper1 + oper2
                #emit load.s.pri o1addr
                #emit lodb.i 2
                #emit load.s.alt value2
                #emit umul
                #emit move.alt
                #emit load.s.pri raddr
                #emit lodb.i 2
                #emit add
                #emit load.s.alt carry
                #emit add
                #emit load.s.alt raddr
                #emit strb.i 2
                #emit shr.c.pri 16
                #emit stor.s.pri carry

                BigIntPrint(result);

                o1addr += 2;
                raddr += 2;
            } {}
            #emit load.s.pri raddr
            #emit lodb.i 2
            #emit load.s.alt carry
            #emit add
            #emit load.s.alt raddr
            #emit strb.i 2

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
        BigIntMov(result, BigIntKaratsubaInternal(oper1, (BigIntGetSize(oper1) << 1) - 1, oper2, (BigIntGetSize(oper2) << 1) - 1));

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

stock BigIntKaratsubaInternal(oper1, size1, oper2, size2) {
    debug("BigIntKaratsubaInternal(%d, %d, %d %d)", oper1, size1, oper2, size2);

    if(size1 == 1 && size2 == 1) {
        #emit load.s.pri oper1
        #emit add.c 2
        #emit lodb.i 2
        #emit move.alt
        #emit load.s.pri oper2
        #emit add.c 2
        #emit lodb.i 2
        #emit umul
        #emit stor.s.pri oper1
        #emit shr.c.pri 16
        #emit stor.s.pri oper2

        if(oper2) {
            size1 = BigIntAlloc(2);

            #emit load.s.pri size1
            #emit add.c 4
            #emit move.alt
            #emit load.s.pri oper2
            #emit strb.i 2
        } else {
            size1 = BigIntAlloc(1);
        } {}
        #emit load.s.pri size1
        #emit add.c 2
        #emit move.alt
        #emit load.s.pri oper1
        #emit strb.i 2

        BigIntPrint(size1);

        return size1;
    }
    new size = max(size1, size2) >> 1;
    new bytes = size << 1;

    new osize1 = size1 - size;
    new osize2 = size2 - size;
    // split the integers into two parts
    new low1 = BigIntAlloc((size + 2) >>> 1);
    new low2 = BigIntAlloc((size + 2) >>> 1);
    new high1 = BigIntAlloc((osize1 + 2) >>> 1);
    new high2 = BigIntAlloc((osize2 + 2) >>> 1);

    raw_memcpy(low1, oper1 + 2, 2, bytes, bytes);
    raw_memcpy(low2, oper2 + 2, 2, bytes, bytes);
    raw_memcpy(high1, oper1 + 2 + bytes, 2, osize1 << 1, osize1 << 1);
    raw_memcpy(high2, oper2 + 2 + bytes, 2, osize2 << 1, osize2 << 1);

    new lh1 = BigIntSubInternal(low1, high1, false);
    new lh2 = BigIntSubInternal(high2, low2, false);
    // do it
    new z0 = BigIntKaratsubaInternal(low1, size, low2, size);
    new z1 = BigIntKaratsubaInternal(lh1, osize1, lh2, osize2);
    new z2 = BigIntKaratsubaInternal(high1, osize1, high2, osize2);
    // (z2 << (2 * lshift)) + ((z0 + z1 + z2) << lshift) + z0
    new alloc = ((size + 1) >>> 1); // additional cells to hold the shifted value
    new lshift = size << 4; // shift result to the correct position
    // readd sign to unsigned multiplication
    if(BigIntIsNeg(lh1) ^ BigIntIsNeg(lh2)) {
        BigIntNeg(z1);
    }
    z1 = BigIntAdd(BigIntAdd(z0, z1), z2);
    z2 = BigIntCopy(z2, alloc);

    BigIntShiftLeft(z2, lshift);
    BigIntTrim(z2);

    z1 = BigIntAdd(z1, z2);

    BigIntRealloc(z1, BigIntGetSize(z1) + alloc);
    BigIntShiftLeft(z1, lshift);
    BigIntTrim(z1);

    return BigIntMov(low1, BigIntAdd(z0, z1));
}

stock BigIntPow(oper, pow) {
    debug("BigIntPow(%d, %d)", oper, pow);

    if(oper) {
        if(pow < 0) {
            // oper = BigIntInv(oper);
            pow *= -1;
        }
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

stock BigIntShiftLeft(oper, lshift) { // inplace
    debug("BigIntShiftLeft(%d, %d)", oper, lshift);

    if(oper && lshift > 0) {
        new offset = (lshift >> 4);
        new size = BigIntGetSize(oper);

        BigIntPrint(oper);

        lshift &= 0xF;
        oper += (size << 2) - 2;
        size = (size << 1) - 1 - offset;
        offset <<= 1;

        new tmp;
        new rshift = 16 - lshift;

        if(size >= 1) {
            while(--size) { // oper[i] = (oper[i - offset] << lshift) | (oper[i - offset - 1] >>> rshift);
                #emit load.s.pri oper
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
                #emit load.s.alt oper
                #emit strb.i 2
                #emit const.pri 2
                #emit sub.alt
                #emit stor.s.pri oper
            } {}
            // last affected cell
            #emit load.s.pri oper
            #emit load.s.alt offset
            #emit sub
            #emit lodb.i 2
            #emit load.s.alt lshift
            #emit shl
            #emit load.s.alt oper
            #emit strb.i 2
            #emit const.pri 2
            #emit sub.alt
            #emit stor.s.pri oper
        }
        if(offset) { // clear all remaining cells
            size += offset >>> 1;

            while(size--) {
                #emit zero.pri
                #emit load.s.alt oper
                #emit strb.i 2
                #emit const.pri 2
                #emit sub.alt
                #emit stor.s.pri oper
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
        new offset = (rshift >> 4);
        new size = BigIntGetSize(oper);

        BigIntPrint(oper);

        oper += 2;
        rshift &= 0xF;
        size = (size << 1) - 1 - offset;
        offset <<= 1;

        new tmp;
        new lshift = 16 - rshift;

        if(size >= 1) {
            while(--size) { // oper[i] = (oper[i + offset] << lshift) | (oper[i + offset + 1] >>> rshift);
                #emit load.s.pri oper
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
                #emit load.s.alt oper
                #emit strb.i 2
                #emit const.pri 2
                #emit add
                #emit stor.s.pri oper
            } {}
            // last affected cell
            #emit load.s.pri oper
            #emit load.s.alt offset
            #emit add
            #emit lodb.i 2
            #emit load.s.alt rshift
            #emit shr
            #emit load.s.alt oper
            #emit strb.i 2
            #emit const.pri 2
            #emit add
            #emit stor.s.pri oper
        }
        if(offset) { // clear all remaining cells
            size += offset >>> 1;

            while(size--) {
                #emit zero.pri
                #emit load.s.alt oper
                #emit strb.i 2
                #emit const.pri 2
                #emit add
                #emit stor.s.pri oper
            }
        }
        return oper;
    }
    return 0;
}

// stock msb(cell) {
//     new r;

//     if(cell & 0xFFFF0000) {
//         cell >>>= 16;
//         r = 16;
//     }
//     if(cell & 0xFF00) {
//         cell >>>= 8;
//         r |= 8;
//     }
//     if(cell & 0xF0) {
//         cell >>>= 4;
//         r |= 4;
//     }
//     if(cell & 0xC) {
//         cell >>>= 2;
//         r |= 2;
//     }
//     return r | (cell >>> 1);
// }

// stock BigIntPow(oper, power) {
//     debug("BigIntPow(%d, %d)", oper, power);

//     if(oper && power > 0) {
//         new shifts[32], shift = 0, count = 0;

//         while(power) {
//             if(power & 1) {
//                 shifts[count++] = shift;
//                 debug("power %d count %d shift %d", power, count, shift);
//             }
//             power >>>= 1;
//             shift++;
//         }
//     }
//     return 0;
// }

stock BigIntToHex(address, dest[], const size = sizeof dest) {
    debug("BigIntToHex(%d, %d)", address, size);

    if(address && size > 0) {
        new isize = BigIntGetSize(address);
        new addr = address + (isize << 2);
        new idx;

        for(new j, h, hex; (addr -= 2) != address; ) {
            #emit load.s.pri addr
            #emit lodb.i 2
            #emit stor.s.pri hex

            for(j = 0; j != 4 && idx < size; ++j) {
                h = (hex >> 12 )& 0xF;
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
        return raw_memcmp(oper1, oper2, size1 << 2); // -1, 0, 1
    }
    return 0;
}

stock BigIntCmpR(oper1, BigIntResult: oper2) {
    new result = BigIntCmp(oper1, _: oper2);

    HeapFree(BigIntGetSize(_: oper2));

    return result;
}

stock BigIntCmpRR(BigIntResult: oper1, BigIntResult: oper2) {
    new result = BigIntCmp(_: oper1, _: oper2);

    HeapFree(BigIntGetSize(_: oper1) + BigIntGetSize(_: oper2));

    return result;
}

stock operator~(const BigInt: oper[], size) {
    #pragma unused size

    debug("operator~(BigInt: %d, %d)", _: oper[0], size);

    return BigIntFree(_: oper[0]);
}

stock BigInt: operator=(oper) {
    #pragma unused oper

    debug("BigInt: operator=(%d)", oper);

    #emit const.pri BigIntFromInt
    #emit add.c 4
    #emit sctrl 6

    return BigInt: 0;
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

    oper2 = BigIntFromInt(oper2);

    BigIntMov(oper2, BigIntAdd(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator+(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntAdd(_: oper1, BigIntFromInt(_: oper2)));
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

    oper2 = BigIntFromInt(oper2);

    BigIntMov(oper2, BigIntSub(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(oper1, BigInt: oper2) {
    debug("BigIntResult: operator-(%d, BigInt: %d)", _: oper1, _: oper2);

    oper1 = BigIntFromInt(oper1);

    BigIntMov(oper1, BigIntSub(oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator-(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntSub(_: oper1, BigIntFromInt(oper2)));
}

stock BigIntResult: operator-(oper1, BigIntResult: oper2) {
    debug("BigIntResult: operator-(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper2, BigIntSub(BigIntFromInt(oper1), _: oper2));
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

    oper2 = BigIntFromInt(oper2);

    BigIntMov(oper2, BigIntMul(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator*(BigIntResult: oper1, oper2) {
    debug("BigIntResult: operator*(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntMul(_: oper1, BigIntFromInt(oper2)));
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

    return !BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2));
}

stock bool: operator==(BigInt: oper1, oper2) {
    debug("bool: operator==(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
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

    return !!BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2));
}

stock bool: operator!=(BigInt: oper1, oper2) {
    debug("bool: operator!=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
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

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) == -1;
}

stock bool: operator<(oper1, BigIntResult: oper2) {
    debug("bool: operator<(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) == -1;
}

stock bool: operator<(BigInt: oper1, oper2) {
    debug("bool: operator<(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<(oper1, BigInt: oper2) {
    debug("bool: operator<(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigIntFromInt(oper1));
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

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) == 1;
}

stock bool: operator>(oper1, BigIntResult: oper2) {
    debug("bool: operator>(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) == 1;
}

stock bool: operator>(BigInt: oper1, oper2) {
    debug("bool: operator>(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit retn

    return false;
}

stock bool: operator>(oper1, BigInt: oper2) {
    debug("bool: operator>(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigIntFromInt(oper1));
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

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) != 1;
}

stock bool: operator<=(oper1, BigIntResult: oper2) {
    debug("bool: operator<=(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) != 1;
}

stock bool: operator<=(BigInt: oper1, oper2) {
    debug("bool: operator<=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(oper1, BigInt: oper2) {
    debug("bool: operator<=(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigIntFromInt(oper1));
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

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) != -1;
}

stock bool: operator>=(oper1, BigIntResult: oper2) {
    debug("bool: operator>=(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) != -1;
}

stock bool: operator>=(BigInt: oper1, oper2) {
    debug("bool: operator>=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(oper1, BigInt: oper2) {
    debug("bool: operator>=(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigIntFromInt(oper1));
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
stock BigIntGetValues(const & BigInt: address, dest[], const size = sizeof dest) {
    debug("BigIntGetValues(%d, %d)", _: address, size);

    if(_: address) {
        new destaddr;

        #emit load.s.pri dest
        #emit stor.s.pri destaddr

        BigIntPrint(_: address);

        if(BigIntIsNeg(_: address)) { // BigInts are always stored as absolute value / positive
            new BigInt: int = BigIntResult: BigIntNegInternal(BigIntCopy(_: address)); // make a copy and negate it

            BigIntPrint(_: int);

            raw_memset(destaddr, size * 4, -1); // fill dest array with 1s

            return raw_memcpy(destaddr, _: int + 2, 0, (BigIntGetSize(_: int) << 2) - 2, size);
        }
        raw_memset(destaddr, size * 4, 0); // clear dest array

        return raw_memcpy(destaddr, _: address + 2, 0, (BigIntGetSize(_: address) << 2) - 2, size);
    }
    return 0;
}

stock BigIntGetHex(const & BigInt: address, dest[], const size = sizeof dest) {
    debug("BigIntGetHex(%d, %d)", _: address, size);

    if(_: address) {
        BigIntPrint(_: address);

        if(BigIntIsNeg(_: address)) { // BigInts are always stored as absolute value / positive
            new BigInt: int = BigIntResult: BigIntNegInternal(BigIntCopy(_: address)); // make a copy and negate it

            BigIntPrint(_: int);

            return BigIntToHex(_: int, dest, size);
        }
        return BigIntToHex(_: address, dest, size);
    }
    return 0;
}