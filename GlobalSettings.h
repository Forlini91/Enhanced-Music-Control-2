#pragma once

//This setting define the path of the ini
#define INI_PATH "Data\\OBSE\\Plugins\\EnhancedMusicControl 2.ini"

//This is the very lowest volume that can be used in DShow.
//Setting the volume to this effectively mutes the playing track.
#define ABSOLUTE_MINIMUM_VOLUME -10000


//But below -4000, you can't really hear the audio anyways.
//So, this is the effective minimum, and is what is used
//during fades.
#define EFFECTIVE_MINIMUM_VOLUME -4000


//There is only one maximum.  It would be funny if they made it '11'.
#define MAXIMUM_VOLUME 0




#define obExplore "obExplore"
#define obPublic "obPublic"
#define obDungeon "obDungeon"
#define obCustom "obExplore"
#define obBattle "obBattle"
#define obDeath "obDeath"
#define obSuccess "obSuccess"
#define obTitle "obTitle"

#define obExplorePath "Data\\Music\\Explore\\"
#define obPublicPath "Data\\Music\\Public\\"
#define obDungeonPath "Data\\Music\\Dungeon\\"
#define obBattlePath "Data\\Music\\Battle\\"
#define obDeathPath "Data\\Music\\Special\\death"
#define obSuccessPath "Data\\Music\\Special\\success"
#define obTitlePath "Data\\Music\\Special\\tes4title"

#define obMaster "obMaster"
#define obMasterIni "obMasterIni"
#define obMusic "obMusic"
#define obMusicIni "obMusicIni"
#define obVoice "obVoice"
#define obVoiceIni "obVoiceIni"
#define obEffects "obEffects"
#define obEffectsIni "obEffectsIni"
#define obFoot "obFoot"
#define obFootIni "obFootIni"

#define CONSOLE IsConsoleOpen () && IsConsoleMode ()

#define BUILD_IN_PLACE(key, ...) piecewise_construct, forward_as_tuple(key), forward_as_tuple(__VA_ARGS__) 