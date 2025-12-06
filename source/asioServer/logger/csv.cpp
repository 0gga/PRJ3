#include "csv.hpp"
#include <fstream>
#include <iostream>
#include <ctime>
#include <filesystem>

void CsvLogger::addLog(std::string door, std::string name, std::string userID, std::string access) const {
	/// get date from RPi
	time_t timestamp   = std::time(NULL);
	struct tm datetime = *localtime(&timestamp);

	/// create date and time for log
	char date[50];
	strftime(date, 50, "%d/%m/%Y", &datetime);

	char time[50];
	strftime(time, 50, "%H:%M", &datetime);

#ifdef DEBUG
	//std::cout << "Logged " << date << ", " << time << ", " << door << ", " << name << ", " << userID << ", " << access << std::endl;
#endif DEBUG

	/// *** add system log ***
	/// get date dd_mm_yyyy for csv name
	char logDateChar[50];
	strftime(logDateChar, 50, "%Y_%m_%d", &datetime);
	/// convert char* to string
	std::string logDate = logDateChar;
	/// create log name
	std::string logNameDate = "Log_" + logDate + ".csv";

	/// get the path of the logs of system
	std::filesystem::path currentPath = std::filesystem::current_path();
	/// check if folder exists (it should) else create folder
	std::filesystem::create_directories(currentPath / "logs" / "systemLogs");
	std::filesystem::path logPathSystem = currentPath / "logs" / "systemLogs" / logNameDate;

	/// bool to check if file already exists
	bool fileExistsDate = std::filesystem::exists(logPathSystem);
	/// file pointer, opens an existing csv file or creates a new file
	std::ofstream foutDate(logPathSystem.string(), std::ios::out | std::ios::app);
	/// if file didn't exist add header to the new csv file
	if (!fileExistsDate) {
		foutDate << "Date;Time;Door;Name;UserID;Access\n";
	}

	/// insert new data to file
	foutDate << date << ";"
			<< time << ";"
			<< door << ";"
			<< name << ";"
			<< userID << ";"
			<< access
			<< "\n";

	/// *** add user log ***
	/// create log name
	std::string logNameUser = "Log_" + name + ".csv";

	/// check if folder exists (it should) else create folder
	std::filesystem::create_directories(currentPath / "logs" / "userLogs");
	/// get the path of the logs for users
	std::filesystem::path logPathUsers = currentPath / "logs" / "userLogs" / logNameUser;

	/// bool to check if file already exists
	bool fileExistsUser = std::filesystem::exists(logPathUsers);
	/// file pointer, opens an existing csv file or creates a new file
	std::ofstream foutUser(logPathUsers.string(), std::ios::out | std::ios::app);
	/// if file didn't exist add header to the new csv file
	if (!fileExistsUser) {
		foutUser << "Date;Time;Door;Name;UserID;Access\n";
	}

	/// insert new data to file
	foutUser << date << ";"
			<< time << ";"
			<< door << ";"
			<< name << ";"
			<< userID << ";"
			<< access
			<< "\n";

	/// *** add door log ***
	/// create log name
	std::string logNameDoor = "Log_" + door + ".csv";

	/// check if folder exists (it should) else create folder
	std::filesystem::create_directories(currentPath / "logs" / "doorLogs");
	/// get the path of the logs for users
	std::filesystem::path logPathDoors = currentPath / "logs" / "doorLogs" / logNameDoor;

	/// bool to check if file already exists
	bool fileExistsDoor = std::filesystem::exists(logPathDoors);
	/// file pointer, opens an existing csv file or creates a new file
	std::ofstream foutDoor(logPathDoors.string(), std::ios::out | std::ios::app);
	/// if file didn't exist add header to the new csv file
	if (!fileExistsDoor) {
		foutDoor << "Date;Time;Door;Name;UserID;Access\n";
	}

	/// insert new data to file
	foutDoor << date << ";"
			<< time << ";"
			<< door << ";"
			<< name << ";"
			<< userID << ";"
			<< access
			<< "\n";
}

/// returnere path til filen med den dato, som en streng
std::string CsvLogger::getLogByDate(std::string date) {
	/// create log name as string
	std::string logNameDate = "Log_" + date + ".csv";

	/// Get the path of the logs
	std::filesystem::path currentPath   = std::filesystem::current_path();
	std::filesystem::path logPathSystem = currentPath / "logs" / "systemLogs" / logNameDate;

	/// check if a file exist by that date
	bool fileExistsDate = std::filesystem::exists(logPathSystem);
	if (!fileExistsDate) {
		/// add error message, that file doesn't exist
		throw std::runtime_error("[ERROR] log file by date " + date + " doesn't exist");
	} else {
		/// if file exists return path
		return logPathSystem.string();
	}
}

/// returnere path til filen med den dato, som en streng
std::string CsvLogger::getLogByName(std::string name) {
	/// create log name as string
	std::string logNameUser = "Log_" + name + ".csv";

	/// Get the path of the logs
	std::filesystem::path currentPath  = std::filesystem::current_path();
	std::filesystem::path logPathUsers = currentPath / "logs" / "userLogs" / logNameUser;

	/// check if a file exist by that name
	bool fileExistsUser = std::filesystem::exists(logPathUsers);
	if (!fileExistsUser) {
		/// add error message, that file doesn't exist
		throw std::runtime_error("[ERROR] log file by name " + name + " doesn't exist");
	} else {
		/// if file exists return path
		return logPathUsers.string();
	}
}

/// returnere path til filen med den dato, som en streng
std::string CsvLogger::getLogByDoor(std::string door) {
	/// create log name as string
	std::string logNameDoor = "Log_" + door + ".csv";

	/// Get the path of the logs
	std::filesystem::path currentPath  = std::filesystem::current_path();
	std::filesystem::path logPathDoors = currentPath / "logs" / "doorLogs" / logNameDoor;

	/// check if a file exist by that door
	bool fileExistsDoor = std::filesystem::exists(logPathDoors);
	if (!fileExistsDoor) {
		/// add error message, that file doesn't exist
		throw std::runtime_error("[ERROR] log file by door " + door + " doesn't exist");
	} else {
		/// if file exists return path
		return logPathDoors.string();
	}
}

/*!
int main() {
    CsvLogger log;
    log.addLog("hovedindgang", "john_doe", 1234, "denied");
    log.addLog("bagindgang", "john_doe", 4321, "approved");

    std::cout << log.getLogByDate("2025_11_28") << std::endl;
    std::cout << log.getLogByName("john_doe") << std::endl;
    std::cout << log.getLogByDoor("hovedindgang") << std::endl;
    std::cout << log.getLogByName("doe_john") << std::endl;

    return 0;
}
*/
