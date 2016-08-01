//Credits to the following:
//Scanti - His SoundCommands plugin was used as a base for this one.
//  While barely any code from his plugin is used in mine, it provided
//  me with the orientation I needed to get started.  He also provided
//  me with some help when I asked.  Thanks alot!
//Kumar Prabhu - The DS Player sample application from CodeProject gave me
//  my orientation into DirectShow.  Hope my implimentation is allright.
//Peter Chen - For providing CPath for general non-profit use.
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
//The Success music does not stop playing until its played through totally
//  once.  I would like it to stop playing as soon as the levelup menu is
//  closed, instead, but I haven't figured out how to do it yet.

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
#include "obse/PluginAPI.h"
#include "obse/GameAPI.h"

//Project includes
#include "Hooks.h"
#include "MainThread.h"
#include "Commands.h"
#include "SafeWrite/SafeWrite.h"
//#include "MusicPathPointer.h"
#include "MusicPlayer.h"
#include "EMC2INISettings.h"
#include "MainGameVarsPointer.h"
#include "MusicState.h"
#include "Multiplier.h"
#include "GlobalSettings.h"

using namespace std;

//#define ENABLE_PLUGINTEST
//#define DISABLE_COMMANDS


IDebugLog gLog("enhanced_music_control_2.log");
PluginHandle				g_pluginHandle = kPluginHandle_Invalid;
OBSESerializationInterface	* g_serialization = NULL;
OBSEArrayVarInterface		* g_arrayIntfc = NULL;
OBSEStringVarInterface		* g_stringIntfc = NULL;
typedef OBSEArrayVarInterface::Array	OBSEArray;
HANDLE hMusicTypeMutex;		//"Music Type" mutex  Lock when changing anything in Music.
HANDLE hThePlayerMutex;		//"ThePlayer" Mutex.  Lock when using ThePlayer.
HANDLE hPlaylistMutex;		//"Playlist" Mutex.  Lock when manipulating any of the playlists.
HANDLE hSentryRequestMutex;	//"sentryRequest" Mutex.  Lock when asking SentryThread to do something.
MainGameVarsPointer *mainGameVars = (MainGameVarsPointer *)0x00B33398;

MusicState music;
MusicPlayer musicPlayer;
ThreadRequest threadRequest;
ThreadState threadState;
Playlist *plExplore;
Playlist *plPublic;
Playlist *plDungeon;
Playlist *plCustom;
Playlist *plBattle;
Playlist *plTitle;
Playlist *plDeath;
Playlist *plSuccess;

pair<MusicType, string> mapDataOrigNames[] = {
	make_pair (MusicType::Explore, "Explore"),
	make_pair (MusicType::Public, "Public"),
	make_pair (MusicType::Dungeon, "Dungeon"),
	make_pair (MusicType::Custom, "Custom"),
	make_pair (MusicType::Battle, "Battle"),
	make_pair (MusicType::Undefined, "Undefined"),
	make_pair (MusicType::Special, "Special"),
	make_pair (MusicType::Mt_NotKnown, "None")
};
pair<SpecialMusicType, string> mapDataOrigSpecNames[] = {
	make_pair (SpecialMusicType::Success, "Success"),
	make_pair (SpecialMusicType::Death, "Death"),
	make_pair (SpecialMusicType::Title, "Title"),
	make_pair (SpecialMusicType::Sp_NotKnown, "None")
};
pair<MusicType, Playlist**> mapDataPlaylists[] = {
	make_pair (MusicType::Explore, &plExplore),
	make_pair (MusicType::Public, &plPublic),
	make_pair (MusicType::Dungeon, &plDungeon),
	make_pair (MusicType::Custom, &plCustom),
	make_pair (MusicType::Battle, &plBattle)
};
pair<SpecialMusicType, Playlist**> mapDataSpecPlaylists[] = {
	make_pair (SpecialMusicType::Success, &plSuccess),
	make_pair (SpecialMusicType::Death, &plDeath),
	make_pair (SpecialMusicType::Title, &plTitle),
};
map <MusicType, string> musicTypes (mapDataOrigNames, mapDataOrigNames + 8);
map <SpecialMusicType, string> specialMusicTypes (mapDataOrigSpecNames, mapDataOrigSpecNames + 4);
map <MusicType, Playlist**> varsMusicType (mapDataPlaylists, mapDataPlaylists + 5);
map <SpecialMusicType, Playlist**> varsSpecialMusicType (mapDataSpecPlaylists, mapDataSpecPlaylists + 3);

unordered_map <string, Playlist> playlists;
unordered_map <string, Multiplier> multipliersVanilla;
unordered_map <string, Multiplier> multipliersCustom;


//This holds the names of playlist that are loaded.
//In the case our collection reallocates itself after a new
//playlist is added, this can be used to repair the bad references.
string vanillaPlaylistNames[] = {
	obExplore,
	obPublic,
	obDungeon,
	obCustom,
	obBattle
};

bool printNewTrack = false;
bool delayTitleMusicEnd = true;
int numPlaylists;
int numMultipliers;











Playlist* addPlaylist (const string& name, const string& paths, bool randomOrder, bool vanillaPlaylist) {
	auto insResult = playlists.emplace (BUILD_IN_PLACE(name, name, paths, randomOrder, vanillaPlaylist));
	return &insResult.first->second;
}



MusicType getAssignedMusicType (Playlist* playlist) {
	if (playlist == plExplore) {
		return MusicType::Explore;
	} else if (playlist == plPublic) {
		return MusicType::Public;
	} else if (playlist == plDungeon) {
		return MusicType::Dungeon;
	} else if (playlist == plCustom) {
		return MusicType::Custom;
	} else if (playlist == plBattle) {
		return MusicType::Battle;
	} else if (playlist == plSuccess || playlist == plDeath || playlist == plTitle) {
		return MusicType::Special;
	} else {
		return MusicType::Mt_NotKnown;
	}
}

SpecialMusicType getAssignedSpecialMusicType (Playlist* playlist) {
	if (playlist == plSuccess) {
		return SpecialMusicType::Success;
	} else if (playlist == plDeath) {
		return SpecialMusicType::Death;
	} else if (playlist == plTitle) {
		return SpecialMusicType::Title;
	} else {
		return SpecialMusicType::Sp_NotKnown;
	}
}



void applyIniValues () {
	float val;

	val = kINIMusicSpeed.GetData ().f;
	if (isInRange (val, 0, 100)) {
		musicPlayer.setMusicSpeed ((double)val);
	}

	val = kINIFadeOut.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setCurrentFadeOutLength ((double)val * 1000);
	}

	val = kINIFadeIn.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setCurrentFadeInLength ((double)val * 1000);
	}

	val = kINITrackPause.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setMinPauseTime ((double)val * 1000);
	}

	val = kINITrackPauseExtra.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setExtraPauseTime ((double)val * 1000);
	}

	val = kINIBattleMusicStartDelay.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setMinBattleDelay ((double)val * 1000);
	}

	val = kINIBattleMusicStartDelayExtra.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setExtraBattleDelay ((double)val * 1000);
	}

	val = kINIBattleMusicEndDelay.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setMinAfterBattleDelay ((double)val * 1000);
	}

	val = kINIBattleMusicEndDelayExtra.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setExtraAfterBattleDelay ((double)val * 1000);
	}

	val = kINIBattleMusicEndDelayExtra.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setExtraAfterBattleDelay ((double)val * 1000);
	}

	val = kINIPreviousTrackRemember.GetData ().f;
	if (isInRange (val, 0, 9999)) {
		musicPlayer.setMaxRestoreTime ((double)val * 1000);
	}

	delayTitleMusicEnd = (kINIDelayTitleMusicEnd.GetData ().i > 0);

	printNewTrack = (kINIPrintTrack.GetData ().i > 0);

	_MESSAGE ("INI loaded");
}



//EMCT - World MusicType playing at time of save.
//EMCV - The Relative Volume.
static void EMC2_SaveCallback(void * reserved) {
	_MESSAGE("Event >> SaveGame");
	WaitForSingleObject (hMusicTypeMutex, INFINITE);
	MusicType save = (music.GetWorldMusic());
	ReleaseMutex (hMusicTypeMutex);

	string dataString = to_string (save);
	g_serialization->OpenRecord('EMCT', 0);
	g_serialization->WriteRecordData(dataString.c_str(), dataString.length());
}



static void EMC2_LoadCallback(void * reserved) {
	UInt32	type, version, length;

	_MESSAGE("Event >> LoadGame");
	char	buf[512];
	while (g_serialization->GetNextRecordInfo(&type, &version, &length))
	{
		//_MESSAGE("record %08X (%.4s) %08X %08X", type, &type, version, length);

		switch (type)
		{
		case 'EMCT':
			g_serialization->ReadRecordData(buf, length);
			buf[length] = 0;
			WaitForSingleObject(hMusicTypeMutex, INFINITE);
			music.world = MusicType (atoi (buf));
			music.state = MusicType::Mt_NotKnown;
			ReleaseMutex(hMusicTypeMutex);
			_MESSAGE("Event >> LoadGame >> World music: %s", buf);
			break;
		}
	}

	for (auto it = multipliersCustom.begin (); it != multipliersCustom.end (); it++) {
		Multiplier& mult = it->second;
		WaitForSingleObject (mult.hThread, INFINITE);
		if (!mult.isDestroyed && !mult.saveSession) {
			mult.isDestroyed = true;
			_MESSAGE ("Event >> LoadGame >> Multiplier destroyed: %s", it->first);
		}
		ReleaseMutex (mult.hThread);
	}

}



static void EMC2_NewGameCallback(void * reserved) {
	_MESSAGE("Event >> NewGame");
	WaitForSingleObject (hMusicTypeMutex, INFINITE);
	music.world = MusicType::Dungeon;			//Should be fine for your vanilla Oblivion.
	music.state = MusicType::Mt_NotKnown;
	ReleaseMutex(hMusicTypeMutex);

	for (auto it = multipliersCustom.begin (); it != multipliersCustom.end (); it++) {
		Multiplier *mult = &it->second;
		WaitForSingleObject (mult->hThread, INFINITE);
		if (!mult->isDestroyed && !mult->saveSession) {
			mult->isDestroyed = true;
			_MESSAGE ("Event >> NewGame >> Multiplier destroyed: %s", it->first);
		}
		ReleaseMutex (it->second.hThread);
	}
}



extern "C" {

	bool OBSEPlugin_Query(const OBSEInterface * obse, PluginInfo * info) {
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

			g_arrayIntfc = (OBSEArrayVarInterface *)obse->QueryInterface(kInterface_ArrayVar);
			if (!g_arrayIntfc) {
				_ERROR("EMC2 >> Array interface not found");
				return false;
			}

			g_stringIntfc = (OBSEStringVarInterface*)obse->QueryInterface(kInterface_StringVar);
			if (!g_serialization) {
				_ERROR("EMC2 >> String interface not found");
				return false;
			} else if (g_stringIntfc->version < OBSEStringVarInterface::kVersion) {
				_ERROR ("EMC2 >> Incorrect string interface version found (got %08X need %08X)", g_stringIntfc->version, OBSEStringVarInterface::kVersion);
				return false;
			}
		}
		return true;
	}



	bool OBSEPlugin_Load(const OBSEInterface * obse) {
		_MESSAGE("EMC2 >> Load");

		RegisterStringVarInterface (g_stringIntfc);
		g_pluginHandle = obse->GetPluginHandle();

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


		void *obSetMusicVolume = (void *)0x006AA1A0;
		void *obQueueMusicTrack = (void *)0x006AB160;
		void *obPlayQueuedMusicTrack = (void *)0x006AB420;
		void *obGetIsInCombat = (void *)0x006605A0;

		// register commands
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
		obse->RegisterTypedCommand	(&kGetTrackNameCommand, kRetnType_String);		//24CC
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
		obse->RegisterCommand		(&kGetBattleDelayCommand);						//2844
		obse->RegisterCommand		(&kSetBattleDelayCommand);						//2845
		obse->RegisterCommand		(&kGetAfterBattleDelayCommand);					//2846
		obse->RegisterCommand		(&kSetAfterBattleDelayCommand);					//2847
		obse->RegisterCommand		(&kGetMaxRestoreTimeCommand);					//2848
		obse->RegisterCommand		(&kSetMaxRestoreTimeCommand);					//2849
		obse->RegisterCommand		(&kPlayTrackCommand);							//244A
		obse->RegisterCommand		(&kMusicStopCommand);							//284B
		obse->RegisterCommand		(&kMusicPauseCommand);							//284C
		obse->RegisterCommand		(&kMusicResumeCommand);							//284D
		obse->RegisterCommand		(&kMusicRestartCommand);						//284E
		obse->RegisterCommand		(&kMusicNextTrackCommand);						//284F

		//Initialize the Mutex so Sentry thread knows when its safe to
		//read or alter global variables.
		hMusicTypeMutex = CreateMutex(NULL, FALSE, NULL);  // Starts cleared, no owner
		hThePlayerMutex = CreateMutex(NULL, FALSE, NULL);
		hPlaylistMutex = CreateMutex(NULL, FALSE, NULL);
		hSentryRequestMutex = CreateMutex(NULL, FALSE, NULL);

		
		// set up serialization callbacks when running in the runtime
		if (!obse->isEditor) {
			// NOTE: SERIALIZATION DOES NOT WORK USING THE DEFAULT OPCODE BASE IN RELEASE BUILDS OF OBSE
			// it works in debug builds
			g_serialization->SetSaveCallback (g_pluginHandle, EMC2_SaveCallback);
			g_serialization->SetLoadCallback (g_pluginHandle, EMC2_LoadCallback);
			g_serialization->SetNewGameCallback (g_pluginHandle, EMC2_NewGameCallback);

			//Override game's music system.
			//Patch the StreamMusic command.
			WriteRelCall (0x005096CA, (UInt32)&StreamMusicType);	//"Random"
			WriteRelCall (0x005096F2, (UInt32)&StreamMusicType);	//"Explore"
			WriteRelCall (0x0050971B, (UInt32)&StreamMusicType);	//"Public"
			WriteRelCall (0x00509741, (UInt32)&StreamMusicType);	//"Dungeon"
			WriteRelCall (0x00509767, (UInt32)&StreamMusicType);	//"Battle"
			WriteRelCall (0x00509779, (UInt32)&StreamMusicFile);	//"Custom"
			WriteRelCall (0x00509780, (UInt32)&StreamMusicFileDoNothing);	//"Pathname"

			//Replace the vanilla behaviour
			WriteRelJump (0x006AB160, (UInt32)&QueueMusicTrack);
			WriteRelJump (0x006AB420, (UInt32)&PlayQueuedMusic);

			//These will tell us when different music states are detected.
			WriteRelJump (0x005B5B68, (UInt32)&DetectTitleMusic);
			WriteRelJump (0x005AD098, (UInt32)&DetectSuccessMusic);
			WriteRelJump (0x0066068B, (UInt32)&DetectBattleMusic);
			WriteRelJump (0x00660697, (UInt32)&DetectNotBattleMusic_1);
			WriteRelJump (0x00660691, (UInt32)&DetectNotBattleMusic_2);
			WriteRelJump (0x006AC026, 0x006AC056);	// Stop Oblivion from writing current volume levels to INI. The audio menu does it anyway.

			EMC2INISettings iniSettings;
			iniSettings.Initialize (INI_PATH, NULL);

			numPlaylists = kINIMaxNumPlaylists.GetData ().i;		//From ini
			numMultipliers = kINIMaxNumMultipliers.GetData ().i;	//From ini
			playlists.reserve (numPlaylists);
			multipliersVanilla.reserve (10);	//FIXED (5 mults from sliders + 5 mults from ini)
			multipliersCustom.reserve (numMultipliers);

			_MESSAGE ("Create default playlists");
			//MusicPathPointer *defaultMusicPaths = (MusicPathPointer *)0x00A76DC4;
			//string explorePath = defaultMusicPaths->pathExploreMusic;
			plExplore = addPlaylist (obExplore, obExplorePath"*.mp3|"obExplorePath"*.wav|"obExplorePath"*.wma", true, true);
			plPublic = addPlaylist (obPublic, obPublicPath"*.mp3|"obPublicPath"*.wav|"obPublicPath"*.wma", true, true);
			plDungeon = addPlaylist (obDungeon, obDungeonPath"*.mp3|"obDungeonPath"*.wav|"obDungeonPath"*.wma", true, true);
			plCustom = plExplore;
			plBattle = addPlaylist (obBattle, obBattlePath"*.mp3|"obBattlePath"*.wav|"obBattlePath"*.wma", true, true);
			plDeath = addPlaylist (obDeath, obDeathPath".mp3|"obDeathPath".wav|"obDeathPath".wma", true, true);
			plSuccess = addPlaylist (obSuccess, obSuccessPath".mp3|"obSuccessPath".wav|"obSuccessPath".wma", true, true);
			plTitle = addPlaylist (obTitle, obTitlePath".mp3|"obTitlePath".wav|"obTitlePath".wma", true, true);
	
			//Set the playlists to default.
			/*curPlaylistNames[0] = obExplore;
			curPlaylistNames[1] = obPublic;
			curPlaylistNames[2] = obDungeon;
			curPlaylistNames[3] = obExplore;
			curPlaylistNames[4] = obBattle;
			curPlaylistNames[5] = obDeath;
			curPlaylistNames[6] = obSuccess;
			curPlaylistNames[7] = obTitle;*/
			
			_beginthread(MainThread, 0, NULL);
		}
		
		return true;
	}

};