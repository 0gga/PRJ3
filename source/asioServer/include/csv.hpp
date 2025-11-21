#include <string>

class CsvLogger {
<<<<<<< HEAD
    public:
        // constructor
=======
public:
    void log(const std::string& info);
    std::string getLog() const;
>>>>>>> 42140dc6fa7751fc8324d8d66fce9f133dec1663

        // addLog adds a new line in the csv-file for the corresponding date
        // it adds data for these specs: Date, Time, Door, Name, UserID, Acces
        // void addLog(const std::string& info);
        void addLog(std::string door, std::string name, int userID, std::string acces);

        // getLogByName transfers the csv-file with the corresponding date
        // to the admin pc using TCP
        std::string getLogByDate(std::string date);

        // getLogByName transfers the csv-file with the corresponding name
        std::string getLogByName(std::string);

        
    private:
        

};
