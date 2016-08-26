#pragma once


#define defaultPauseTime 0
#define defaultStartBattleDelay 1000
#define defaultStopBattleDelay 0
#define defaultMusicRememberTime 20000
#define defaultBattleMusicRememberTime 60000
#define defaultFadeIn 1000
#define defaultFadeOut 1000

struct MusicTimes {


private:

	int minPauseTime = defaultPauseTime;
	int extraPauseTime = 0;
	int pauseTime = defaultPauseTime;

	int minStartBattleDelay = defaultStartBattleDelay;
	int extraStartBattleDelay = 0;
	int startBattleDelay = defaultStartBattleDelay;

	int minStopBattleDelay = defaultStopBattleDelay;
	int extraStopBattleDelay = 0;
	int stopBattleDelay = defaultStopBattleDelay;

	//Restore period is used to restore the previous song if the music type changes once, then suddenly changes back again.
	//Say the player enters an Inn, then exits within a short period of time.
	//This will make it so the previous music will play where it left off instead of restarting a new track.
	int musicRememberTime = defaultMusicRememberTime;
	int battleMusicRememberTime = defaultBattleMusicRememberTime;

	int fadeIn = defaultFadeIn;
	int fadeOut = defaultFadeOut;



public:
	//Gets the min pause between tracks
	int getMinPauseTime () const;

	//Sets the min pause between tracks
	bool setMinPauseTime (int time);

	//Gets the extra pause between tracks
	int getExtraPauseTime () const;

	//Sets the extra pause between tracks
	bool setExtraPauseTime (int time);

	//Gets the calculated random pause (in the range [min, min+extra]) between tracks
	int getPauseTime () const;

	//Calculate a new random pause (in the range [min, min+extra]) between tracks
	void recalculatePauseTime ();


	//Gets the extra delay before starting the battle music
	int getMinStartBattleDelay () const;

	//Sets the min delay before starting the battle music
	bool setMinStartBattleDelay (int delay);

	//Gets the extra delay before starting the battle music
	int getExtraStartBattleDelay () const;

	//Sets the extra delay before starting the battle music
	bool setExtraStartBattleDelay (int delay);

	//Gets the calculated random delay (in the range [min, min+extra]) before starting the battle music
	int getStartBattleDelay () const;

	//Calculate a new random delay (in the range [min, min+extra]) before starting the battle music
	void recalculateStartBattleDelay ();


	//Gets the min delay before stopping the battle music
	int getMinStopBattleDelay () const;

	//Sets the min delay before stopping the battle music
	bool setMinStopBattleDelay (int delay);

	//Gets the extra delay before stopping the battle music
	int getExtraStopBattleDelay () const;

	//Sets the extra delay before stopping the battle music
	bool setExtraStopBattleDelay (int delay);

	//Gets the calculated random delay (in the range [min, min+extra]) before stopping the battle music
	int getStopBattleDelay () const;

	//Calculate a new random delay (in the range [min, min+extra]) before stopping the battle music
	void recalculateStopBattleDelay ();


	//Gets the current fade in lenght
	int getFadeIn () const;

	//Sets the current fade in lenght
	bool setFadeIn (int time);

	//Gets the current fade out lenght
	int getFadeOut () const;

	//Sets the current fade out lenght
	bool setFadeOut (int time);


	//Gets the max restore time (how much time the player will remember the previous track to restore it)
	int getMusicRememberTime () const;

	//Sets the max restore time (how much time the player will remember the previous track to restore it)
	bool setMusicRememberTime (int time);

	//Gets the max restore time in battle (how much time the player will remember the previous track to restore it, when in battle)
	int getBattleMusicRememberTime () const;

	//Sets the max restore time in battle (how much time the player will remember the previous track to restore it, when in battle)
	bool setBattleMusicRememberTime (int time);

};


typedef int (MusicTimes::*MusicTimesGetter)() const;

extern MusicTimes musicTimes;
extern HANDLE hMusicTimesMutex;

extern MusicTimesGetter musicTimesGetter[];