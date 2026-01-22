// Copyright (C)2026 lkk9898969 <lkk9898969@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <string>
#include <Windows.h>

namespace NppChineseConverter {

	std::string WideCharToUtf8(const wchar_t* wstr) {
		if (!wstr)
			return {};
		int len = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0)
			return {};
		std::string result(len - 1, '\0'); // 不要包含結尾的 '\0'
		::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr,
			nullptr);
		return result;
	}
	std::wstring Utf8ToWideChar(const char* str) {
		if (!str)
			return {};
		int len = ::MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
		if (len <= 0)
			return {};
		std::wstring result(len - 1, L'\0'); // 不要包含結尾的 L'\0'
		::MultiByteToWideChar(CP_UTF8, 0, str, -1, result.data(), len);
		return result;
	}

} // namespace NppChineseConverter