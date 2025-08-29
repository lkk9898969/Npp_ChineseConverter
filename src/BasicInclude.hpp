#pragma once

#include <string>

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "Scintilla.h"
#include "Notepad_plus_msgs.h"

extern FuncItem funcItem[nbFunc];
extern NppData nppData;

struct Sci_TextRangeFull_cpp {
	struct Sci_CharacterRangeFull chrg;
	std::string lpstr;
};