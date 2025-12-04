#include "cli.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <cstring>
#include <algorithm>

cli::cli(int portno, const char* server_ip) : portno(portno), server_ip(server_ip) {
	while ((sockfd = connect_to_server()) < 0) {
		std::cerr << "trying to connect... 2 seconds wait...";
		sleep(2); // sleeps 2 seconds -> blocking call
	}
}

cli::~cli() {
	if (sockfd >= 0)
		close(sockfd);
}


// TCP connect
int cli::connect_to_server() {
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd < 0) {
		perror("ERROR opening socket");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port   = htons(portno);

	if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
		perror("Error converting server ip");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR connecting");
		return -1;
	}

	std::cerr << "Connection established" << std::endl;

	connection = true;

	return sockfd;
}


void cli::send_data(const std::string& msg) {
	std::string cmd = msg + "\n";

	ssize_t n = write(sockfd, cmd.c_str(), cmd.size());

	if (n < 0) {
		perror("write");
		return;
	}
}

bool cli::recieve_data() {
	size_t total = 0; // antallet af bytes modtaget
	ssize_t n    = 0; // antallet af bytes læst

	// læser indtil \n eller \r, og kan blive stuck her hvis, det ikke er en del af beskeden
	// overvej eventuelt, bare at læse indtil EOF via read
	memset(buffer_receive, 0, sizeof(buffer_receive));
	while (total < sizeof(buffer_receive) - 1) {
		n = read(sockfd, buffer_receive + total, sizeof(buffer_receive) - 1 - total);
		if (n < 0) {
			perror("ERROR reading from socket");
			return false;
		}

		if (n == 0) // end of file check.
		{
			std::cerr << "Server closed connection" << std::endl;
			return false;
		}

		total += n;
		// end of line characters check.
		if (buffer_receive[total - 1] == '\n') {
			buffer_receive[total - 1] = '\0';
			break;
		}
	}
	memmove(buffer_receive,
			buffer_receive + 14,
			total - 14 + 1);
	buffer_receive[total] = '\0';

	std::cout << buffer_receive << std::endl;

	return true;
}

bool cli::admin_identification() {
	std::string cli_identification;
	while (true) {
		if (!recieve_data())
			return false;

		if (strcmp(buffer_receive, "Input CLI Identification") == 0
			|| strcmp(buffer_receive, "Incorrect CLI Identification") == 0) {
			std::cout << "<";
			std::cin >> cli_identification;

			send_data(cli_identification);

			continue;
		}

		if (strcmp(buffer_receive, "CLI is ready") == 0)
			return true;
	}
}


void cli::run() {
	printCommands();

	if (!admin_identification())
		return;

	while (connection) {
		std::cout << "Input command. Type 'help' to see overview";

		std::cout << "> ";
		std::string input;
		std::getline(std::cin, input);

		if (input.empty())
			continue;

		if (input.rfind("newDoor", 0) == 0)
			handle_newDoor(input);

		else if (input.rfind("newUser", 0) == 0)
			handle_newUser(input);

		else if (input.rfind("rmDoor", 0) == 0)
			handle_rmDoor(input);

		else if (input.rfind("rmUser", 0) == 0)
			handle_rmUser(input);

		else if (input == "getLog")
			handle_getLog(input);

		else if (input == "exit")
			handle_exit(input);

		else if (input == "shutdown") {
			handle_shutdown(input);
		} else if (input == "help")
			printCommands();
	}

	if (sockfd >= 0) {
		close(sockfd);
		sockfd = -1;
		return;
	}
}


void cli::printCommands() const {
	std::cout
			<< "  SmartLock commands\n"
			<< "  Commands :\n"
			<< "  newDoor <Door name> <accessLevel>   - Add door\n"
			<< "  newUser <Username> <accessLevel>    - Add user (includes NFC-scan)\n"
			<< "  rmDoor <Door name>                  - Delete door\n"
			<< "  rmUser <Username>                   - Delete user\n"
			<< "  mvDoor <Door name> <accessLevel>    - Edit existing door\n"
			<< "  mvUser <Username> <accessLevel>     - Edit existing user\n"
			<< "  exit                                - Exit and kill CLI connection \n"
			<< "  shutdown                            - Shutdown the entire system\n"
			<< "  getSystemLog <'date'>               - Get date-specific system log\n"
			<< "  getUserLog <Username>               - Get user-specific log\n"
			<< "  getDoorLog <Door name>              - Get door-specific log\n"
			<< "  help                                - Print command overview\n"
			<< "\n";
}

bool cli::handle_newDoor(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << std::endl; // confirm msg

	std::string confirmation;
	std::cout << "<approved/denied> ";
	std::cin >> confirmation;

	send_data(confirmation);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Did not add door") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Failed to add door") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Door added successfully") == 0) {
		std::cout << buffer_receive;
		return true;
	}
}

bool cli::handle_newUser(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Awaiting card read") != 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	// Send UID??
	std::string uid{"User ID"};
	send_data(uid);

	if (!recieve_data())
		return false;

	std::cout << buffer_receive << "\n"; // confirm msg

	std::string confirmation;
	std::cout << "<approved/denied> ";
	std::cin >> confirmation;

	send_data(confirmation);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Failed to add user") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "User added successfully") == 0) {
		std::cout << buffer_receive;
		return true;
	}
}

bool cli::handle_rmDoor(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
		strcmp(buffer_receive, "Door could not be found") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << std::endl; // confirm msg

	std::string confirmation;
	std::cout << "<approved/denied> ";
	std::cin >> confirmation;

	send_data(confirmation);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Cancelled remove operation") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Failed to remove door") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Door removed successfully") == 0) {
		std::cout << buffer_receive;
		return true;
	}
}

bool cli::handle_rmUser(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
		strcmp(buffer_receive, "User could not be found") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << std::endl; // confirm msg

	std::string confirmation;
	std::cout << "<approved/denied> ";
	std::cin >> confirmation;

	send_data(confirmation);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Cancelled remove operation") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Failed to remove user") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "User removed successfully") == 0) {
		std::cout << buffer_receive;
		return true;
	}
}

bool cli::handle_getLog(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Failed to get log") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << "\n";

	return true;
}

bool cli::handle_exit(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Failed to exit") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << "\n";

	close(sockfd);
	return true;
}

bool cli::handle_shutdown(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Failed to shutdown") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << "\n";
	close(sockfd);
	sockfd     = -1;
	connection = false;

	return true;
}

bool cli::handle_mvUser(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
		strcmp(buffer_receive, "User could not be found") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << std::endl; // confirm msg

	std::string confirmation;
	std::cout << "<approved/denied> ";
	std::cin >> confirmation;

	send_data(confirmation);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Cancelled edit operation") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Failed to edit user, data may be corrupted") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "User edited successfully") == 0) {
		std::cout << buffer_receive;
		return true;
	}
}

bool cli::handle_mvDoor(const std::string& cmd) {
	send_data(cmd);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Operation failed - Incorrect CLI syntax") == 0 ||
		strcmp(buffer_receive, "Door could not be found") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	std::cout << buffer_receive << std::endl; // confirm msg

	std::string confirmation;
	std::cout << "<approved/denied> ";
	std::cin >> confirmation;

	send_data(confirmation);

	if (!recieve_data())
		return false;

	if (strcmp(buffer_receive, "Cancelled edit operation") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Failed to edit door, data may be corrupted") == 0) {
		std::cout << buffer_receive << "\n";
		return false;
	}

	if (strcmp(buffer_receive, "Door edited successfully") == 0) {
		std::cout << buffer_receive;
		return true;
	}
}
