#pragma once

#include "MusicType.h"
#include "C:\Program Files (x86)\Microsoft DirectX SDK (August 2008)\Include\dsound.h"
#include "dsound.h"

struct SoundStruct {
	int				Unknown000;						//000
	int				Unknown004;						//004
	LPDIRECTSOUND8	DSoundInterface;				//008
	int				Unknown00C[(0x70 - 0xC) / 4];		//00C
	LPVOID			*FilterGraphInterface;			//070
	int				Unknown074[(0xB0 - 0x74) / 4];		//074
	MusicType		musicType;						//0B0
	int				Unknown0B4;						//0B4
	float			fMasterVolume;					//0B8
	float			fFootVolume;					//0BC
	float			fVoiceVolume;					//0C0
	float			fEffectsVolume;					//0C4
	DWORD			dTickCount1;					//0C8
	DWORD			dTickCount2;					//0CC
	DWORD			dTickCount3;					//0D0
	int				Unknown0D4[(0xDC - 0xD4) / 4];		//0D4
	byte			Flags;							//0DC
	int				Unknown0F0[(0x1E4 - 0xE0) / 4];		//0E0
	char			MusicFilename[260];				//1E4
	int				Unknown2E8;						//2E8
	int				Unknown2EC;						//2EC
	float			fMusicVolume;					//2F0
	int				Unknown2F4;						//2F4
	float			fMusicVolumeDup;				//2F8
	int				Unknown2FC;						//2FC
	void			*TESGameSoundMap;				//300
	void			*NiAVObjectMap;					//304
	void			*SoundMessageList;				//308
	int				Unknown30C[(0x320 - 0x30C) / 4];	//30C
	void			*NiTList;						//320
	int				Unknown324;						//324
};