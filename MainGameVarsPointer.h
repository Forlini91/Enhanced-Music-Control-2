#pragma once

#include "MainGameVarsStruct.h"

struct MainGameVarsPointer {
	MainGameVarsStruct *gameVars;
};

extern MainGameVarsPointer *mainGameVars;