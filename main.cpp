//Credits to the following:
//Scanti - His SoundCommands plugin was used as a base for this one.
//  While barely any code from his plugin is used in mine, it provided
//  me with the orientation I needed to get started.  He also provided
//  me with some help when I asked.  Thanks alot!
//Kumar Prabhu - The DS Player sample application from CodeProject gave me
//  my orientation into DirectShow.  Hope my implimentation is alright.
//
//And numerous other places and people around the net as I Googled my problems.

//Known Issues:
//fMusicVolume (from SoundStruct) keeps  changing, fading out, then back to full
//  blast.  It is not stable for use.  Anything that relies on this variable will
//  likely have a fit.
//The music player does not pause when a movie, such as the Opening Movie
//  after starting a new game, plays.  This should be no sweat to fix.
//  Just need to find the point in code where BINKs are started and stopped.
//  Okay, maybe it is a little harder than I say.  :P  Anyone have any ideas
//  that will get me along faster?

//Known Non-Issues
//When you save a game, the current music type is saved with it.  If you die,
//  no music will play unless you're loading a save game that has that music
//  data saved into the save game, or your save requires a loadscreen to
//  complete.  So, when you first use this plugin, you might notice that after
//  you die, no music will play even after loading a save game.  In order to
//  fix this, just make a save and load from it when you die instead.

//Standard includes.
#include "process.h"
#include <string>
#include <unordered_map>

//OBSE includes
#include "PluginAPI.h"
#include "GameAPI.h"
#include "CommandTable.h"

//Project includes
#include "Globals.h"
#include "SafeWrite/SafeWrite.h"
#include "Multiplier.h"
#include "MusicType.h"
#include "MusicState.h"
#include "Hooks.h"
#include "Commands.h"
#include "EMCthread.h"
#include "OBSEInterfaces.h"
#include "TimeManagement.h"



using namespace std;



IDebugLog gLog("enhanced_music_control_2.log");



//EMCT - World MusicType playing at time of save.
//EMCV - The Relative Volume.
static void EMC2_SaveCallback(void *reserved) {
	_MESSAGE("%lld | Event >> SaveGame", timeStamp);
	WaitForSingleObject (hMusicStateMutex, INFINITE);
		MusicType worldType = musicState.getWorldType();
	ReleaseMutex (hMusicStateMutex);

	string dataString = to_string (worldType);
	g_serialization->OpenRecord('EMCT', 0);
	g_serialization->WriteRecordData(dataString.c_str(), dataString.length());
}



static void EMC2_LoadCallback(void *reserved) {
	UInt32	type, version, length;

	_MESSAGE ("%lld | Event >> LoadGame", timeStamp);
	char	buf[512];
	while (g_serialization->GetNextRecordInfo(&type, &version, &length))
	{
		//_MESSAGE("record %08X (%.4s) %08X %08X", type, &type, version, length);

		switch (type)
		{
		case 'EMCT':
			g_serialization->ReadRecordData(buf, length);
			buf[length] = 0;
			WaitForSingleObject (hMusicStateMutex, INFINITE);
				musicState.setWorldType (static_cast<MusicType>(atoi (buf)));
				musicState.setEventType (MusicType::mtNotKnown);
			ReleaseMutex (hMusicStateMutex);
			_MESSAGE ("%lld | Event >> LoadGame >> World music: %s", timeStamp, buf);
			break;
		}
	}

	for (MultipliersMap::iterator it = multipliersCustom.begin (); it != multipliersCustom.end (); it++) {
		Multiplier& mult = it->second;
		WaitForSingleObject (mult.hThread, INFINITE);
			if (!mult.isDestroyed && !mult.saveSession) {
				mult.isDestroyed = true;
				_MESSAGE ("%lld | Event >> LoadGame >> Multiplier destroyed: %s", timeStamp, it->first);
			}
		ReleaseMutex (mult.hThread);
	}
	recalculateMultipliers = true;
	musicState.reloadBattleState ();
}



static void EMC2_NewGameCallback(void *reserved) {
	_MESSAGE ("%lld | Event >> NewGame", timeStamp);
	WaitForSingleObject (hMusicStateMutex, INFINITE);
		musicState.setWorldType (MusicType::mtDungeon);			//Should be fine for your vanilla Oblivion.
		musicState.setEventType (MusicType::mtNotKnown);
	ReleaseMutex (hMusicStateMutex);

	for (MultipliersMap::iterator it = multipliersCustom.begin (); it != multipliersCustom.end (); it++) {
		Multiplier *mult = &it->second;
		WaitForSingleObject (mult->hThread, INFINITE);
			if (!mult->isDestroyed && !mult->saveSession) {
				mult->isDestroyed = true;
				_MESSAGE ("%lld | Event >> NewGame >> Multiplier destroyed: %s", timeStamp, it->first);
			}
		ReleaseMutex (it->second.hThread);
	}
	recalculateMultipliers = true;
	musicState.reloadBattleState ();
}



extern "C" {

	bool OBSEPlugin_Query(const OBSEInterface *obse, PluginInfo *info) {
		_MESSAGE("EMC2 >> Query");

		// fill out the info structure
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "enhanced_music_control_2";
		info->version = 121;

		// version checks
		if (!obse->isEditor) {
			if (obse->obseVersion < OBSE_VERSION_INTEGER) {
				_ERROR("EMC2 >> OBSE version too old (got %08X expected at least %08X)", obse->obseVersion, OBSE_VERSION_INTEGER);
				return false;
			} else if (obse->oblivionVersion != OBLIVION_VERSION) {
				_ERROR("EMC2 >> Incorrect Oblivion version (got %08X need %08X)", obse->oblivionVersion, OBLIVION_VERSION);
				return false;
			}

			g_serialization = (OBSESerializationInterface *)obse->QueryInterface(kInterface_Serialization);
			if (!g_serialization) {
				_ERROR("EMC2 >> Serialization interface not found");
				return false;
			} else if (g_serialization->version < OBSESerializationInterface::kVersion) {
				_ERROR("EMC2 >> Incorrect serialization interface version found (got %08X need %08X)", g_serialization->version, OBSESerializationInterface::kVersion);
				return false;
			}

			g_stringIntfc = (OBSEStringVarInterface*)obse->QueryInterface(kInterface_StringVar);
			if (!g_stringIntfc) {
				_ERROR("EMC2 >> String interface not found");
				return false;
			} else if (g_stringIntfc->version < OBSEStringVarInterface::kVersion) {
				_ERROR ("EMC2 >> Incorrect string interface version found (got %08X need %08X)", g_stringIntfc->version, OBSEStringVarInterface::kVersion);
				return false;
			}

			g_arrayIntfc = (OBSEArrayVarInterface *)obse->QueryInterface (kInterface_ArrayVar);
			if (!g_arrayIntfc) {
				_ERROR ("EMC2 >> Array interface not found");
				return false;
			}
		}
		return true;
	}



	bool OBSEPlugin_Load(const OBSEInterface *obse) {
		_MESSAGE("EMC2 >> Load");

		RegisterStringVarInterface (g_stringIntfc);

		/***************************************************************************
		*
		*	READ THIS!
		*
		*	Before releasing your plugin, you need to request an opcode range from
		*	the OBSE team and set it in your first SetOpcodeBase call. If you do not
		*	do this, your plugin will create major compatibility issues with other
		*	plugins, and may not load in future versions of OBSE. See
		*	obse_readme.txt for more information.
		*
		**************************************************************************/

		if (obse->oblivionVersion != 0x010201A0) {
			if (obse->isEditor) {
				_MESSAGE ("EMC2 >> Using the construction set...");
			} else {
				_MESSAGE ("EMC2 >> Unsupported Oblivion version, will disable commands.");
			}
			return true;	//Oblivion version is not supported. Stop right there.
		}

		//Register commands
		//I was assigned the following opCode ranges:  0x24C0-0x24DF, 0x2840-0x284F
		obse->SetOpcodeBase(0x24C0);
		obse->RegisterCommand		(&kGetMusicTypeCommand);						//24C0
		obse->RegisterCommand		(&kSetMusicTypeCommand);						//24C1
		obse->RegisterCommand		(&kCreatePlaylistCommand);						//24C2
		obse->RegisterCommand		(&kAddPathToPlaylistCommand);					//24C3
		obse->RegisterCommand		(&kPlaylistExistsCommand);						//24C4
		obse->RegisterCommand		(&kIsPlaylistActiveCommand);					//24C5
		obse->RegisterCommand		(&kSetPlaylistCommand);							//24C6
		obse->RegisterCommand		(&kRestorePlaylistCommand);						//24C7
		obse->RegisterCommand		(&kIsMusicSwitchingCommand);					//24C8
		obse->RegisterTypedCommand	(&kGetAllPlaylistsCommand, kRetnType_Array);	//24C9
		obse->RegisterTypedCommand	(&kGetPlaylistCommand, kRetnType_String);		//24CA
		obse->RegisterTypedCommand	(&kGetPlaylistTracksCommand, kRetnType_Array);	//24CB
		obse->RegisterTypedCommand	(&kGetTrackCommand, kRetnType_String);		//24CC
		obse->RegisterCommand		(&kGetTrackDurationCommand);					//24CD
		obse->RegisterCommand		(&kGetTrackPositionCommand);					//24CE
		obse->RegisterCommand		(&kSetTrackPositionCommand);					//24CF
		obse->RegisterCommand		(&kIsBattleOverriddenCommand);					//24D0
		obse->RegisterCommand		(&kSetBattleOverrideCommand);					//24D1
		obse->RegisterCommand		(&kIsMusicOnHoldCommand);						//24D2
		obse->RegisterCommand		(&kSetMusicHoldCommand);						//24D3
		obse->RegisterCommand		(&kGetMasterVolumeCommand);						//24D4
		obse->RegisterCommand		(&kSetMasterVolumeCommand);						//24D5
		obse->RegisterCommand		(&kGetMusicVolumeCommand);						//24D6
		obse->RegisterCommand		(&kSetMusicVolumeCommand);						//24D7
		obse->RegisterCommand		(&kGetEffectsVolumeCommand);					//24D8
		obse->RegisterCommand		(&kSetEffectsVolumeCommand);					//24D9
		obse->RegisterCommand		(&kGetFootVolumeCommand);						//24DA
		obse->RegisterCommand		(&kSetFootVolumeCommand);						//24DB
		obse->RegisterCommand		(&kGetVoiceVolumeCommand);						//24DC
		obse->RegisterCommand		(&kSetVoiceVolumeCommand);						//24DD
		obse->RegisterCommand		(&kGetMusicSpeedCommand);						//28DE
		obse->RegisterCommand		(&kSetMusicSpeedCommand);						//28DF

		obse->SetOpcodeBase(0x2840);
		obse->RegisterCommand		(&kGetPauseTimeCommand);						//2440
		obse->RegisterCommand		(&kSetPauseTimeCommand);						//2441
		obse->RegisterCommand		(&kGetFadeTimeCommand);							//2842
		obse->RegisterCommand		(&kSetFadeTimeCommand);							//2843
		obse->RegisterCommand		(&kGetStartBattleDelayCommand);					//2844
		obse->RegisterCommand		(&kSetStartBattleDelayCommand);					//2845
		obse->RegisterCommand		(&kGetStopBattleDelayCommand);					//2846
		obse->RegisterCommand		(&kSetStopBattleDelayCommand);					//2847
		obse->RegisterCommand		(&kGetMusicRememberTimeCommand);				//2848
		obse->RegisterCommand		(&kSetMusicRememberTimeCommand);				//2849
		obse->RegisterCommand		(&kPlayTrackCommand);							//244A
		obse->RegisterCommand		(&kMusicStopCommand);							//284B
		obse->RegisterCommand		(&kMusicPauseCommand);							//284C
		obse->RegisterCommand		(&kMusicResumeCommand);							//284D
		obse->RegisterCommand		(&kMusicRestartCommand);						//284E
		obse->RegisterCommand		(&kMusicNextTrackCommand);						//284F



		// set up serialization callbacks when running in the runtime
		if (!obse->isEditor) {
			// NOTE: SERIALIZATION DOES NOT WORK USING THE DEFAULT OPCODE BASE IN RELEASE BUILDS OF OBSE
			// it works in debug builds
			g_pluginHandle = obse->GetPluginHandle ();
			g_serialization->SetSaveCallback (g_pluginHandle, EMC2_SaveCallback);
			g_serialization->SetLoadCallback (g_pluginHandle, EMC2_LoadCallback);
			g_serialization->SetNewGameCallback (g_pluginHandle, EMC2_NewGameCallback);

			//Override game's music system.
			/*UInt32 opCode = g_scriptCommands.GetByName ("StreamMusic")->opcode;
			CommandInfo newStreamMusic = kPlayTrackCommand;
			g_scriptCommands.Replace (opCode, &newStreamMusic);*/

			//Patch the StreamMusic command.
			WriteRelCall (0x005096CA, (UInt32)&StreamMusicType);		//"Random"
			WriteRelCall (0x005096F2, (UInt32)&StreamMusicType);		//"Explore"
			WriteRelCall (0x0050971B, (UInt32)&StreamMusicType);		//"Public"
			WriteRelCall (0x00509741, (UInt32)&StreamMusicType);		//"Dungeon"
			WriteRelCall (0x00509767, (UInt32)&StreamMusicType);		//"Battle"
			WriteRelCall (0x00509779, (UInt32)&StreamMusicFile);		//"Pathname"
			WriteRelCall (0x00509780, (UInt32)&StreamMusicFileDoNothing);	//"Return"

			//Block the vanilla behaviour
			WriteRelJump (0x006AB160, (UInt32)&QueueMusicTrack);
			WriteRelJump (0x006AB420, (UInt32)&PlayQueuedMusic);

			//Detect the different music states.
			WriteRelJump (0x005B5B68, (UInt32)&DetectTitleMusic);
			WriteRelJump (0x005AD098, (UInt32)&DetectSuccessMusic);
			WriteRelJump (0x0066068B, (UInt32)&DetectBattleMusic);
			WriteRelJump (0x00660697, (UInt32)&DetectNotBattleMusic_1);
			WriteRelJump (0x00660691, (UInt32)&DetectNotBattleMusic_2);
			WriteRelJump (0x006AC026, 0x006AC056);	// Stop Oblivion from writing current volume levels to INI. The audio menu does it anyway.

			_beginthread (EMCthread, 0, nullptr);
		}
		
		return true;
	}

};