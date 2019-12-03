edit newEdit() { return newString(); }
void freeEdit(edit e) { freeString(e); }
int lengthEdit(edit e) { return lengthString(e); }
void *resizeEdit(edit e, int n) { return resizeString(e, n); }
void clearEdit(edit e) { clearString(e); }
int opEdit(edit e) { return opString(e); }
void setOpEdit(edit e, int op) { setOpString(e, op); }
void setAtEdit(edit e, int at) { setAtString(e, at); }
int atEdit(edit e) { return atString(e); }
