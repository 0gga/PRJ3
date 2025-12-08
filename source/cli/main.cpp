#include "cli.h"

int main() {
	cli c(9001, "192.168.8.179");
	c.run();
	return 0;
}
