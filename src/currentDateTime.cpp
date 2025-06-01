#include "pch.h"
#include "currentDateTime.h"
#include <ctime>

std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    errno_t err = localtime_s(&tstruct, &now);
    if (err != 0) {
        throw std::runtime_error("Failed to convert time using localtime_s");
    }
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &tstruct);
    
    std::string result = "[" + std::string(buf) + "] ";
    return result;
}