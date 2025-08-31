//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
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
#include "opencc_data.h"


static std::vector<Sci_TextRangeFull_cpp> getAllSelectionsTexts(const HWND scintilla_handle);
static std::vector<Sci_TextRangeFull_cpp> translateAllTexts(std::vector<Sci_TextRangeFull_cpp>& texts,
                                                            TRANSLATE_MODE translate_mode);
static void setAllSelectionsTexts(const HWND scintilla_handle, const std::vector<Sci_TextRangeFull_cpp>& texts);
static bool extractConfigs();
void translate(TRANSLATE_MODE);
//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

static wchar_t plugin_config_dir[MAX_PATH];

#pragma region Helper Functions

static __inline HWND getCurrentScintilla()
{
	// Get the current scintilla
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, reinterpret_cast<LPARAM>(&which));
	if (which == -1)
		return nullptr;
	HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
	return curScintilla;
}

static std::string WideCharToUtf8(const wchar_t* wstr)
{
	if (!wstr) return {};
	int len = ::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 0) return {};
	std::string result(len - 1, '\0'); // 不要包含結尾的 '\0'
	::WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), len, nullptr, nullptr);
	return result;
}

static std::wstring Utf8ToWideChar(const char* str)
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
bool setCommand(const size_t index, TCHAR* cmdName, const PFUNCPLUGINCMD pFunc, ShortcutKey* sk, const bool check0nInit)
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

#pragma endregion

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
	setCommand(0, TEXT("Hello Notepad++"), hello, NULL, false);
	setCommand(1, TEXT("Hello (with dialog)"), helloDlg, NULL, false);
	setCommand(2, TEXT("簡體到繁體"), []() { translate(TRANSLATE_MODE::SC2TC); }, NULL, false);
	setCommand(3, TEXT("重新載入翻譯設定"), []() { extractConfigs(); }, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void hello()
{
	// Open a new document
	::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

	// Get the current scintilla
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	if (which == -1)
		return;
	HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

	// Say hello now :
	// Scintilla control has no Unicode mode, so we use (char *) here
	::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");
}

void helloDlg()
{
	::MessageBox(NULL, TEXT("Hello, Notepad++!"), TEXT("Notepad++ Plugin Template"), MB_OK);
}

void reloadConfigs()
{
	if (extractConfigs())
	{
		::MessageBox(NULL, TEXT("重新載入成功。"), TEXT("成功"), MB_OK | MB_ICONINFORMATION);
	}
}

static bool extractConfigs()
{
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
		//::MessageBox(NULL, fullpath.data(), TEXT("Debug Message"), MB_OK);
		archive_entry_set_pathname(entry, WideCharToUtf8(fullpath.c_str()).c_str());
		//printf("%s\n", archive_entry_pathname(entry));
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

void translate(TRANSLATE_MODE translate_mode)
{
	// 1. 取得目前的 Scintilla 控制項
	HWND curScintilla = getCurrentScintilla();
	if (!curScintilla)
	{
		::MessageBox(NULL, TEXT("No active Scintilla control."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return;
	}
	// 2. 取得所有選擇區的文字
	auto selectedTexts = getAllSelectionsTexts(curScintilla);
	if (selectedTexts.empty())
	{
		::MessageBox(NULL, TEXT("No text selected."), TEXT("Info"), MB_OK | MB_ICONINFORMATION);
		return;
	}
	// 3. 轉換所有選擇區的文字
	auto translatedTexts = translateAllTexts(selectedTexts, translate_mode);
	if (translatedTexts.empty())
	{
		::MessageBox(NULL, TEXT("Translation failed."), TEXT("Error"), MB_OK | MB_ICONERROR);
		return;
	}
	// 4. 將轉換後的文字設定回 Scintilla 控制項
	setAllSelectionsTexts(curScintilla, translatedTexts);
}


// 一個輔助函式，用來向 Scintilla 控制項傳送訊息
// 這是與 Scintilla 互動的核心
static sptr_t SciCall(const HWND hwnd, const UINT msg, const uptr_t wParam = 0, const sptr_t lParam = 0)
{
	return SendMessage(hwnd, msg, wParam, lParam);
}

/**
 * @brief 回傳 Scintilla 控制項中所有選擇的文字
 * @param scintilla_handle 指向 Scintilla 編輯器視窗的 HWND 控制代碼
 */
static std::vector<Sci_TextRangeFull_cpp> getAllSelectionsTexts(const HWND scintilla_handle)
{
	int64_t numSelections = SciCall(scintilla_handle, SCI_GETSELECTIONS);
	std::vector<Sci_TextRangeFull_cpp> results(numSelections);

	for (int64_t i = numSelections - 1; i >= 0; --i)
	{
		// 2. 取得第 i 個選擇區的起始和結束位置
		Sci_Position startPos = SciCall(scintilla_handle, SCI_GETSELECTIONNSTART, i);
		Sci_Position endPos = SciCall(scintilla_handle, SCI_GETSELECTIONNEND, i);

		if (startPos == endPos)
		{
			continue;
		}

		// 3. 取得這個選擇區內的文字 (使用 FULL 版本)
		Sci_Position length = endPos - startPos;
		// 使用 std::string 來自動管理記憶體，更安全
		std::string selectedText(static_cast<size_t>(length), '\0');

		Sci_TextRangeFull trf{};
		trf.chrg.cpMin = startPos;
		trf.chrg.cpMax = endPos;
		trf.lpstrText = selectedText.data(); // C++11 以後保證 string 內部是連續記憶體

		SciCall(scintilla_handle, SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<sptr_t>(&trf));

		Sci_TextRangeFull_cpp trf_cpp{trf.chrg, std::move(selectedText)};

		results[i] = std::move(trf_cpp);
	}
	return results;
}


static std::vector<Sci_TextRangeFull_cpp> translateAllTexts(std::vector<Sci_TextRangeFull_cpp>& texts,
                                                            TRANSLATE_MODE translate_mode)
{
	std::vector<Sci_TextRangeFull_cpp> results;
	if (texts.empty())
	{
		return results;
	}
	// 1. 初始化 OpenCC
	std::wstring config_name_w = std::wstring(TRANSLATE_MODE_NAME[translate_mode]);
	// 改用 WideCharToUtf8
	std::string config_name = WideCharToUtf8(config_name_w.c_str());
	std::string config_path = WideCharToUtf8(plugin_config_dir);
	auto Converter = opencc::SimpleConverter(config_name, std::vector{std::string(config_path)});

	results.reserve(texts.size());
	// 2. 逐一轉換每個選擇區的文字
	for (auto& [chrg, lpstr] : texts)
	{
		std::string convertedText = Converter.Convert(lpstr);
		results.emplace_back(Sci_TextRangeFull_cpp{chrg, std::move(convertedText)});
	}
	return results;
}


static void setAllSelectionsTexts(const HWND scintilla_handle, const std::vector<Sci_TextRangeFull_cpp>& texts)
{
	int64_t numSelections = SciCall(scintilla_handle, SCI_GETSELECTIONS);
	if (numSelections != static_cast<int>(texts.size()))
	{
		// 選擇區數量不匹配，無法設定
		return;
	}
	// 反向處理選擇區，避免影響後續選擇區的位置
	for (int64_t i = numSelections - 1; i >= 0; --i)
	{
		const auto& [chrg, lpstr] = texts[i];
		Sci_Position startPos = chrg.cpMin;
		Sci_Position endPos = chrg.cpMax;
		if (startPos == endPos)
		{
			continue;
		}
		// 5. 設定「目標」(Target) 為 64-bit 範圍
		Sci_CharacterRangeFull targetRange{};
		targetRange.cpMin = startPos;
		targetRange.cpMax = endPos;
		SciCall(scintilla_handle, SCI_SETTARGETRANGE, 0, reinterpret_cast<sptr_t>(&targetRange));

		// 6. 執行替換
		SciCall(scintilla_handle, SCI_REPLACETARGET, lpstr.length(),
		        reinterpret_cast<sptr_t>(lpstr.c_str()));
	}
}
