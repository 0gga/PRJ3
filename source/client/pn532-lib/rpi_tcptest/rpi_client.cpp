#include "rpi_client.h"
#include "wiringPi.h"
#include <chrono>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

client::client(int portno, const char *server_ip, std::string doorname) : portno(portno), server_ip(server_ip), doorname(doorname)
{
	// First wiring pi setup for physical pins.
	if (wiringPiSetupPhys() == -1)
	{
		std::cerr << "WiringPi setup failed" << std::endl;
		exit(1);
	}

	// initialized to correct physical pins.
	buzzer = std::make_unique<Buzzer>(21);
	Led_Green = std::make_unique<Led>(8);
	Led_Yellow = std::make_unique<Led>(11);
	Led_Red = std::make_unique<Led>(10);
	RS = std::make_unique<ReedSwitch>(35);

	// Second setup RFID reader.
	rfid_reader = std::make_unique<PN532Reader>();

	if (!rfid_reader->init_pn532())
	{
		std::cerr << "Failed to initialize PN532" << std::endl;
	}

	// initLeds();

	// Third connect to server (continuous calls, until connected)
	while ((sockfd = connect_to_server()) < 0)
	{
		std::cerr << "trying to connect.. 2 seconds wait.." << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}
}

client::~client()
{
	close(sockfd);
}

int client::connect_to_server()
{
	struct sockaddr_in serv_addr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		perror("ERROR opening socket");
		return -1;
	}

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);

	if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
	{
		perror("Error converting server ip");
		return -1;
	}

	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR connecting");
		return -1;
	}

	std::cerr << "connection established" << std::endl;

	return sockfd;
}

void client::send_data(const std::string &uidstring)
{
	std::string totalstring(doorname);
	totalstring += ':';
	totalstring += uidstring;
	totalstring += '\n';
	std::cerr << "Sending: " << totalstring << std::endl;

	ssize_t n = write(sockfd, totalstring.c_str(), strlen(totalstring.c_str()));
	if (n < 0)
	{
		perror("ERROR writing to socket");
		return;
	}
	std::cerr << "Sent " << n << " bytes" << std::endl;
}

bool client::recieve_data()
{
	size_t total = 0;
	ssize_t n = 0;

	memset(buffer_receive, 0, sizeof(buffer_receive));
	while (total < sizeof(buffer_receive) - 1)
	{
		n = read(sockfd, buffer_receive + total, 1);
		if (n < 0)
		{
			perror("ERROR reading from socket");
			return false;
		}

		if (n == 0) // end of file check.
		{
			std::cerr << "Server closed connection" << std::endl;
			return false;
		}

		// end of line characters check.
		if (buffer_receive[total] == '\n' || buffer_receive[total] == '\r')
		{
			break;
		}

		total += n;
	}

	buffer_receive[total] = '\0';

	// Find the "%%%" delimiter
	char *delimiter = strstr(buffer_receive, "%%%");
	if (delimiter != nullptr)
	{
		// Skip past "%%%" (3 characters)
		char *payload = delimiter + 3;

		// Move payload to start of buffer
		size_t payload_len = strlen(payload);
		memmove(buffer_receive, payload, payload_len + 1); // +1 for null terminator
	}
	// If no delimiter found, keep the buffer as-is (might be error message)

	return true;
}

void client::io_feedback()
{
	// std::cerr << "Buffer contains: '" << buffer_receive << "'" << std::endl;

	if (strcmp(buffer_receive, "approved") == 0)
	{
		std::cerr << "GODKENDT!! velkommen :))" << std::endl;

		// Yellow LED off. Green LED on.
		Led_Yellow->off();
		Led_Green->on();

		std::cout << "RS value before going into loops: " << RS->isDoorOpen() << std::endl;

		auto start_door_closed = std::chrono::high_resolution_clock::now();
		// waiting here for the user to open the door
		while (RS->isDoorOpen())
		{
			auto now_door_closed = std::chrono::high_resolution_clock::now();
			auto duration_door_closed = std::chrono::duration_cast<std::chrono::seconds>(now_door_closed - start_door_closed).count();
			if (duration_door_closed > 10) // 10 seconds
			{
				Led_Green->off();
				Led_Yellow->on();
				std::cout << "Door not opened inside 10 seconds. locking again" << std::endl;
				return; // if door not opened in 10 seconds, return.
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200)); // small sleep time.
		}

		auto start_door_open = std::chrono::high_resolution_clock::now();

		std::cout << "Door open" << std::endl;

		// Waiting here for the user to close the door
		while (RS->isDoorClosed())
		{
			std::cerr << "RS->isDoorClosed() = " << RS->isDoorClosed() << std::endl;
			auto now_door_open = std::chrono::high_resolution_clock::now();
			auto duration_door_open = std::chrono::duration_cast<std::chrono::seconds>(now_door_open - start_door_open).count();
			if (duration_door_open > 10) // 120 seconds / 2 min.
			{
				Led_Green->off();
				Led_Red->on();
				buzzer->beep(1000); // beeps one second and sleeps one second
				std::cout << "Door not closed inside 2 min" << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(200)); // small sleep time.
		}
		// Door closed, turn off green. turn on yellow.
		Led_Green->off();
		Led_Red->off();
		Led_Yellow->on();
	}
	else if (strcmp(buffer_receive, "denied") == 0)
	{
		std::cerr << "AFVIST!! øv bøv, slem slem slem :((" << std::endl;

		Led_Green->off();
		Led_Yellow->off();
		Led_Red->on();

		int i = 0;
		while (i < 6) // blink i 3 sekunder.
		{
			Led_Red->blink(250);
			i++;
		}

		Led_Red->off();
		Led_Yellow->on();
	}
	else
	{
		std::cerr << "Unknown response: '" << buffer_receive << "'" << std::endl;

		Led_Red->off();
		Led_Green->off();

		for (int i = 0; i < 3; i++)
		{
			Led_Yellow->off();
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
			Led_Yellow->on();
			std::this_thread::sleep_for(std::chrono::milliseconds(300));
		}
	}
}

void client::run()
{
	enum State
	{
		WAIT_INPUT,
		SEND,
		RESPONSE,
		FEEDBACK,
	};

	State curr_state = State::WAIT_INPUT;

	// Initial LED states.
	Led_Green->off();
	Led_Red->off();
	Led_Yellow->on();

	std::string uid;
	while (rfid_reader->isInitialized())
	{
		switch (curr_state)
		{
		case State::WAIT_INPUT:
			std::cerr << "State: WAIT_INPUT" << std::endl;
			if (rfid_reader->waitForScan())
			{
				uid = rfid_reader->getStringUID();
				// std::cerr << "Card detected: " << uid << std::endl;
				curr_state = State::SEND;
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(200)); // poll every 200 for uid string ms
			}
			break;

		case State::SEND:
			std::cerr << "State: SEND" << std::endl;
			send_data(uid);
			curr_state = State::RESPONSE;
			break;

		case State::RESPONSE:
		{
			std::cerr << "State: RESPONSE" << std::endl;
			if (recieve_data())
			{
				curr_state = State::FEEDBACK;
				// std::cerr << "Received data: '" << buffer_receive << "'" << std::endl;
			}
			else // if recieve_data returns false -> in case of socket error.
			{
				std::cerr << "No data received, showing error and returning to wait" << std::endl;

				for (int i = 0; i < 2; i++)
				{
					std::cerr << "Error blink " << (i + 1) << std::endl;
					Led_Red->on();
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
					Led_Red->off();
					std::this_thread::sleep_for(std::chrono::milliseconds(200));
				}

				Led_Yellow->on();
				curr_state = State::WAIT_INPUT;
			}
			break;
		}

		case State::FEEDBACK:
			std::cerr << "State: FEEDBACK" << std::endl;
			io_feedback();
			curr_state = State::WAIT_INPUT;
			break;
		}
	}
}

void client::initLeds()
{
	std::cerr << "=== INITLEDS() START ===" << std::endl;
	for (int i = 0; i < 2; i++)
	{
		printf("Cycle %d/2\n", i + 1);

		printf("GREEN ON\n");
		Led_Green->on();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("GREEN OFF\n");
		Led_Green->off();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		;

		printf("YELLOW ON\n");
		Led_Yellow->on();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("YELLOW OFF\n");
		Led_Yellow->off();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		printf("RED ON\n");
		Led_Red->on();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printf("RED OFF\n");
		Led_Red->off();
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		printf("BUZZER ON\n");
		buzzer->beep(1000);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));

		printf("REED SWITCH OPEN: %d\n", RS->isDoorOpen());
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	return;
}
