#include <string>

class CsvLogger {
    public:
        /// addLog adds a new line in the csv-file for the corresponding date
        /// it adds data for these specs: Date, Time, Door, Name, UserID, Acces
        /// void addLog(const std::string& info);
        void addLog(std::string door, std::string name, std::string userID, std::string access) const;

        /// getLogByName transfers the csv-file with the corresponding date
        /// to the admin pc using TCP
        std::string getLogByDate(std::string date);

        /// getLogByName transfers the csv-file with the corresponding name
        std::string getLogByName(std::string name);

        /// getLogByDoor transfers the csv-file withe the corresponding door name
        std::string getLogByDoor(std::string door);

};


