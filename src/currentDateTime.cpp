#include "pch.h"
#include "currentDateTime.h"
#include <chrono>
#include <format>

std::string currentDateTime() {
    auto const time = std::chrono::current_zone()
        ->to_local(std::chrono::system_clock::now());
    return std::format("[{:%Y-%m-%d %X}] ", time);
}