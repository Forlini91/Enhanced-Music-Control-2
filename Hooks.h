#pragma once


#include <time.h>
#include "Globals.h"
#include "MusicState.h"
#include "PlayMusicFile.h"



//Not needed, as the combat state is extracted directly from the code
static void *obGetIsInCombat = (void *)0x006605A0;

//Not needed, as the vanilla music player is not used
static void *obSetMusicVolume = (void *)0x006AA1A0;
static void *obQueueMusicTrack = (void *)0x006AB160;
static void *obPlayQueuedMusicTrack = (void *)0x006AB420;




static int *streamSelectedType = new int (0);
static char *streamSelectedText = new char[200];
//MusicType of the World (Explore, Public, or Dungeon)
//MusicTypes worldType = NotKnown;
static MusicType *worldType = musicState.getWorldTypePtr ();

//MusicType the game should be playing.
//MusicTypes currentDesiredMusicType = NotKnown;
static MusicType *eventType = musicState.getEventTypePtr ();

//Remember to change this before you change the above.
//Otherwise sentry thread may execute music twice.
//specialTypes specialType = Death;
static SpecialMusicType *specialType = musicState.getSpecialTypePtr ();






_declspec(naked) void QueueMusicTrack (void) {
	_asm {
			push eax
			push ecx
			push edx
	}
	WaitForSingleObject (hMusicStateMutex, INFINITE);
	_asm {
			pop edx
			pop ecx
			pop eax
			//
			push ebx
			mov eax, dword ptr[esp + 08h]		//Get the MusicType...
			cmp eax, mtSpecial					//If EAX != Special
				jne DoNormal
			//Else, the player may have died.  Go and do a state test.
			push eax							//MusicType: Special
			push ecx
			push esi
			//Pull up the player data.
			mov esi, 0x00B333C4					//Get the player data.
			mov esi, [esi]						//Player data continued.
			mov eax, dword ptr[esi]			//Get a class...?
			mov edx, [eax + 198h]					//Get a function of that class...
			push 01h							//Push a paramter.
			mov ecx, esi						//Place player data into ECX.
				call edx							//Call the function.  EAX now contains if the player is in combat.
			test al, al							//If EAX (the return value) != 0
			pop esi
			pop ecx
			mov ebx, specialType
				jnz PlayerDead						//If not in battle, then it must be dead (success is managed elsewhere). So jump to the dead music!
			mov[ebx], spNotKnown				//Set the special music to NotKnown.
			pop eax
				jmp DoNormal
		PlayerDead :
			mov[ebx], spDeath					//Set the special music to Death.
			pop eax
		DoNormal :
			mov ebx, worldType
			mov[ebx], eax						//and place it into our little variable.
			pop ebx
			//
			push eax
			push ecx
			push edx
	}
	ReleaseMutex (hMusicStateMutex);
	_asm {
			pop edx
			pop ecx
			pop eax
			mov eax, 01h						//Return true.
			retn 0Ch
	}
}



//Prevent the Oblivion music player from playing the music. The Direct Show player will replace it.
_declspec(naked) void PlayQueuedMusic (void) {
	_asm {
			mov eax, 00h
			retn
	}
}



//We've entered the title screen.  Let us know.
_declspec(naked) void DetectTitleMusic (void) {
	_asm {
			push eax
			push ecx
			push edx
	}
	WaitForSingleObject (hMusicStateMutex, INFINITE);
	_asm {
			pop edx
			pop ecx
			pop eax
			//
			push ebx
			mov ebx, specialType
			mov[ebx], spTitle					//Set the special music to Title.
			mov ebx, eventType
			mov[ebx], mtSpecial					//Set MusicType to Special.
			pop ebx
			//
			push eax
			push ecx
			push edx
	}
	ReleaseMutex (hMusicStateMutex);
	_asm {
			pop edx
			pop ecx
			pop eax
			add esp, 108h
			retn
	}
}



//We've entered the success screen.  Let us know.
_declspec(naked) void DetectSuccessMusic (void) {
	_asm {
			push eax
			push ecx
			push edx
	}
	WaitForSingleObject (hMusicStateMutex, INFINITE);
	_asm {
			pop edx
			pop ecx
			pop eax
			push ebx
			mov ebx, specialType
			mov[ebx], spSuccess					//Set the special music to Success.
			mov ebx, eventType
			mov[ebx], mtSpecial					//Set MusicType to Special.
			pop ebx
			push eax
			push ecx
			push edx
	}
	ReleaseMutex (hMusicStateMutex);
	_asm {
			pop edx
			pop ecx
			pop eax
			//
			pop ecx
			pop edi
			pop esi
			pop ebp
			pop ebx
			add esp, 14h
			retn
	}
}



_declspec(naked) void DetectBattleMusic (void) {
	_asm {
			push eax
			push ecx
			push edx
	}
	WaitForSingleObject (hMusicStateMutex, INFINITE);		//Affects EAX, ECX, and EDX
	_asm {
			pop edx
			pop ecx
			pop eax
			//Check to see if we're using the player's data.
			push ebx
			mov ebx, 0x00B333C4					//Get the player data.
			mov ebx, [ebx]						//Player data continued.
			cmp edi, ebx
				jne Abort
			//Else, it was the player's data.
			mov ebx, eventType
			mov[ebx], mtBattle					//Set MusicType to Battle.
		Abort :
			pop ebx
			push eax
			push ecx
			push edx
	}
	ReleaseMutex (hMusicStateMutex);
	_asm {
			pop edx
			pop ecx
			pop eax
			mov al, 01h							//return value = true
			pop edi
			retn 0004h
	}
}



_declspec(naked) void DetectNotBattleMusic_1 (void) {
	_asm {
			push eax
			push ecx
			push edx
	}
	WaitForSingleObject (hMusicStateMutex, INFINITE);
	_asm {
			pop edx
			pop ecx
			pop eax
			//Check to see if we're using the player's data.
			push ebx
			mov ebx, 0x00B333C4					//Get the player data.
			mov ebx, [ebx]						//Player data continued.
			cmp edi, ebx
				jne Abort
			//Else, it was the player's data.
			mov ebx, eventType
			mov[ebx], mtNotKnown				//Set MusicType to NotKnown.
		Abort :
			pop ebx
			push eax
			push ecx
			push edx
	}
	ReleaseMutex (hMusicStateMutex);
	_asm {
			pop edx
			pop ecx
			pop eax
			xor al, al							//Return value = false
			pop edi
			retn 0004h
	}
}



_declspec(naked) void DetectNotBattleMusic_2 (void) {
	_asm {
		push eax
			push ecx
			push edx
	}
	WaitForSingleObject (hMusicStateMutex, INFINITE);
	_asm {
			pop edx
			pop ecx
			pop eax
			//Check to see if we're using the player's data.
			push ebx
			mov ebx, 0x00B333C4					//Get the player data.
			mov ebx, [ebx]						//Player data continued.
			cmp edi, ebx
				jne Abort
			//Else, it was the player's data.
			mov al, cl							//return value = ...?
			test al, al							//If AL == 0
				jz NoBattle							//Then Abort
			//Else, we are in combat.
			mov ebx, eventType
			mov[ebx], mtBattle					//Set MusicType to Battle.
				jmp Abort
		NoBattle :
			mov ebx, eventType
			mov[ebx], mtNotKnown				//Set MusicType to NotKnown.
		Abort :
			pop ebx
			push eax
			push ecx
			push edx
	}
	ReleaseMutex (hMusicStateMutex);
	_asm {
			pop edx
			pop ecx
			pop eax
			mov al, cl							//return value = ...?
			pop edi
			retn 0004h
	}
}












_declspec(naked) void StreamMusicType (void) {
	_asm {
			push ebp
			mov ebp, esp
			pushad
			mov eax, [ebp + 8h]
			mov edx, streamSelectedType
			mov[edx], eax
	}

	_MESSAGE ("Command >> StreamMusic >> Music type: %d", *streamSelectedType);
	parsePlayTrackCommand (*streamSelectedType);

	_asm {
			popad
			mov esp, ebp
			pop ebp
			retn 00Ch
	}
}



_declspec(naked) void StreamMusicFile (void) {
	_asm {
		push ebp
			mov ebp, esp
			pushad
			mov eax, [ebp + 0Ch]
			mov streamSelectedText, eax
	}

	_MESSAGE ("Command >> StreamMusic >> %s", streamSelectedText);
	parsePlayTrackCommand (streamSelectedText);

	_asm {
		popad
			mov esp, ebp
			pop ebp
			ret 00Ch
	}
}



_declspec(naked) void StreamMusicFileDoNothing (void) {
	_asm{
		ret
	}
}