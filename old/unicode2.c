#include <stdio.h>
#include <time.h>

// Table of lengths for ulength. Non-static so ulength can be inlined.
const char ulengthTable[] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 3, 3, 4, 0
};

// Find length of UTF8 character from first byte, or return 0 for invalid.
// Based on https://nullprogram.com/blog/2017/10/06/.
extern inline int ulength(char *s) {
    unsigned char ch = s[0];
    return ulengthTable[ch>>3];
}

// Get the length of a UTF8 character, or zero for invalid.
int ulength2(char *s) {
    int n = 0x1F00 | ((unsigned char *)s)[0];
    n = n >> 3;
    n = n & (n >> 1);
    n = n & (n >> 2);
    n = n & (n >> 4);
    int m = 1 - ((n>>4)&1) + ((n>>2)&2) + ((n>>2)&1) + ((n>>1)&1) - ((n<<2)&4);
    return m;
}


// Linear combo?
//a*0 + b*16 + c*24 + d*28 + e*30 + f*31 + g
//0->1 => g = 1
//a*0 + b*16 + c*24 + d*28 + e*30 + f*31 + 1
//24->2 => c*24+1 = 2 =? c*24 = 1 => c = 1/24


int main(int argc, char const *argv[]) {
    int total1 = 0, total2 = 0;
    char c;
    clock_t t0, t1, t2;
    t0 = clock();
    for (int i = 0; i < 10000000; i++) {
        c = i;
        total1 += ulength(&c);
    }
    t1 = clock();
    for (int i = 0; i < 10000000; i++) {
        c = i;
        total2 += ulength2(&c);
    }
    t2 = clock();
    printf("%d %d\n", total1, total2);
    printf("%ld %ld\n", t1-t0, t2-t1);
    return 0;
}
