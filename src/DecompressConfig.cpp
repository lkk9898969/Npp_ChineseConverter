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

#include <shlwapi.h>
#include <string>
#include <vector>
#include <windows.h>

#include "7z.h"
#include "7zAlloc.h"
#include "7zCrc.h"
#include "7zVersion.h"

#include "opencc_archive.h"

// Link with Shlwapi.lib
#pragma comment(lib, "Shlwapi.lib")

namespace NppChineseConverter {

	// Memory Input Stream
	struct CMemLookInStream {
		ILookInStream vt;
		const Byte* buf;
		size_t bufSize;
		size_t pos;
	};

	SRes MemLook(ILookInStreamPtr pp, const void** buf, size_t* size) {
		CMemLookInStream* p = (CMemLookInStream*)pp;
		if (p->pos >= p->bufSize) {
			*size = 0;
			*buf = NULL;
			return SZ_OK;
		}
		*size = p->bufSize - p->pos;
		*buf = p->buf + p->pos;
		return SZ_OK;
	}

	SRes MemSkip(ILookInStreamPtr pp, size_t offset) {
		CMemLookInStream* p = (CMemLookInStream*)pp;
		if (p->pos + offset > p->bufSize)
			offset = p->bufSize - p->pos;
		p->pos += offset;
		return SZ_OK;
	}

	SRes MemRead(ILookInStreamPtr pp, void* buf, size_t* size) {
		CMemLookInStream* p = (CMemLookInStream*)pp;
		size_t rem = p->bufSize - p->pos;
		if (*size > rem)
			*size = rem;
		if (*size == 0)
			return SZ_OK;
		memcpy(buf, p->buf + p->pos, *size);
		p->pos += *size;
		return SZ_OK;
	}

	SRes MemSeek(ILookInStreamPtr pp, Int64* pos, ESzSeek origin) {
		CMemLookInStream* p = (CMemLookInStream*)pp;
		Int64 newPos = 0;
		switch (origin) {
		case SZ_SEEK_SET:
			newPos = *pos;
			break;
		case SZ_SEEK_CUR:
			newPos = p->pos + *pos;
			break;
		case SZ_SEEK_END:
			newPos = p->bufSize + *pos;
			break;
		}
		if (newPos < 0)
			newPos = 0;
		if ((size_t)newPos > p->bufSize)
			newPos = p->bufSize;
		p->pos = (size_t)newPos;
		*pos = newPos;
		return SZ_OK;
	}

	void MemLookInStream_Init(CMemLookInStream* p, const Byte* buf, size_t bufSize) {
		p->vt.Look = MemLook;
		p->vt.Skip = MemSkip;
		p->vt.Read = MemRead;
		p->vt.Seek = MemSeek;
		p->buf = buf;
		p->bufSize = bufSize;
		p->pos = 0;
	}

	// Allocator
	static ISzAlloc g_Alloc = { SzAlloc, SzFree };

	std::wstring GetErrorMessage(SRes res) {
		switch (res) {
		case SZ_OK:
			return L"No Error";
		case SZ_ERROR_DATA:
			return L"Data Error";
		case SZ_ERROR_MEM:
			return L"Memory Error";
		case SZ_ERROR_CRC:
			return L"CRC Error";
		case SZ_ERROR_UNSUPPORTED:
			return L"Unsupported Error";
		case SZ_ERROR_PARAM:
			return L"Param Error";
		case SZ_ERROR_INPUT_EOF:
			return L"Input EOF";
		case SZ_ERROR_OUTPUT_EOF:
			return L"Output EOF";
		case SZ_ERROR_READ:
			return L"Read Error";
		case SZ_ERROR_WRITE:
			return L"Write Error";
		case SZ_ERROR_PROGRESS:
			return L"Progress Error";
		case SZ_ERROR_FAIL:
			return L"Fail Error";
		case SZ_ERROR_THREAD:
			return L"Thread Error";
		case SZ_ERROR_ARCHIVE:
			return L"Archive Error";
		case SZ_ERROR_NO_ARCHIVE:
			return L"No Archive Error";
		}
		return L"Unknown Error";
	}

	bool DecompressConfigs(const wchar_t* destDir) {
		CMemLookInStream lookStream;
		MemLookInStream_Init(&lookStream, opencc_data, opencc_data_len);

		CrcGenerateTable();

		CSzArEx db;
		SzArEx_Init(&db);

		SRes res = SzArEx_Open(&db, &lookStream.vt, &g_Alloc, &g_Alloc);
		if (res != SZ_OK) {
			std::wstring msg = L"Failed to open archive: " + GetErrorMessage(res);
			::MessageBox(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
			SzArEx_Free(&db, &g_Alloc);
			return false;
		}

		UInt32 blockIndex = 0xFFFFFFFF;
		Byte* outBuffer = 0;
		size_t outBufferSize = 0;

		for (UInt32 i = 0; i < db.NumFiles; i++) {
			size_t offset = 0;
			size_t outSizeProcessed = 0;

			bool isDir = SzArEx_IsDir(&db, i);
			// Get Filename
			size_t len = SzArEx_GetFileNameUtf16(&db, i, NULL);
			// len includes null terminator
			if (len <= 1)
				continue;

			std::vector<UInt16> fileName(len);
			SzArEx_GetFileNameUtf16(&db, i, fileName.data());
			std::wstring wFileName((wchar_t*)fileName.data());

			// Construct full path
			// Ensure destDir doesn't end with backslash if wFileName starts with one?
			// Assuming simple concatenation.
			std::wstring fullPath = std::wstring(destDir) + L"\\" + wFileName;

			if (isDir) {
				// CreateDirectory handles existing directories gracefully usually?
				// Windows CreateDirectory fails if it exists.
				if (!::PathFileExists(fullPath.c_str())) {
					::CreateDirectory(fullPath.c_str(), NULL);
				}
				continue;
			}

			// Ensure parent dir exists (if 7z entries order wasn't dir first)
			wchar_t parent[MAX_PATH];
			lstrcpyW(parent, fullPath.c_str());
			::PathRemoveFileSpec(parent);
			if (!::PathFileExists(parent)) {
				// Basic recursive create might be needed if deeply nested and dirs not in
				// archive But usually archives contain dirs. For safety, assume simple
				// structure or dirs present.
				::CreateDirectory(parent, NULL);
			}

			res = SzArEx_Extract(&db, &lookStream.vt, i, &blockIndex, &outBuffer, &outBufferSize, &offset, &outSizeProcessed, &g_Alloc, &g_Alloc);
			if (res != SZ_OK) {
				std::wstring msg =
					L"Failed to extract file " + wFileName + L": " + GetErrorMessage(res);
				::MessageBox(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
				break;
			}

			// Write to file
			HANDLE hFile = ::CreateFile(fullPath.c_str(), GENERIC_WRITE, 0, NULL,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE) {
				std::wstring msg = L"Failed to create file: " + fullPath;
				::MessageBox(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
				res = SZ_ERROR_FAIL;
				break;
			}

			DWORD written;
			if (!WriteFile(hFile, outBuffer + offset, (DWORD)outSizeProcessed, &written, NULL) || written != outSizeProcessed) {
				std::wstring msg = L"Failed to write file: " + fullPath;
				::MessageBox(NULL, msg.c_str(), L"Error", MB_OK | MB_ICONERROR);
				CloseHandle(hFile);
				res = SZ_ERROR_FAIL;
				break;
			}
			CloseHandle(hFile);
		}

		SzFree(NULL, outBuffer);
		SzArEx_Free(&db, &g_Alloc);

		return res == SZ_OK;
	}
} // namespace NppChineseConverter