//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//Copyright (C)2026 lkk9898969 <lkk9898969@gmail.com>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 3 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <codecvt>
#include <string>
#include <vector>
#include <shlwapi.h>

#include "archive.h"
#include "archive_entry.h"
#include "BasicInclude.hpp"
#include "opencc.h"
#include "opencc_archive.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

namespace
{
	__inline HWND getCurrentScintilla();
	std::string WideCharToUtf8(const wchar_t* wstr);
	std::wstring Utf8ToWideChar(const char* str);
	//
	// Function which sets your command 
	//
	bool setCommand(size_t index, const TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk = NULL,
	                bool check0nInit = false);
	sptr_t SciCall(HWND hwnd, UINT msg, uptr_t wParam = 0, sptr_t lParam = 0);

	void translateAll(HWND scintilla_handle, TRANSLATE_MODE translate_mode);
	void reloadConfigs();
	bool extractConfigs();
	void translate(TRANSLATE_MODE);

	wchar_t plugin_config_dir[MAX_PATH];
	std::string config_path;
	std::vector<opencc::SimpleConverter*> converters(HK2TC, nullptr);
}


#pragma region Npp Plugin System Interface
//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{
	// get path of plugin configuration
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, reinterpret_cast<LPARAM>(plugin_config_dir));

	// if config path doesn't exist, we create it
	if (PathFileExists(plugin_config_dir) == FALSE)
	{
		::CreateDirectory(plugin_config_dir, NULL);
	}
	PathAppend(plugin_config_dir, L"ChineseConverter");
	if (PathFileExists(plugin_config_dir) == FALSE)
	{
		::CreateDirectory(plugin_config_dir, NULL);
		extractConfigs();
	}
	config_path = WideCharToUtf8(plugin_config_dir);
	//--------------------------------------------//
	//-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
	//--------------------------------------------//
	// with function :
	// setCommand(int index,                      // zero based number to indicate the order of command
	//            TCHAR *commandName,             // the command name that you want to see in plugin menu
	//            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
	//            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
	//            bool check0nInit                // optional. Make this menu item be checked visually
	//            );
	setCommand(0, TEXT("簡體到繁體"), []() { translate(TRANSLATE_MODE::SC2TC); }, NULL, false);
	setCommand(1, TEXT("簡體到繁體 (含用詞)"), []() { translate(TRANSLATE_MODE::S2TWP); }, NULL, false);
	setCommand(2, TEXT("繁體到簡體"), []() { translate(TRANSLATE_MODE::TC2SC); }, NULL, false);
	setCommand(3, TEXT("繁體到簡體 (含用詞)"), []() { translate(TRANSLATE_MODE::TW2SP); }, NULL, false);
	setCommand(nbFunc - 2, TEXT("---"), NULL, NULL, false);
	setCommand(nbFunc - 1, TEXT("重新載入翻譯設定"), reloadConfigs, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	for (auto& converter : converters)
	{
		if (converter != nullptr)
		{
			delete converter;
			converter = nullptr;
		}
	}
	// Don't forget to deallocate your shortcut here
}

#pragma endregion

namespace
{
#pragma region Helper Functions

	__inline HWND getCurrentScintilla()
	{
		// Get the current scintilla
		int which = -1;
		::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&which));
		if (which == -1)
			return nullptr;
		HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
		return curScintilla;
	}

	std::string WideCharToUtf8(const wchar_t* wstr)
	{
		if (!wstr) return {};
		int len = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0) return {};
		std::string result(len - 1, '\0'); // 不要包含結尾的 '\0'
		::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr, nullptr);
		return result;
	}

	std::wstring Utf8ToWideChar(const char* str)
	{
		if (!str) return {};
		int len = ::MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
		if (len <= 0) return {};
		std::wstring result(len - 1, L'\0'); // 不要包含結尾的 L'\0'
		::MultiByteToWideChar(CP_UTF8, 0, str, -1, result.data(), len);
		return result;
	}

	//
	// This function help you to initialize your plugin commands
	//
	bool setCommand(const size_t index, const TCHAR* cmdName, const PFUNCPLUGINCMD pFunc, ShortcutKey* sk,
	                const bool check0nInit)
	{
		if (index >= nbFunc)
			return false;

		if (!pFunc)
			return false;

		lstrcpy(funcItem[index]._itemName, cmdName);
		funcItem[index]._pFunc = pFunc;
		funcItem[index]._init2Check = check0nInit;
		funcItem[index]._pShKey = sk;

		return true;
	}


	sptr_t SciCall(const HWND hwnd, const UINT msg, const uptr_t wParam, const sptr_t lParam)
	{
		return SendMessage(hwnd, msg, wParam, lParam);
	}

#pragma endregion

	void reloadConfigs()
	{
		if (extractConfigs())
		{
			::MessageBox(NULL, TEXT("重新載入成功。"), TEXT("成功"), MB_OK | MB_ICONINFORMATION);
		}
	}

	void translate(TRANSLATE_MODE translate_mode)
	{
		HWND curScintilla = getCurrentScintilla();
		if (!curScintilla)
		{
			::MessageBox(NULL, TEXT("No active Scintilla control."), TEXT("Error"), MB_OK | MB_ICONERROR);
			return;
		}
		try
		{
			translateAll(curScintilla, translate_mode);
		}
		catch (std::exception e)
		{
			::MessageBox(NULL, Utf8ToWideChar(e.what()).data(), TEXT("Error"), MB_OK | MB_ICONERROR);
		}
	}

	bool extractConfigs()
	{
		/* 參考libarchive github範例 "List contents of Archive stored in Memory" 與 "A Complete Extractor" */
		struct archive* a = archive_read_new();
		struct archive* ext = archive_write_disk_new();
		struct archive_entry* entry;
		constexpr int flags = ARCHIVE_EXTRACT_TIME | ARCHIVE_EXTRACT_PERM;
		int r;
		bool result = false;

		std::wstring wpathname;
		std::wstring fullpath;
		archive_read_support_filter_all(a);
		archive_read_support_format_all(a);
		archive_write_disk_set_options(ext, flags);
		archive_write_disk_set_standard_lookup(ext);
		r = archive_read_open_memory(a, opencc_data, opencc_data_len);
		if (r != ARCHIVE_OK)
		{
			auto error_str = archive_error_string(a);
			auto error_str_w = TEXT("Failed to open archive.\n") + Utf8ToWideChar(error_str);
			::MessageBox(NULL, error_str_w.data(), TEXT("Error"), MB_OK | MB_ICONERROR);
			goto ARCHIVE_FREE;
		}
		while ((r = archive_read_next_header(a, &entry)) == ARCHIVE_OK)
		{
			wpathname = Utf8ToWideChar(archive_entry_pathname(entry));
			fullpath = std::wstring(plugin_config_dir) + L"\\" + wpathname;
			archive_entry_set_pathname(entry, WideCharToUtf8(fullpath.c_str()).c_str());
			r = archive_write_header(ext, entry);
			if (r != ARCHIVE_OK)
			{
				auto error_str = archive_error_string(ext);
				auto error_str_w = TEXT("Error writing header.\n") + Utf8ToWideChar(error_str);
				::MessageBox(NULL, error_str_w.data(), TEXT("Error"), MB_OK | MB_ICONERROR);
				goto ARCHIVE_FREE;
			}
			else if (archive_entry_size(entry) > 0)
			{
				const void* buff;
				size_t size;
				la_int64_t offset;
				for (;;)
				{
					r = archive_read_data_block(a, &buff, &size, &offset);
					if (r == ARCHIVE_EOF)
						break;
					if (r != ARCHIVE_OK)
					{
						auto error_str = archive_error_string(a);
						auto error_str_w = TEXT("Error reading data block.\n") + Utf8ToWideChar(error_str);
						::MessageBox(NULL, error_str_w.data(), TEXT("Error"), MB_OK | MB_ICONERROR);
						goto ARCHIVE_FREE;
					}
					r = static_cast<int>(archive_write_data_block(ext, buff, size, offset));
					if (r != ARCHIVE_OK)
					{
						auto error_str = archive_error_string(ext);
						auto error_str_w = TEXT("Error writing data block.\n") + Utf8ToWideChar(error_str);
						::MessageBox(NULL, error_str_w.data(), TEXT("Error"), MB_OK | MB_ICONERROR);
						goto ARCHIVE_FREE;
					}
				}
			}
			r = archive_write_finish_entry(ext);
			if (r != ARCHIVE_OK)
			{
				auto error_str = archive_error_string(ext);
				auto error_str_w = TEXT("Error finishing entry.\n") + Utf8ToWideChar(error_str);
				::MessageBox(NULL, error_str_w.data(), TEXT("Error"), MB_OK | MB_ICONERROR);
				goto ARCHIVE_FREE;
			}
		}
		if (r != ARCHIVE_EOF)
		{
			auto error_str = archive_error_string(a);
			auto error_str_w = TEXT("Error reading archive.\n") + Utf8ToWideChar(error_str);
			::MessageBox(NULL, error_str_w.data(), TEXT("Error"), MB_OK | MB_ICONERROR);
			goto ARCHIVE_FREE;
		}
		result = true;
	ARCHIVE_FREE:
		archive_read_close(a);
		archive_read_free(a);
		archive_write_close(ext);
		archive_write_free(ext);
		return result;
	}

	void translateAll(const HWND scintilla_handle, TRANSLATE_MODE translate_mode)
	{
		int64_t numSelections = SciCall(scintilla_handle, SCI_GETSELECTIONS);

		/* 初始化Opencc並暫存 */
		if (converters[translate_mode] == nullptr)
		{
			std::wstring config_name_w = std::wstring(TRANSLATE_MODE_NAME[translate_mode]);
			std::string config_name = WideCharToUtf8(config_name_w.c_str()) + ".json";
			converters[translate_mode] = new
				opencc::SimpleConverter(config_name, std::vector{std::string(config_path)});
		}
		const auto Converter = converters[translate_mode];

		for (int64_t i = numSelections - 1; i >= 0; --i)
		{
#pragma region getAllSelectionsTexts
			/* get 文字選取範圍，起點與終點一致代表沒選字，跳過 */
			Sci_Position startPos = SciCall(scintilla_handle, SCI_GETSELECTIONNSTART, i);
			Sci_Position endPos = SciCall(scintilla_handle, SCI_GETSELECTIONNEND, i);
			if (startPos == endPos)
			{
				continue;
			}

			/* get 選取的文字 */
			Sci_Position length = endPos - startPos;
			std::string selectedText(static_cast<size_t>(length), '\0');
			Sci_TextRangeFull trf{};
			trf.chrg.cpMin = startPos;
			trf.chrg.cpMax = endPos;
			trf.lpstrText = selectedText.data(); // C++11 以後保證 string 內部是連續記憶體
			SciCall(scintilla_handle, SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<sptr_t>(&trf));

#pragma endregion
			/* 翻譯後替換掉原本的文字 */
			std::string convertedText = Converter->Convert(selectedText);
			Sci_CharacterRangeFull& targetRange = trf.chrg;
			SciCall(scintilla_handle, SCI_SETTARGETRANGE, targetRange.cpMin, targetRange.cpMax);
			SciCall(scintilla_handle, SCI_REPLACETARGET, convertedText.length(),
			        reinterpret_cast<sptr_t>(convertedText.c_str()));
		}
	}
}
