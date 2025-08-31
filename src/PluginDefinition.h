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

#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H

//
// All difinitions of plugin interface
//
#include "PluginInterface.h"

typedef enum TRANSLATE_MODE
{
	SC2TC = 1, // 簡體到繁體
	SC2TW, // 簡體到台灣正體
	SC2HK, // 簡體到香港繁體
	S2TWP, // 簡體到台灣正體(含用詞)
	TC2SC, // 繁體到簡體
	TC2TW, // 繁體到台灣正體
	TC2HK, // 繁體到香港繁體
	TW2SC, // 台灣正體到簡體
	TW2TC, // 台灣正體到繁體
	TW2SP, // 台灣正體到簡體(含用詞)
	HK2SC, // 香港繁體到簡體
	HK2TC, // 香港繁體到繁體
} TRANSLATE_MODE;

constexpr TCHAR TRANSLATE_MODE_NAME[][7]
{
	TEXT(""), // 0. 未使用
	TEXT("s2t"), // 簡體到繁體
	TEXT("s2tw"), // 簡體到台灣正體
	TEXT("s2hk"), // 簡體到香港繁體
	TEXT("s2twp"), // 簡體到台灣正體(含用詞)
	TEXT("t2s"), // 繁體到簡體
	TEXT("t2tw"), // 繁體到台灣正體
	TEXT("t2hk"), // 繁體到香港繁體
	TEXT("tw2s"), // 台灣正體到簡體
	TEXT("tw2t"), // 台灣正體到繁體
	TEXT("tw2sp"), // 台灣正體到簡體(含用詞)
	TEXT("hk2s"), // 香港繁體到簡體
	TEXT("hk2t"), // 香港繁體到繁體
};

//-------------------------------------//
//-- STEP 1. DEFINE YOUR PLUGIN NAME --//
//-------------------------------------//
// Here define your plugin name
//
const TCHAR NPP_PLUGIN_NAME[]
	= TEXT("Chinese Converter");

//-----------------------------------------------//
//-- STEP 2. DEFINE YOUR PLUGIN COMMAND NUMBER --//
//-----------------------------------------------//
//
// Here define the number of your plugin commands
//
const int nbFunc = 6;


//
// Initialization of your plugin data
// It will be called while plugin loading
//
void pluginInit(HANDLE hModule);

//
// Cleaning of your plugin
// It will be called while plugin unloading
//
void pluginCleanUp();

//
//Initialization of your plugin commands
//
void commandMenuInit();

//
//Clean up your plugin commands allocation (if any)
//
void commandMenuCleanUp();


#endif //PLUGINDEFINITION_H
