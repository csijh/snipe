// <Q >   ->   =Q
// <Q *   ->   >L
// Backwards
// <S >   ->   =S
// <Q >   ->   =Q
// * >    ->   =

// Trial incremental bracket matching algorithm. Only forward matching is
// handled. Conventions are C-like, with brackets ( ) and [ ] and { } in
// ascending priority order. Multiline comment delimiters are treated as
// brackets with < > standing in for /* and */. String and character literal
// delimiters " ' are also treated as brackets.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Convert from a bracket code (small integer) to its character, and vice versa.
// Characters < and > represent multiline comment delimiters, characters / and !
// represent a single line comment delimiter and end of line, and characters "
// and ' are literal delimiters, all treated as brackets in this algorithm. The
// character $ is used as a sentinel.
enum { N = 13 };
char spelling[] = "()[]{}<>/!\"'$";
int code[128] = {
    ['('] = 0, [')'] = 1, ['['] = 2, [']'] = 3, ['{'] = 4, ['}'] = 5, ['<'] = 6,
    ['>'] = 7, ['/'] = 8, ['!'] = 9, ['"'] = 10, ['\''] = 11, ['$'] = 12
};

// Each bracket has a code, a status and a pointer to another bracket.
struct bracket { char code, status; struct bracket *other; };
typedef struct bracket bracket;

// The status of a bracket is one of these three.
enum status { Matched = 'M', Mismatched = 'X', Unmatched = 'U' };

// Results of comparing two brackets, namely the most recent opener and the next
// bracket. Result PL means the next bracket is to be a new opener, and result
// XX means not applicable (e.g. first bracket isn't an opener).
enum compare { EQ, GT, LT, PL, XX };

// Table which compares two brackets.
int compare[N][N] = {
/*        (   )   [   ]   {   }   /   !   "   '   $              */
/* ( */ { PL, EQ, PL, LT, PL, LT, PL, GT, PL, PL, LT },
/* [ */ { PL, GT, PL, EQ, PL, LT, PL, GT, PL, PL, LT },
/* { */ { PL, GT, PL, GT, PL, EQ, PL, GT, PL, PL, LT },
/* / */ { GT, GT, GT, GT, GT, GT, GT, EQ, GT, GT, LT },
/* " */ { GT, GT, GT, GT, GT, GT, GT, EQ, EQ, GT, LT },
/* ' */ { GT, GT, GT, GT, GT, GT, GT, EQ, GT, EQ, LT },
/* $ */ { PL, GT, PL, GT, PL, GT, PL, GT, PL, PL, LT },
};

// If comparison is PL, add the next bracket as an opener, e.g. { (
// Return the updated reference to the most recent opener.
static inline bracket *add(bracket *opener, bracket *next) {
    next->other = opener;
    return next;
}

// Undo an add.
static inline bracket *unAdd(bracket *opener, bracket *next) {
    assert(opener == next);
    return opener->other;
}

// The new bracket loses to the last opener, and is mismatched.
static inline bracket *lose(bracket *opener, bracket *next) {
    next->status = Mismatched;
    next->other = opener;
    return opener;
}

// Undo a lose.
static inline bracket *unLose(bracket *opener, bracket *next) {
    assert(next->other == opener);
    next->status = Unmatched;
    next->other = NULL;
    return opener;
}

// Match the new bracket with the last opener, e.g. ( )
static inline bracket *match(bracket *opener, bracket *next) {
    next->status = Matched;
    next->other = opener;
    opener->status = Matched;
    opener->other = next;
    return opener->other;
}

// Undo a match.
static inline bracket *unMatch(bracket *opener, bracket *next) {
    bracket *partner = next->other;
    next->status = Unmatched;
    next->other = partner->other;
    partner->status = Unmatched;
    partner->other = opener;
    return partner;
}

// The next bracket wins over the last opener, which is mismatched.
// TODO: return indication of a repeat? Always repeat? Include repeat?
static inline bracket *win(bracket *opener, bracket *next) {
    opener->status = Mismatched;
    opener->other = next->other;
    next->other = opener;
    return opener->other;
}

// Undo a win.
static inline bracket *unWin(bracket *opener, bracket *next) {
    bracket *partner = next->other;
    next->other = partner->other;
    partner->status = Unmatched;
    partner->other = opener;
    return partner;
}

void show(bracket bs[], int n) {
    for (int i = 0; i < n; i++) printf("%c", spelling[bs[i].code]);
    printf("\n");
    for (int i = 0; i < n; i++) printf("%c", bs[i].status);
    printf("\n");
    for (int i = 0; i < n; i++) printf("%d", (int)(bs[i].other - bs));
    printf("\n");
}

void fill(bracket bs[], char *s) {
    bs[0].code = code['$'];
    bs[0].status = Unmatched;
    bs[0].other = &bs[0];
    for (int i = 0; i < strlen(s); i++) {
        bs[i+1].code = code[s[i]];
        bs[i+1].status = Unmatched;
        bs[i+1].other = &bs[0];
    }
}

int main() {
    bracket bs[100];
    char *s = "{[()]}";
    int n = strlen(s);
    fill(bs, s);
    bracket *opener = &bs[0];
    show(bs, 7);
    for (int i = 1; i < n+1; i++) {
        bracket *next = &bs[i];
        int cmp = compare[opener->code][next->code];
        switch (cmp) {
            case PL: opener = add(opener, next); break;
            case EQ: opener = match(opener, next); break;
            case LT: opener = win(opener, next); break;
            case GT: opener = lose(opener, next); break;
        }
        show(bs, 7);
    }
    /*
        char ot = SE;
        if (o >= 0) ot = bs[os[o]].type;
        char op = table[ot][type];
        printf("OP %d %d %d\n", ot, type, op);
        switch (op) {
            case PL:   add(bs, os);    break;
            case EQ: match(bs, os);  break;
            case GT:   win(bs, os);    break;
            case LOSE:  lose(bs, os);   break;
        }
        show(bs,os);
    }
    */
    return 0;
}
