#include "cli.h"

int main() {
    cli c(9001, "173.16.15.2");
    c.run();
    return 0;
}