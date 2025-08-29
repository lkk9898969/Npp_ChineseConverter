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

#include <string>
#include <vector>

#include "BasicInclude.hpp"
#include "opencc.h"
//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

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
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
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
    HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;

    // Say hello now :
    // Scintilla control has no Unicode mode, so we use (char *) here
    ::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");
}

void helloDlg()
{
    ::MessageBox(NULL, TEXT("Hello, Notepad++!"), TEXT("Notepad++ Plugin Template"), MB_OK);
}


// 一個輔助函式，用來向 Scintilla 控制項傳送訊息
// 這是與 Scintilla 互動的核心
sptr_t SciCall(HWND hwnd, UINT msg, uptr_t wParam = 0, sptr_t lParam = 0) {
    return SendMessage(hwnd, msg, wParam, lParam);
}

/**
 * @brief 回傳 Scintilla 控制項中所有選擇的文字
 * @param scintilla_handle 指向 Scintilla 編輯器視窗的 HWND 控制代碼
 */
std::vector<Sci_TextRangeFull_cpp> getAllSelectionsTexts(HWND scintilla_handle) {
    int64_t numSelections = SciCall(scintilla_handle, SCI_GETSELECTIONS);
    std::vector<Sci_TextRangeFull_cpp> results(numSelections);

    for (int64_t i = numSelections - 1; i >= 0; --i) {
        // 2. 取得第 i 個選擇區的起始和結束位置
        Sci_Position startPos = SciCall(scintilla_handle, SCI_GETSELECTIONNSTART, i);
        Sci_Position endPos = SciCall(scintilla_handle, SCI_GETSELECTIONNEND, i);

        if (startPos == endPos) {
            continue;
        }

        // 3. 取得這個選擇區內的文字 (使用 FULL 版本)
        Sci_Position length = endPos - startPos;
        // 使用 std::string 來自動管理記憶體，更安全
        std::string selectedText(static_cast<size_t>(length), '\0');

        Sci_TextRangeFull trf{};
        trf.chrg.cpMin = startPos;
        trf.chrg.cpMax = endPos;
        trf.lpstrText = &selectedText[0]; // C++11 以後保證 string 內部是連續記憶體

        SciCall(scintilla_handle, SCI_GETTEXTRANGEFULL, 0, reinterpret_cast<sptr_t>(&trf));

        Sci_TextRangeFull_cpp trf_cpp{ std::move(trf.chrg) , std::move(selectedText) };

		results[i] = std::move(trf_cpp);
    }
	return results;
}

void setAllSelectionsTexts(HWND scintilla_handle, const std::vector<Sci_TextRangeFull_cpp>& texts) {
    int64_t numSelections = SciCall(scintilla_handle, SCI_GETSELECTIONS);
    if (numSelections != static_cast<int>(texts.size())) {
        // 選擇區數量不匹配，無法設定
        return;
    }
    // 反向處理選擇區，避免影響後續選擇區的位置
    for (int64_t i = numSelections - 1; i >= 0; --i) {
        const auto& trf_cpp = texts[i];
        Sci_Position startPos = trf_cpp.chrg.cpMin;
        Sci_Position endPos = trf_cpp.chrg.cpMax;
        if (startPos == endPos) {
            continue;
        }
        // 5. 設定「目標」(Target) 為 64-bit 範圍
        Sci_CharacterRangeFull targetRange{};
        targetRange.cpMin = startPos;
        targetRange.cpMax = endPos;
        SciCall(scintilla_handle, SCI_SETTARGETRANGE, 0, reinterpret_cast<sptr_t>(&targetRange));

        // 6. 執行替換
        SciCall(scintilla_handle, SCI_REPLACETARGET, trf_cpp.lpstr.length(),
            reinterpret_cast<sptr_t>(trf_cpp.lpstr.c_str()));
    }
}