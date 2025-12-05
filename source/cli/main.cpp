#include "cli.h"

int main() {
	cli c(9001, "192.168.86.187");
	c.run();
	return 0;
}
