#include "EMC2INISettings.h"

#include "GlobalSettings.h"



EMC2INISettings iniSettings;


void EMC2INISettings::Initialize (const char* INIPath, void* Parameter) {
	this->INIFilePath = INIPath;
	_MESSAGE ("INI Path: %s", INIPath);

	std::fstream INIStream (INIPath, std::fstream::in);
	bool CreateINI = false;
	if (INIStream.fail ()) {
		_MESSAGE ("INI File not found; Creating one...");
		CreateINI = true;
	}
	INIStream.close ();
	INIStream.clear ();

	RegisterSetting (&kINIMusicSpeed);
	RegisterSetting (&kINIFadeOut);
	RegisterSetting (&kINIFadeIn);
	RegisterSetting (&kINITrackPause);
	RegisterSetting (&kINITrackPauseExtra);
	RegisterSetting (&kINIBattleMusicStartDelay);
	RegisterSetting (&kINIBattleMusicStartDelayExtra);
	RegisterSetting (&kINIBattleMusicEndDelay);
	RegisterSetting (&kINIBattleMusicEndDelayExtra);
	RegisterSetting (&kINIPreviousTrackRemember);
	RegisterSetting (&kINIDelayTitleMusicEnd);
	RegisterSetting (&kINIMaxNumPlaylists);
	RegisterSetting (&kINIMaxNumMultipliers);
	RegisterSetting (&kINIPrintTrack);

	if (CreateINI)
		Save ();

	PopulateFromSection ("GenericSection");
}



void EMC2INISettings::applySettings (bool &delayTitleMusicEnd, bool &printNewTrack, int &numPlaylists, int &numMultipliers) {
	_MESSAGE ("Initialization >> EMC2 ini data");
	float val;

	val = kINIMusicSpeed.GetData ().f;
	if (isBetween (val, 0, 100)) {
		musicPlayer.setMusicSpeed ((double)val);
	}

	val = kINIFadeOut.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setCurrentFadeOutLength (val * 1000);
	}

	val = kINIFadeIn.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setCurrentFadeInLength (val * 1000);
	}

	val = kINITrackPause.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setMinPauseTime (val * 1000);
	}

	val = kINITrackPauseExtra.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setExtraPauseTime (val * 1000);
	}

	val = kINIBattleMusicStartDelay.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setMinBattleDelay (val * 1000);
	}

	val = kINIBattleMusicStartDelayExtra.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setExtraBattleDelay ((double)val * 1000);
	}

	val = kINIBattleMusicEndDelay.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setMinAfterBattleDelay ((double)val * 1000);
	}

	val = kINIBattleMusicEndDelayExtra.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setExtraAfterBattleDelay ((double)val * 1000);
	}

	val = kINIBattleMusicEndDelayExtra.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setExtraAfterBattleDelay ((double)val * 1000);
	}

	val = kINIPreviousTrackRemember.GetData ().f;
	if (isBetween (val, 0, 9999)) {
		musicPlayer.setMaxRestoreTime ((double)val * 1000);
	}

	delayTitleMusicEnd = (kINIDelayTitleMusicEnd.GetData ().i > 0);

	printNewTrack = (kINIPrintTrack.GetData ().i > 0);

	numPlaylists = kINIMaxNumPlaylists.GetData ().i;

	numMultipliers = kINIMaxNumMultipliers.GetData ().i;
}