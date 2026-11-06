#pragma once
#include <filesystem>
#include <string>
#include <vector>

/** The files the user picked in the system's open dialog, none when cancelled. */
std::vector<std::filesystem::path> Win64OpenFilesDialog(const std::wstring& InTitle);
