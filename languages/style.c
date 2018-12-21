// The Snipe editor is free and open source, see licence.txt.
#include "style.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Check that the two halves of this array match up with the enumeration in
// style.h.
static char *styleNames[CountStyles] = {
    "Point",
    "Select",
    "Gap",
    "Word",
    "Name",
    "Id",
    "Variable",
    "Field",
    "Function",
    "Key",
    "Reserved",
    "Property",
    "Type",
    "Sign",
    "Label",
    "Open",
    "Close",
    "Op",
    "Number",
    "Char",
    "String",
    "Paragraph",
    "Escape",
    "OpenComment",
    "CloseComment",
    "OpenNest",
    "CloseNest",
    "OpenNote",
    "CloseNote",

    "BadPoint",
    "BadSelect",
    "BadGap",
    "BadWord",
    "BadName",
    "BadId",
    "BadVariable",
    "BadField",
    "BadFunction",
    "BadKey",
    "BadReserved",
    "BadProperty",
    "BadType",
    "BadSign",
    "BadLabel",
    "BadOpen",
    "BadClose",
    "BadOp",
    "BadNumber",
    "BadChar",
    "BadString",
    "BadParagraph",
    "BadEscape",
    "BadOpenComment",
    "BadCloseComment",
    "BadOpenNest",
    "BadCloseNest",
    "BadOpenNote",
    "BadCloseNote",
};

static void error(char *s, char *n) {
    printf("Error: %s %s\n", s, n);
    exit(1);
}

style findStyle(char *name) {
    int result = -1;
    for (style s = 0; s < CountStyles; s++) {
        if (styleNames[s] == NULL) error("Missing style name", name);
        if (strcmp(name, styleNames[s]) == 0) {
            if (result != -1) error("Duplicate style name", name);
            result = s;
        }
    }
    if (result == -1) error("Unknown style name", name);
    return result;
}

char *styleName(style s) { return styleNames[s]; }

#ifdef styleTest

int main() {
    setbuf(stdout, NULL);
    assert(findStyle("Point") == Point);
    assert(strcmp(styleName(Point), "Point") == 0);
    assert(findStyle("Select") == Select);
    assert(strcmp(styleName(Select), "Select") == 0);
    assert(findStyle("CloseNote") == CloseNote);
    assert(strcmp(styleName(CloseNote), "CloseNote") == 0);
    assert(findStyle("BadPoint") == Bad+Point);
    assert(strcmp(styleName(Bad+Point), "BadPoint") == 0);
    assert(findStyle("BadCloseNote") == Bad+CloseNote);
    assert(strcmp(styleName(Bad+CloseNote), "BadCloseNote") == 0);
    printf("Style module OK\n");
    return 0;
}

#endif
