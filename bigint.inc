stock raw_memcpy(dest, const source, index, numbytes, maxlength) {
    #pragma unused dest, source, index, numbytes, maxlength

    printf("raw_memcpy(%d, %d, %d, %d, %d)", dest, source, index, numbytes, maxlength);

    #emit stack 8
    #emit sysreq.c memcpy
    #emit stack -8
    #emit retn

    return 0;
}

stock raw_memset(address, cells, value) {
    printf("raw_memset(%d, %d, %d)", address, cells, value);

    #emit lctrl 6
    #emit move.alt
    #emit lctrl 0
    #emit add
    #emit move.alt
    #emit lctrl 1
    #emit sub.alt
    #emit add.c 92 // 23 * 4
    #emit load.s.alt address
    #emit stor.s.pri address
    #emit load.s.pri cells
    #emit shl.c.pri 2
    #emit sref.s.pri address
    #emit load.s.pri value
    #emit fill 0
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

stock BigIntAlloc(cells) {
    printf("BigIntAlloc(%d)", cells);

    if((1 << (cellbits / 2)) > cells > 0) {
        new address = HeapAlloc(cells);

        raw_memset(address, cells, 0);
        BigIntSetSize(address, cells);

        return address;
    }
    return 0;
}

stock BigIntCopy(address) {
    printf("BigIntCopy(%d)", address);

    if(address) {
        new size = BigIntGetSize(address);
        new copy = BigIntAlloc(size);
        new cpyaddr = copy;

        for(new i; i < size; ++i) {
            #emit lref.s.pri address
            #emit sref.s.pri cpyaddr

            address += 4;
            cpyaddr += 4;
        }
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

stock BigIntSetSize(address, cells) {
    printf("BigIntSetSize(%d, %d)", address, cells);

    if(address) {
        #emit load.s.pri cells
        #emit load.s.alt address
        #emit strb.i 2
        #emit retn
    }
    return false;
}

stock BigIntGetSize(address) {
    printf("BigIntGetSize(%d)", address);

    if(address) {
        #emit load.s.pri address
        #emit lodb.i 2
        #emit retn
    } {}
    return false;
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

        printf("    size %d", size);

        address += 2;

        for(new i = 1; i < size; ++i) {
            #emit lref.s.pri address
            #emit stor.s.pri value

            printf("    addr %10d value %10d %032b", address, value, value);

            address += 4;
        }
        #emit load.s.pri address
        #emit lodb.i 2
        #emit stor.s.pri value

        printf("    addr %10d value %10d %016b", address, value, value);
    }
}

stock BigIntAdd(oper1, oper2) {
    #pragma unused oper2

    // TODO: test access to freed variables
    // TODO: maybe reuse existing allocated heap

    printf("BigIntResult: BigIntAdd(BigIntResult: %d, BigIntResult: %d)", oper1, oper2);

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
    new result = BigIntAlloc(size1 + 1);
    new addr = result + 2;

    BigIntPrint(_: oper1);
    BigIntPrint(_: oper2);

    oper1 += 2;
    oper2 += 2;
    // add both values together
    for(new i, j = size2 * 2 - 1; i != j; ++i) {
        #emit load.s.pri oper1
        #emit lodb.i 2
        #emit move.alt
        #emit load.s.pri oper2
        #emit lodb.i 2
        #emit add
        #emit lref.s.alt addr
        #emit add
        #emit sref.s.pri addr

        printf("    adding addresses (%d + %d + %d)", oper1, oper2, addr);

        oper1 += 2;
        oper2 += 2;
        addr += 2;
    }
    // add missing bytes from bigger operator
    for(new i, j = (size1 - size2) * 2; i != j; ++i) {
        #emit load.s.pri oper1
        #emit lodb.i 2
        #emit lref.s.alt addr
        #emit add
        #emit sref.s.pri addr

        printf("    adding addresses (%d + %d)", oper1, addr);

        oper1 += 2;
        addr += 2;
    }
    BigIntPrint(result);
    BigIntTrim(result);

    return result;
}

stock operator~(BigInt: oper[], size) {
    #pragma unused size

    // #emit lref.s.pri oper
    // #emit stor.s.pri oper

    printf("operator~(BigInt: %d, %d)", _: oper[0], size);

    return BigIntFree(_: oper[0]);
}

stock BigInt: operator=(oper) {
    printf("BigInt: operator=(%d)", oper);

    new cells = (oper < (1 << (cellbits / 2))) ? 1 : 2;
    new int = BigIntAlloc(cells);

    if(cells == 1) {
        #emit load.s.pri int
        #emit add.c 2
        #emit move.alt
        #emit load.s.pri oper
        #emit strb.i 2
    } else {
        #emit load.s.pri int
        #emit add.c 2
        #emit move.alt
        #emit load.s.pri oper
        #emit stor.i
    }
    BigIntPrint(int);

    return BigInt: int;
}

stock BigInt: operator=(BigIntResult: oper) {
    printf("BigInt: operator=(BigIntResult: %d)", _: oper);

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

    new result = BigIntAdd(_: oper1, _: oper2);
    new result_size = BigIntGetSize(result);
    new oper1_size = BigIntGetSize(_: oper1);
    new oper2_size = BigIntGetSize(_: oper2);

    new dest = _: ((oper1 < oper2) ? oper1 : oper2);

    raw_memcpy(dest, result, 0, result_size * 4, result_size);
    HeapFree(oper1_size + oper2_size);

    return BigIntResult: dest;
}

stock BigIntResult: operator+(BigInt: oper1, BigIntResult: oper2) {
    printf("BigIntResult: operator+(BigInt: %d BigIntResult: %d)", _: oper1, _: oper2);

    new result = BigIntAdd(_: oper1, _: oper2);
    new result_size = BigIntGetSize(result);
    new oper2_size = BigIntGetSize(_: oper2);

    raw_memcpy(_: oper2, result, 0, result_size * 4, result_size);
    HeapFree(oper2_size);
    // avoid destructor of BigInt
    #emit load.s.pri oper2
    #emit stack 12
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

    new const BigInt: int = oper2;
    new BigIntResult: result = oper1 + BigIntResult: int;
    // avoid destructor of BigInt
    #emit load.s.pri result
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

stock BigIntResult: operator+(BigIntResult: oper1, oper2) {
    printf("BigIntResult: operator+(BigIntResult: %d, %d)", _: oper1, _: oper2);

    new const BigInt: int = oper2;
    new BigIntResult: result = oper1 + BigIntResult: int;
    // avoid destructor of BigInt
    #emit load.s.pri result
    #emit stack 8
    #emit retn

    return BigIntResult: 0;
}

#define cBigInt: const BigInt:
// pass BigInt object as reference otherwise it would call the copy constructor
stock BigIntGetValues(const & BigInt: address, dest[], const size = sizeof dest) {
    printf("BigIntGetValues(%d)", _: address);

    if(address) {
        new destaddr;

        #emit load.s.pri dest
        #emit stor.s.pri destaddr

        BigIntPrint(_: address);

        raw_memcpy(destaddr, _: address + 2, 0, BigIntGetSize(_: address) * 4 - 2, size);

        #emit stack 4
        #emit retn
    }
    // avoid destructor of BigInt
    #emit zero.pri
    #emit retn

    return 0;
}