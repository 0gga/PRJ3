#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <utility>

extern "C" {
#include "pn532.h"
#include "pn532_rpi.h"
}
#include <nlohmann/json.hpp>
using json = nlohmann::json;


//Append new JSON object to a JSON array in file
void appendToJSON(const std::string& filename, const json& entry){
    json json_arr; 

    std::ifstream inFile(filename); //opens stream for reading file 
    /*
    inFile.good() -> checks file stream ok (file exist) 
    inFile.peek() != eof() checks if the file is not empty -> only read json if its not empty
    inFile >> json_arr -> parse all the existing content into json object.
    */
    if (inFile.good() && inFile.peek() != std::ifstream::traits_type::eof()) {
        try {
            inFile >> json_arr;
        } catch (...) {
            json_arr = json::array(); // reset if corrupt
        }
    } else { //if json doesnt exist or empty -> make new empty array
        json_arr = json::array(); 
    }
    inFile.close(); //closes file
    
    if(!json_arr.is_array()){
        json_arr = json::array(); //make sure its array 
    }

    json_arr.push_back(entry); //append the new json object array
    
    std::ofstream out(filename, std::ios::trunc); //open file to write (overwrite)
    if(!out.is_open()){
        std::cerr << "Could not open: " << filename << "for writing\n"; 
        return; 
    }

    out << json_arr.dump(4); //pretty print (4 times indented)
}


int main(){

    std::string filename = "test_log2.json"; 
    std::ifstream inFile(filename); //opens stream for reading file 

    json json_arr; 
    if (inFile.good() && inFile.peek() != std::ifstream::traits_type::eof()) {
        try {
            inFile >> json_arr;
        } catch (...) {
            json_arr = json::array(); // reset if corrupt
        }
    } else { //if json doesnt exist or empty -> make new empty array
        json_arr = json::array(); 
    }
    inFile.close(); //closes file
    
    if(!json_arr.is_array()){
        json_arr = json::array(); //make sure its array 
    }



    
    return 0; 
}