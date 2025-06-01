#pragma once

#ifndef PCH_H
#define PCH_H

// Add headers that you want to pre-compile here
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sha256/sha256.h>  // Add this
#include <cpp-base64/base64.h>  // Add this

#endif //PCH_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include <nlohmann/json.hpp>