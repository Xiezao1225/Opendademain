// util.h -- 路径 / 文件 / 编码 小工具
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

#include <string>

std::wstring PathJoin(const std::wstring& dir, const std::wstring& sub);  // [0x140005060]
std::wstring TrimRight(std::wstring s);                                   // [0x140002800]
std::wstring Trim(std::wstring s);
bool         FileExists(const std::wstring& path);
std::wstring GetModulePath();                                             // [0x140002ab0]
std::wstring GetModuleDir();

bool ReadWholeFile(const std::wstring& path, std::string& out);           // [0x1400015e0]
bool WriteWholeFile(const std::wstring& path, const void* data, size_t len);  // [0x1400017e0]
bool WriteWholeFile(const std::wstring& path, const std::wstring& s);
bool WriteWholeFile(const std::wstring& path, const std::string& s);

std::wstring Utf8ToWide(const std::string& s);                            // [0x1400028f0]
std::string  WideToUtf8(const std::wstring& s);

bool EnsureParentDir(const std::wstring& filePath);                       // [0x140003120]

std::wstring FormatWinError(DWORD code);
