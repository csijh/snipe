// Variable length string. Deliberately not opaque.
struct text { int length, max; char *a; };
typedef struct text text;

text *readFile(char *name);

void writeFile(char *name, text *t);
