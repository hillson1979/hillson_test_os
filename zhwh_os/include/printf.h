// printf.h
#pragma once

void printf(const char* fmt, ...);
int snprintf(char* str, unsigned int size, const char* fmt, ...);  // 🔥 添加 snprintf 声明
char * decimal_to_hex(int decimal);
