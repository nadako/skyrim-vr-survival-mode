#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOGDI

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

namespace logger = SKSE::log;
constexpr size_t kTrampolineJumpSize = 14; // 6 bytes instruction + 8 bytes address
constexpr size_t kTrampolineAddressSize = 8; // 8 bytes address only
