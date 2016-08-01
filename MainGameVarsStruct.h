#pragma once

#include "SoundStruct.h"

struct MainGameVarsStruct {
	byte		bHasQuitGame;			//00
	byte		bExitingToMainMenu;		//01
	byte		bUnknown1;				//02
	byte		bUnknown2;				//03
	byte		bUnknown3;				//04
	//0
	//0
	//0									
	HWND		GameWindow;				//08
	HINSTANCE	GameInstance;			//0C
	DWORD		GameMainThreadId;		//10
	LPHANDLE	GameProcessHandle;		//14
	int			iUnknown4;				//18
	int			iUnknown5;				//1C
	void		*InputStruct;			//20
	SoundStruct	*SoundRecords;			//24
};