#include <stdio.h>

char table[10][10] = {
    [0][0] = 0,
    [4][5] = 9
};

int main(int argc, char const *argv[]) {
    printf("4,5,%d\n", table[4][5]);
    return 0;
}
