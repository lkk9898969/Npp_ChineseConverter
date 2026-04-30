# Npp Chinese Converter - Notepad++ 繁簡轉換插件

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

> [!NOTE]  
> 在Notepad++ v8.9.2之後，可以從在Notepad++內的 `外掛-->管理外掛模組`內，  
> 搜尋`Chinese Converter`即可直接安裝。

於 Release 頁面下載預先編譯好的 DLL 檔案，或是自行依下方「編譯方式」進行編譯。接著依 [Notepad++ 官網的 Install plugin manually](https://npp-user-manual.org/docs/plugins/#install-plugin-manually) 步驟進行安裝。

1. 建立/進入 `Notepad++\plugins\NppChineseConverter` 資料夾（名稱需與 DLL 主檔名一致）。
2. 將 `NppChineseConverter.dll` 放入該資料夾中。
3. 重新啟動 Notepad++ 即可於「外掛模組」功能表看到 `Chinese Converter`。

> [!WARNING]  
> ARM64架構未經過任何測試，若使用上有遭遇問題歡迎回報。

> [!NOTE]  
> 可能會需要[Visual Studio Runtime v14](https://learn.microsoft.com/zh-tw/cpp/windows/latest-supported-vc-redist?view=msvc-170#visual-c-v14-redistributable)*(aka Microsoft Visual C++ 可轉散發套件)*


## 🚀 使用方式

1. 在 Notepad++ 文件內 **選取欲轉換的文字**  
   - 支援多重選取
2. 開啟 `外掛模組 -> Chinese Converter`  
3. 選擇你要的動作（如：簡轉繁、繁轉簡）即可完成。  

## 🔨 編譯方式

### 依賴項目(子模組)
- [OpenCC](https://github.com/BYVoid/OpenCC)
- [LZMA SDK](https://www.7-zip.org/sdk.html) (用於解壓縮字典)


### 環境需求
- [Visual Studio 2026](https://visualstudio.microsoft.com/) 以及其 C++ 桌面開發套件
- `Windows 11 SDK (10.0.26100.0)` (作者使用的版本)  
- [git](https://git-scm.com/downloads/win) 與 [cmake](https://cmake.org/)，且於你的`PATH`環境變數中
- [Python](https://www.python.org/)（用於腳本處理）
- [7-Zip](https://www.7-zip.org/) (用於打包OpenCC字典)

**備註**：請確保 `git`, `cmake`, `Python` 的執行檔路徑都已加入到系統 `PATH` 環境變數中  
你可以透過`-DSEVENZIP_EXECUTABLE`提供7z程式的路徑

### 步驟
1. 配置(Configure)
   
     ```bash
     cmake -Bbuild -G"Visual Studio 18 2026" -A x64
     ```
     如果是32位元架構(x86):
     ```bash
     cmake -Bbuild -G"Visual Studio 18 2026" -A Win32
     ```
     如果是ARM架構(ARM64):
     ```bash
     cmake -Bbuild -G"Visual Studio 18 2026" -A ARM64
     ```

2. 編譯(Build)

     ```bash
     cmake --build build --config Release --target NppChineseConverter
     ```

> [!WARNING]  
> ARM64架構目前是由amd64交叉編譯實現，由於OpenCC在編譯過程需要運行編譯出的執行檔，  
> 而在進行Cross Compile時會因為編譯出的執行檔是ARM架構無法執行導致編譯失敗，
> 因此目前作者先用暫時的patch workaround解決。

## 🔄 更新子模組或字典
若需更新任何子模組或 OpenCC 字典：
  1. 進入對應的子模組資料夾。
  2. 使用 `git pull origin master` 或 `git checkout <tag>` 切換到想要的版本。
  3. 回到專案根目錄。
  4. 重新執行**編譯步驟 `1` 和 `2`**(建議刪除 `build` 資料夾)。  
  
  （理論上這樣就可以完成更新 😅）
 
## License

This project is licensed under the GNU General Public License v3.0

See the LICENSE.txt and NOTICE.txt for details.

### License Notice

The author does not claim expertise in software licensing.
This project’s licensing information was prepared with the assistance of AI tools.

If you spot any errors in the copyright notices or license usage, your feedback is highly appreciated.