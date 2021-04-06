#include <stdio.h>

// Try tree history.
int history[] = { 'a', 'b', -1, 'c', -3, 'd', -6, 'e', 'f', -1, 'g'};
int state[] = { 'e', 'g' };
int n = 11, hpos = 11, spos = 2;
// Actual sequence:
// ab      type a, b
// a       backspace
// ac      type c
// a       backspace
// ad      type d
//         backspace * 2
// ef      type e, f
// e       backspace
// eg      type g

void print() {
    for (int i=0; i<spos; i++) printf("%c", state[i]);
    printf("\n");
}

// Undo most recent, if up follow it.
void undo() {
    if (hpos == 0) return;
    int ch = history[hpos-1];
    if (ch < 0) printf("unexpected negative\n");
    spos--;
    hpos--;
    if (hpos == 0) return;
//    printf("h1=%d\n", hpos);
    int n = history[hpos-1];
    if (n >= 0) return;
    hpos = hpos -1 + n;
//    printf("h2=%d\n", hpos);
}

// If last child, down. Add char.
void redo() {
    for (int i = n-1; i > hpos; i--) {
        if (history[i] == hpos - i) { hpos = i+1; break; }
    }
    int ch = history[hpos];
    if (ch < 0) printf("unexpected negative\n");
    state[spos++] = ch;
    hpos++;
}

// From end, first undo and redo
// Then small undo and small redo
int main(int argc, char const *argv[]) {
    print();
    printf("---- undo\n");
    undo();
//    printf("h=%d\n", hpos);
    print();
    undo();
//    printf("h=%d\n", hpos);
    print();
    printf("---- redo\n");
    redo();
    print();
    redo();
    print();

    return 0;
}
