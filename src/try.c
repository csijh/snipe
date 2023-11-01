#include <stdio.h>
#include <stdlib.h>
int main(int argc, char const *argv[]) {
    int r = system("start ../help/index.html");
    if (r != 0) r = system("open ../help/index.xhtml");
    if (r != 0) r = system("xdg-open ../help/index.xhtml");
    if (r != 0) r = system("google-chrome ../help/index.xhtml");
    if (r != 0) r = system("chromium-browser ../help/index.xhtml");
    if (r != 0) r = system("firefox ../help/index.xhtml");
    printf("sys %d\n", r);
    return 0;
}
