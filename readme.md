# Npp Chinese Converter - Notepad++ 中文轉換器

使用 [OpenCC](https://github.com/BYVoid/OpenCC) 於 Notepad++ 裡進行繁簡轉換的插件。


## 📖 介紹

作者在網路上找不到有針對 Notepad++ 製作的內建繁簡轉換插件，因此決定自己動手開發。  
目前功能仍然非常陽春，可能存在許多 BUG，請自行斟酌使用。

> ⚠️ **注意**  
> 作者是超級新手，專案結構非常陽春，手動編譯不保證一定能夠成功。


## 🚀 使用方式

1. 在 Notepad++ 文件內 **選取欲轉換的文字**  
   - 支援多重選取
2. 開啟 `外掛模組 -> Chinese Converter`  
3. 選擇你要的動作（如：簡轉繁、繁轉簡）即可完成。  


## 🔨 編譯方式

### 環境需求
- [Visual Studio 2022](https://visualstudio.microsoft.com/)  
- C++ 桌面開發套件  
- Windows 11 SDK (10.0.22621.0) (作者使用的版本)  
- [Python](https://www.python.org/)（用於腳本處理）

### 步驟
1. 執行根目錄下的 `Configure.bat`
2. 使用 Python 執行 `scripts/copy_dicts.py`  
   - 此步驟會於根目錄產生 `dict` 資料夾  
3. 將所有`dict`內的檔案壓縮成 `opencc.7z`
   - 請注意`opencc.7z`內不要含有任何資料夾，所有檔案應在7z的根目錄。
4. 使用 Python 執行 `scripts/file_to_c_array.py`
5. 開啟 `ChineseConverter.sln` 解決方案並編譯  
   - `Release` 組態會於 `bin64` 資料夾內產生dll


## 🔄 更新子模組或字典

若需更新任何子模組或 OpenCC 字典：
1. 進入子模組資料夾
2. `checkout` 到所需的 branch
3. 再執行一次 `Configure.bat`  

（理論上這樣就可以完成更新 😅）

## 📜 授權
本專案採用 [GPL-2.0 授權條款](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html) 發佈。  
請在使用、修改或再發佈本專案時，遵守 GPL-2.0 的規範。
