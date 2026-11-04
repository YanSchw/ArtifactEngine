#pragma once
#include <filesystem>

/** The roaming AppData directory of the current user, or an empty path if Windows cannot resolve it. */
std::filesystem::path Win64GetRoamingAppDataDirectory();