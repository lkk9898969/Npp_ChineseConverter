# Npp Chinese Converter - Notepad++ 中文轉換器

使用 [OpenCC](https://github.com/BYVoid/OpenCC) 於 Notepad++ 裡進行繁簡轉換的插件。


## 📖 介紹

作者在網路上找不到有人幫 Notepad++ 製作繁簡轉換插件，因此決定自己動手開發。(經典程式人第一反應)  
目前功能仍然非常陽春，可能存在許多 BUG，請自行斟酌使用。

> [!IMPORTANT]  
> Notepad++ 版本需 v8.4.3 以上才可正常運作。  

> [!NOTE]  
> 作者是超級新手，專案結構非常陽春，手動編譯不保證一定能夠成功。  
> 歡迎前輩們提意見。

## 📦 安裝方式

於 Release 頁面下載預先編譯好的 DLL 檔案，或是自行依下方「編譯方式」進行編譯。接著依 [Notepad++ 官網的 Install plugin manually](https://npp-user-manual.org/docs/plugins/#install-plugin-manually) 步驟進行安裝。

1. 建立/進入 `Notepad++\plugins\NppChineseConverter` 資料夾（名稱需與 DLL 主檔名一致）。
2. 將 `NppChineseConverter.dll` 放入該資料夾中。
3. 重新啟動 Notepad++ 即可於「外掛模組」功能表看到 `Chinese Converter`。

> [!WARNING]  
> 可能會需要[Visual Studio 2022 Runtime](https://learn.microsoft.com/zh-tw/cpp/windows/latest-supported-vc-redist?view=msvc-170#visual-studio-2015-2022)*(aka Microsoft Visual C++ 2015-2022 可轉散發套件)*


## 🚀 使用方式

1. 在 Notepad++ 文件內 **選取欲轉換的文字**  
   - 支援多重選取
2. 開啟 `外掛模組 -> Chinese Converter`  
3. 選擇你要的動作（如：簡轉繁、繁轉簡）即可完成。  

## 🔨 編譯方式

### 依賴項目(子模組，已在專案中)
- [OpenCC](https://github.com/BYVoid/OpenCC)
- [libarchive](https://github.com/libarchive/libarchive)
- [liblzma](https://github.com/ShiftMediaProject/liblzma) (這邊使用[Shift Media Project](https://github.com/ShiftMediaProject)修改後的`liblzma`以便於用`CMake`配置與編譯)

### 環境需求
- [Visual Studio 2022](https://visualstudio.microsoft.com/) 以及其 C++ 桌面開發套件
- `Windows 11 SDK (10.0.22621.0)` (作者使用的版本)  
- [git](https://git-scm.com/downloads/win) 與 [cmake](https://cmake.org/)，且於你的`PATH`環境變數中
- [Python](https://www.python.org/)（用於腳本處理）
- [7-Zip](https://www.7-zip.org/) (用於打包OpenCC字典)

**備註**：請確保 `git`, `cmake`, `Python` 的執行檔路徑都已加入到系統 `PATH` 環境變數中  
你可以透過`-DSEVENZIP_EXECUTABLE`提供7z程式的路徑

### 步驟
1. 配置(Configure)  
```bash
cmake -Bbuild -G"Visual Studio 17 2022" -A x64
```
> [!NOTE]  
> 可以透過提供  
> -DUSE_SYSTEM_LIBLZMA=ON  
> -DUSE_SYSTEM_LIBARCHIVE=ON  
> 使用已安裝好的依賴函式庫(例如使用vcpkg預先安裝)  
> 當USE_SYSTEM_LIBARCHIVE為ON時USE_SYSTEM_LIBLZMA選項無效。

> [!WARNING]  
> 當使用USE_SYSTEM_LIBARCHIVE時請確保系統的libarchive有啟用lzma！  
> 否則編譯出的dll將無法正常解壓縮字典檔案。
2. 編譯(Build)
```bash
cmake --build build --config Release --target NppChineseConverter
```

## 🔄 更新子模組或字典
若需更新任何子模組或 OpenCC 字典：
  1. 進入對應的子模組資料夾。
  2. 使用 `git pull origin master` 或 `git checkout <tag>` 切換到想要的版本。
  3. 回到專案根目錄。
  4. 重新執行**編譯步驟 `1` 和 `2`**(建議刪除 `build` 資料夾)。  
  
  （理論上這樣就可以完成更新 😅）
