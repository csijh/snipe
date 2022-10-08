#include <stdio.h>
#include <stdint.h>

int main() {
    int define = 42;
    float b = 08.;
    printf("%d %d %f\n", define, _Alignof(int), b);
}
