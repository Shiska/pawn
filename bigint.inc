stock raw_memcpy(dest, const source, index, numbytes, maxlength) {
    #pragma unused dest, source, index, numbytes, maxlength

    printf("raw_memcpy(%d, %d, %d, %d, %d)", dest, source, index, numbytes, maxlength);

    #emit stack 8
    #emit sysreq.c memcpy
    #emit stack -8
    #emit retn

    return 0;
}

stock raw_memset(address, numbytes, value) {
    printf("raw_memset(%d, %d, %d)", address, numbytes, value);

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
    printf("raw_memcmp(%d, %d, %d)", address1, address2, numbytes);

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
    printf("HeapAlloc(%d)", cells);

    #emit lctrl 2
    #emit move.alt
    #emit load.s.pri cells
    #emit shl.c.pri 2
    #emit add
    #emit sctrl 2
    #emit move.pri
    #emit retn

    return 0;
}

stock HeapFree(cells) {
    printf("HeapFree(%d)", cells);

    #emit lctrl 2
    #emit load.s.alt cells
    #emit shl.c.alt 2
    #emit sub
    #emit sctrl 2
    #emit retn

    return 0;
}

enum BigIntFlags (<<= 1) {
    bool: BigIntIsNeg = 1
}

stock BigIntAlloc(cells) {
    printf("BigIntAlloc(%d)", cells);

    if(256 > cells > 0) {
        new address = HeapAlloc(cells);

        raw_memset(address, cells * 4, 0);
        BigIntSetSize(address, cells);

        return address;
    }
    return 0;
}

stock BigIntFromInt(integer) {
    printf("BigIntFromInt(%d)", integer);

    new bool: isneg = integer < 0;

    if(isneg) {
        integer *= -1;
    }
    new cells = (integer & 0xFFFF0000) ? 2 : 1;
    new int = BigIntAlloc(cells);

    if(isneg) {
        BigIntSetFlags(int, BigIntIsNeg);
    }
    if(cells == 1) {
        #emit load.s.pri int
        #emit add.c 2
        #emit move.alt
        #emit load.s.pri integer
        #emit strb.i 2
    } else {
        #emit load.s.pri int
        #emit add.c 2
        #emit move.alt
        #emit load.s.pri integer
        #emit stor.i
    }
    BigIntPrint(int);

    return int;
}

stock BigIntZero(& zero) { // uses a local variable as storage
    printf("BigIntZero(%d)", zero);

    new addr;

    zero = 0;

    #emit load.s.pri zero
    #emit stor.s.pri addr

    BigIntSetSize(addr, 1);

    return addr;
}

stock BigIntCopy(address) {
    printf("BigIntCopy(%d)", address);

    if(address) {
        new size = BigIntGetSize(address);
        new copy = BigIntAlloc(size);

        raw_memcpy(copy, address, 0, size * 4, size);

        return copy;
    }
    return 0;
}

stock BigIntFree(address) {
    printf("BigIntFree(%d)", address);

    if(address) {
        return HeapFree(BigIntGetSize(address));
    }
    return false;
}
// Note: Moves bigint to dest and frees everything behind it!!!
// Only use this function with BigIntResult
stock BigIntMov(dest, src) {
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

stock BigIntSetSize(address, cells) {
    printf("BigIntSetSize(%d, %d)", address, cells);

    if(address) {
        #emit load.s.pri cells
        #emit load.s.alt address
        #emit strb.i 1
        #emit retn
    }
    return false;
}

stock BigIntGetSize(address) {
    printf("BigIntGetSize(%d)", address);

    if(address) {
        #emit load.s.pri address
        #emit lodb.i 1
        #emit retn
    } {}
    return false;
}

stock BigIntSetFlags(address, BigIntFlags: flags) {
    printf("BigIntSetFlags(%d, %08b)", address, _: flags);

    if(address) {
        #emit load.s.pri address
        #emit add.c 1
        #emit move.alt
        #emit load.s.pri flags
        #emit strb.i 1
        #emit retn
    }
    return false;
}

stock BigIntFlags: BigIntGetFlags(address) {
    printf("BigIntGetFlags(%d)", address);

    if(address) {
        #emit load.s.pri address
        #emit add.c 1
        #emit lodb.i 1
        #emit retn
    } {}
    return BigIntFlags: 0;
}

// removes empty cells at the end
// Warning: only use it on the last created BigInt otherwise this could free other data
stock BigIntTrim(address) {
    printf("BigIntTrim(%d)", address);

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

            printf("    addr %d value %d", addr, value);

            if(value) {
                break;
            }
            newsize--;
        }
        if(size != newsize) {
            BigIntSetSize(address, newsize);
            HeapFree(size - newsize);
        }
        return true;
    }
    return false;
}

stock BigIntPrint(address) {
    printf("BigIntPrint(%d)", address);

    if(address) {
        new size = BigIntGetSize(address), value;
        new BigIntFlags: flags = BigIntGetFlags(address);

        printf("    size %d flags %08b", size, _: flags);

        address += 2;

        for(new i = 1; i < size; ++i) {
            #emit lref.s.pri address
            #emit stor.s.pri value

            printf("    addr %10d value %s%10d %032b", address, (value >= 0) ? "+" : "", value, value);

            address += 4;
        }
        #emit load.s.pri address
        #emit lodb.i 2
        #emit stor.s.pri value

        printf("    addr %10d value %s%10d %032b", address, (value >= 0) ? "+" : "", value, value);

    }
}

stock BigIntAdd(oper1, oper2) {
    printf("BigIntAdd(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new bool: isneg1 = bool: (BigIntGetFlags(oper1) & BigIntIsNeg);
        new bool: isneg2 = bool: (BigIntGetFlags(oper2) & BigIntIsNeg);

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
    printf("BigIntSub(%d, %d)", oper1, oper2);

    if(oper1 && oper2) { // same as BigIntAdd, except checks are inverted because isneg2 is mentally inverted due to subbing
        new bool: isneg1 = bool: (BigIntGetFlags(oper1) & BigIntIsNeg);
        new bool: isneg2 = bool: (BigIntGetFlags(oper2) & BigIntIsNeg);

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
    printf("BigIntAddInternal(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);

        new bool: isneg = bool: (BigIntGetFlags(oper1) & BigIntIsNeg); // get it before swapping
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

            printf("    adding addresses (%d + %d + %d)", oper1, oper2, addr);
            BigIntPrint(result);
        }
        j = size1 * 2;

        if(i == j) { // size1 == size2
            if(carry) { // allocate additional cell
                printf("Allocating additional cell - %d", carry);


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

                printf("    adding addresses (%d + %d)", oper1, addr);
                BigIntPrint(result);
            }
        }
        if(isneg) { // if any oper is negative result is negative
            BigIntSetFlags(result, BigIntIsNeg);
        }
        return result;
    }
    return 0;
}

// gets called for one positive (oper1) and one negative (oper2) number
stock BigIntSubInternal(oper1, oper2) {
    printf("BigIntSubInternal(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);
        new minsize, maxsize, maxoper;

        if(size1 < size2) {
            minsize = size1;
            maxsize = size2;
            maxoper = oper2;
        } else {
            minsize = size2;
            maxsize = size1;
            maxoper = oper1;
        }
        new result = BigIntAlloc(maxsize);
        new addr = result + 2;

        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);

        new i = 1;
        new carry = 0;

        oper1 += 2;
        oper2 += 2;
        // compute first half cell
        #emit load.s.pri oper1
        #emit lodb.i 2
        #emit move.alt
        #emit load.s.pri oper2
        #emit lodb.i 2
        #emit sub.alt
        #emit load.s.alt addr
        #emit strb.i 2
        #emit shr.c.pri 31
        #emit stor.s.pri carry

        oper1 += 2;
        oper2 += 2;
        addr += 2;
        // substract remaining cells
        while(i != minsize) {
            #emit lref.s.pri oper1
            #emit lref.s.alt oper2
            #emit sub
            #emit load.s.alt carry
            #emit sub
            #emit sref.s.pri addr
            #emit shr.c.pri 31
            #emit stor.s.pri carry

            printf("    adding addresses (%d + %d + %d)", oper1, oper2, addr);
            BigIntPrint(result);

            oper1 += 4;
            oper2 += 4;
            addr += 4;
            i++;
        }
        // add missing bytes from bigger operator
        if(minsize != maxsize) {
            maxoper += i * 4;

            while(i != maxsize) {
                #emit lref.s.pri maxoper
                #emit load.s.alt carry
                #emit sub
                #emit sref.s.pri addr
                #emit shr.c.pri 31
                #emit stor.s.pri carry

                printf("    adding addresses (%d + %d)", oper1, addr);
                BigIntPrint(result);

                maxoper += 4;
                addr += 4;
                i++;
            }
        }
        if(carry) {
            BigIntNeg(result);
        }
        BigIntPrint(result);
        BigIntTrim(result);
        BigIntPrint(result);

        return result;
    }
    return 0;
}
// don't use this, use BigIntNegFlag instead
stock BigIntNeg(oper) { // inplace
    printf("BigIntNeg(%d)", oper);

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
        }
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
        BigIntSetFlags(oper, BigIntGetFlags(oper) ^ BigIntIsNeg);
        BigIntPrint(oper);

        return oper;
    }
    return 0;
}

stock BigIntNegFlag(oper) {
    printf("BigIntNegFlag(%d)", oper);

    if(oper) {
        BigIntSetFlags(oper, BigIntGetFlags(oper) ^ BigIntIsNeg);

        return oper;
    }
    return 0;
}

stock BigIntMul(oper1, oper2) {
    printf("BigIntMul(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new size1 = BigIntGetSize(oper1);
        new size2 = BigIntGetSize(oper2);
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

        if((BigIntGetFlags(oper1) & BigIntIsNeg) ^ (BigIntGetFlags(oper2) & BigIntIsNeg)) {
            BigIntSetFlags(result, BigIntIsNeg);
        }
        BigIntPrint(_: oper1);
        BigIntPrint(_: oper2);

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

            printf("value2 %d", value2);

            for(i1 = 1; i1 != j1; ++i1) { // result += carry + oper1 + oper2
                printf("i1 %d carry %d %032b", i1, carry);

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
            }
            #emit load.s.pri raddr
            #emit lodb.i 2
            #emit load.s.alt carry
            #emit add
            #emit load.s.alt raddr
            #emit strb.i 2

            BigIntPrint(result);
        }
        return result;
    }
    return 0;
}

stock BigIntCmp(oper1, oper2) { // -1, 0, 1 - oper1 is less than, equal or greater than
    printf("BigIntCmp(%d, %d)", oper1, oper2);

    if(oper1 && oper2) {
        new bool: isneg1 = bool: (BigIntGetFlags(oper1) & BigIntIsNeg);
        new bool: isneg2 = bool: (BigIntGetFlags(oper2) & BigIntIsNeg);
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

stock operator~(BigInt: oper[], size) {
    #pragma unused size

    printf("operator~(BigInt: %d, %d)", _: oper[0], size);

    return BigIntFree(_: oper[0]);
}

stock BigInt: operator=(oper) {
    printf("BigInt: operator=(%d)", oper);

    #emit const.pri BigIntFromInt
    #emit add.c 4
    #emit sctrl 6

    return BigInt: 0;
}

stock BigInt: operator=(BigIntResult: oper) {
    printf("BigInt: operator=(BigIntResult: %d)", _: oper);
    // pass through, just to avoid tag mistake
    return BigInt: oper;
}

stock BigInt: operator=(BigInt: oper) {
    printf("BigInt: operator=(BigInt: %d)", _: oper);

    #emit const.pri BigIntCopy
    #emit add.c 4
    #emit sctrl 6

    return BigInt: 0;
}

stock BigIntResult: operator+(BigIntResult: oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator+(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), BigIntAdd(_: oper1, _: oper2));
}

stock BigIntResult: operator+(BigInt: oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator+(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper2, BigIntAdd(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigInt: oper1, BigInt: oper2) {
    printf("BigIntResult: operator+(BigInt: %d BigInt: %d)", _: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit const.pri BigIntAdd
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigInt: oper1, oper2) {
    printf("BigIntResult: operator+(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = BigIntFromInt(oper2);

    BigIntMov(oper2, BigIntAdd(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigIntResult: oper1, oper2) {
    printf("BigIntResult: operator+(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntAdd(_: oper1, BigIntFromInt(_: oper2)));
}

stock BigIntResult: operator-(BigIntResult: oper) {
    printf("BigIntResult: operator-(BigIntResult: %d)", _: oper);
    // faster than calling the function
    #emit const.pri BigIntNegFlag
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;    
}

stock BigIntResult: operator-(BigInt: oper) {
    printf("BigIntResult: operator-(BigInt: %d)", _: oper);

    (_: oper) = BigIntCopy(_: oper);
    // avoid destructor of BigInt
    #emit const.pri BigIntNegFlag
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;    
}

stock BigIntResult: operator-(BigIntResult: oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator-(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), BigIntSub(_: oper1, _: oper2));
}

stock BigIntResult: operator-(BigInt: oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator-(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper2, BigIntSub(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigIntResult: oper1, BigInt: oper2) {
    printf("BigIntResult: operator-(BigIntResult: %d BigInt: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper1, BigIntSub(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigInt: oper1, BigInt: oper2) {
    printf("BigIntResult: operator-(BigInt: %d BigInt: %d)", _: oper1, _: oper2);

    #emit const.pri BigIntSub
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigInt: oper1, oper2) {
    printf("BigIntResult: operator-(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = BigIntFromInt(oper2);

    BigIntMov(oper2, BigIntSub(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(oper1, BigInt: oper2) {
    printf("BigIntResult: operator-(%d, BigInt: %d)", _: oper1, _: oper2);

    oper1 = BigIntFromInt(oper1);

    BigIntMov(oper1, BigIntSub(oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator-(BigIntResult: oper1, oper2) {
    printf("BigIntResult: operator-(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntSub(_: oper1, BigIntFromInt(oper2)));
}

stock BigIntResult: operator-(oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator-(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper2, BigIntSub(BigIntFromInt(oper1), _: oper2));
}

stock BigIntResult: operator*(BigIntResult: oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator*(BigIntResult: %d BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(min(_: oper1, _: oper2), BigIntMul(_: oper1, _: oper2));
}

stock BigIntResult: operator*(BigInt: oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator*(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    BigIntMov(_: oper2, BigIntMul(_: oper1, _: oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator*(BigInt: oper1, BigInt: oper2) {
    printf("BigIntResult: operator*(BigInt: %d BigInt: %d)", _: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit const.pri BigIntMul
    #emit add.c 4
    #emit sctrl 6

    return BigIntResult: 0;
}

stock BigIntResult: operator*(BigInt: oper1, oper2) {
    printf("BigIntResult: operator*(BigInt: %d, %d)", _: oper1, _: oper2);

    oper2 = BigIntFromInt(oper2);

    BigIntMov(oper2, BigIntMul(_: oper1, oper2));
    // avoid destructor of BigInt
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator*(BigIntResult: oper1, oper2) {
    printf("BigIntResult: operator*(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntResult: BigIntMov(_: oper1, BigIntMul(_: oper1, BigIntFromInt(oper2)));
}

stock bool: operator==(BigIntResult: oper1, BigIntResult: oper2) {
    printf("bool: operator==(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return !BigIntCmpRR(oper1, oper2);
}

stock bool: operator==(BigInt: oper1, BigIntResult: oper2) {
    printf("bool: operator==(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit not
    #emit retn

    return false;
}

stock bool: operator==(BigInt: oper1, BigInt: oper2) {
    printf("bool: operator==(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit not
    #emit retn

    return false;
}

stock bool: operator==(BigIntResult: oper1, oper2) {
    printf("bool: operator==(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return !BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2));
}

stock bool: operator==(BigInt: oper1, oper2) {
    printf("bool: operator==(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit not
    #emit retn

    return false;
}

stock bool: operator!=(BigIntResult: oper1, BigIntResult: oper2) {
    printf("bool: operator!=(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return !!BigIntCmpRR(oper1, oper2);
}

stock bool: operator!=(BigInt: oper1, BigIntResult: oper2) {
    printf("bool: operator!=(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit not
    #emit not
    #emit retn

    return false;
}

stock bool: operator!=(BigInt: oper1, BigInt: oper2) {
    printf("bool: operator!=(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit not
    #emit not
    #emit retn

    return false;
}

stock bool: operator!=(BigIntResult: oper1, oper2) {
    printf("bool: operator!=(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return !!BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2));
}

stock bool: operator!=(BigInt: oper1, oper2) {
    printf("bool: operator!=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit not
    #emit not
    #emit retn

    return false;
}

stock bool: operator<(BigIntResult: oper1, BigIntResult: oper2) {
    printf("bool: operator<(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) == -1;
}

stock bool: operator<(BigIntResult: oper1, BigInt: oper2) {
    printf("bool: operator<(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri 1 // operator order swapped
    #emit retn

    return false;
}

stock bool: operator<(BigInt: oper1, BigIntResult: oper2) {
    printf("bool: operator<(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<(BigInt: oper1, BigInt: oper2) {
    printf("bool: operator<(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<(BigIntResult: oper1, oper2) {
    printf("bool: operator<(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) == -1;
}

stock bool: operator<(oper1, BigIntResult: oper2) {
    printf("bool: operator<(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) == -1;
}

stock bool: operator<(BigInt: oper1, oper2) {
    printf("bool: operator<(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<(oper1, BigInt: oper2) {
    printf("bool: operator<(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigIntFromInt(oper1));
    // avoid destructor of BigInt
    #emit eq.c.pri 1 // operators swapped
    #emit retn

    return false;
}

stock bool: operator>(BigIntResult: oper1, BigIntResult: oper2) {
    printf("bool: operator>(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) == 1;
}

stock bool: operator>(BigIntResult: oper1, BigInt: oper2) {
    printf("bool: operator>(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator>(BigInt: oper1, BigIntResult: oper2) {
    printf("bool: operator>(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit retn

    return false;
}

stock bool: operator>(BigInt: oper1, BigInt: oper2) {
    printf("bool: operator>(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit retn

    return false;
}

stock bool: operator>(BigIntResult: oper1, oper2) {
    printf("bool: operator>(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) == 1;
}

stock bool: operator>(oper1, BigIntResult: oper2) {
    printf("bool: operator>(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) == 1;
}

stock bool: operator>(BigInt: oper1, oper2) {
    printf("bool: operator>(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit retn

    return false;
}

stock bool: operator>(oper1, BigInt: oper2) {
    printf("bool: operator>(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigIntFromInt(oper1));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit retn

    return false;
}

stock bool: operator<=(BigIntResult: oper1, BigIntResult: oper2) {
    printf("bool: operator<=(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) != 1;
}

stock bool: operator<=(BigIntResult: oper1, BigInt: oper2) {
    printf("bool: operator<=(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri -1 // operators swapped
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(BigInt: oper1, BigIntResult: oper2) {
    printf("bool: operator<=(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(BigInt: oper1, BigInt: oper2) {
    printf("bool: operator<=(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(BigIntResult: oper1, oper2) {
    printf("bool: operator<=(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) != 1;
}

stock bool: operator<=(oper1, BigIntResult: oper2) {
    printf("bool: operator<=(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) != 1;
}

stock bool: operator<=(BigInt: oper1, oper2) {
    printf("bool: operator<=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator<=(oper1, BigInt: oper2) {
    printf("bool: operator<=(%d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, BigIntResult: BigIntFromInt(oper1));
    // avoid destructor of BigInt
    #emit eq.c.pri -1 // operators swapped
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigIntResult: oper1, BigIntResult: oper2) {
    printf("bool: operator>=(BigIntResult: %d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, oper2) != -1;
}

stock bool: operator>=(BigIntResult: oper1, BigInt: oper2) {
    printf("bool: operator>=(BigIntResult: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper2, oper1);
    // avoid destructor of BigInt
    #emit eq.c.pri 1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigInt: oper1, BigIntResult: oper2) {
    printf("bool: operator>=(BigInt: %d, BigIntResult: %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigInt: oper1, BigInt: oper2) {
    printf("bool: operator>=(BigInt: %d, BigInt: %d)", _: oper1, _: oper2);

    BigIntCmp(_: oper1, _: oper2);
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(BigIntResult: oper1, oper2) {
    printf("bool: operator>=(BigIntResult: %d, %d)", _: oper1, _: oper2);

    return BigIntCmpRR(oper1, BigIntResult: BigIntFromInt(oper2)) != -1;
}

stock bool: operator>=(oper1, BigIntResult: oper2) {
    printf("bool: operator>=(%d, BigIntResult: %d)", _: oper1, _: oper2);

    return BigIntCmpRR(BigIntResult: BigIntFromInt(oper1), oper2) != -1;
}

stock bool: operator>=(BigInt: oper1, oper2) {
    printf("bool: operator>=(BigInt: %d, %d)", _: oper1, _: oper2);

    BigIntCmpR(_: oper1, BigIntResult: BigIntFromInt(oper2));
    // avoid destructor of BigInt
    #emit eq.c.pri -1
    #emit not
    #emit retn

    return false;
}

stock bool: operator>=(oper1, BigInt: oper2) {
    printf("bool: operator>=(%d, BigInt: %d)", _: oper1, _: oper2);

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

// always use cBigInt for variables otherwise you could overwrite an initialized variable
// resulting that the deconstructor, for the overwritten variable, isn't called anymore, thus creating a leak in the heap
#define cBigInt: const BigInt:
// pass BigInt object as reference otherwise it would call the copy constructor
stock BigIntGetValues(const & BigInt: address, dest[], const size = sizeof dest) {
    printf("BigIntGetValues(%d)", _: address);

    if(_: address) {
        new destaddr;

        #emit load.s.pri dest
        #emit stor.s.pri destaddr

        BigIntPrint(_: address);

        if(BigIntGetFlags(_: address) & BigIntIsNeg) { // BigInts are always stored as absolute value / positive
            new BigInt: int = BigIntResult: BigIntNeg(BigIntCopy(_: address)); // make a copy and negate it

            BigIntPrint(_: int);

            raw_memset(destaddr, size * 4, -1); // fill dest array with 1s

            return raw_memcpy(destaddr, _: int + 2, 0, BigIntGetSize(_: int) * 4 - 2, size);
        }
        raw_memset(destaddr, size * 4, 0); // clear dest array

        return raw_memcpy(destaddr, _: address + 2, 0, BigIntGetSize(_: address) * 4 - 2, size);
    }
    return 0;
}