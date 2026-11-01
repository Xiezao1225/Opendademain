// console.h -- 控制台读写（UTF-16 + UTF-8 双通道）
#pragma once

#include <string>

void ConsoleSetup();                                   // 代码页 / 标题
void ConsoleClear();                                   // 对应原文 FUN_140001530
void ConsoleWrite(const std::wstring& s);              // 对应 FUN_1400025a0
void ConsoleWriteLine(const std::wstring& s = {});     // 写一行
bool ConsoleReadLine(std::wstring& out);               // 读一行（去 CRLF）
