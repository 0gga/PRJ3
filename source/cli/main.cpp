#include "cli.h"

int main() {
	cli c(9001, "172.20.10.9");
	c.run();
	return 0;
}
