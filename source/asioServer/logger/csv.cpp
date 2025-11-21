#include "../include/csv.hpp"
#include <fstream>
#include <iostream>
#include <ctime>
#include <filesystem>


void CsvLogger::addLog(std::string door, std::string name, int userID, std::string access) {
	// get date from RPi
	time_t timestamp   = std::time(NULL);
	struct tm datetime = *localtime(&timestamp);

	// create date and time for log
	char date[50];
	strftime(date, 50, "%d/%m/%Y", &datetime);

	char time[50];
	strftime(time, 50, "%H:%M", &datetime);

	std::cout << date << ", " << time << ", " << door << ", " << name << ", " << userID << ", " << access << std::endl;

	// add system log
	// get date dd_mm_yyyy for csv name
	char logDateChar[50];
	strftime(logDateChar, 50, "%Y_%m_%d", &datetime);
	// convert char* to string
	std::string logDate = logDateChar;
	// create log name
	std::string logNameDate = "Log_" + logDate + ".csv";

	// get the path of the logs of system
	std::filesystem::path currentPath   = std::filesystem::current_path();
	std::filesystem::path logPathSystem = currentPath.parent_path() / "logs" / "systemLogs" / logNameDate;
	// file pointer, opens an existing csv file or creates a new file
	std::ofstream foutDate(logPathSystem.string(), std::ios::out | std::ios::app);

	// insert new data to file
	foutDate << date << ";"
			<< time << ";"
			<< door << ";"
			<< name << ";"
			<< userID << ";"
			<< access
			<< "\n";

	// add user log
	// create log name
	std::string logNameUser = "Log_" + name + ".csv";

	// get the path of the logs for users
	std::filesystem::path logPathUsers = currentPath.parent_path() / "logs" / "userLogs" / logNameUser;
	// file pointer, opens an existing csv file or creates a new file
	std::ofstream foutUser(logPathUsers.string(), std::ios::out | std::ios::app);

	// insert new data to file
	foutUser << date << ";"
			<< time << ";"
			<< door << ";"
			<< name << ";"
			<< userID << ";"
			<< access
			<< "\n";
}

std::string CsvLogger::getLogByDate(std::string date) {
	// check if a file exist by that date
	// if statement
	// hvis ikke eksisterer, så kast en "denied".

	// returnere path til filen med den dato, som en streng

	// create log name as string
	std::string logNameDate = "Log_" + date + ".csv";

	// Get the path of the logs
	std::filesystem::path currentPath   = std::filesystem::current_path();
	std::filesystem::path logPathSystem = currentPath.parent_path() / "logs" / "systemLogs" / logNameDate;

	return logPathSystem.string();
}

std::string CsvLogger::getLogByName(std::string name) {
	// check if a file exist by that date
	// if statement
	// hvis ikke eksisterer, så kast en "denied".

	// returnere path til filen med den dato, som en streng

	// create log name as string
	std::string logNameUser = "Log_" + name + ".csv";

	// Get the path of the logs
	std::filesystem::path currentPath  = std::filesystem::current_path();
	std::filesystem::path logPathUsers = currentPath.parent_path() / "logs" / "userLogs" / logNameUser;

	return logPathUsers.string();
}

int main() {
	CsvLogger log;
	log.addLog("Hovedindgang", "JohnDoe", 1234, "denied");
	log.addLog("Bagindgang", "JohnDoe", 4321, "approved");

	std::cout << log.getLogByDate("2025_11_21") << std::endl;
	std::cout << log.getLogByName("JohnDoe") << std::endl;

	return 0;
}
