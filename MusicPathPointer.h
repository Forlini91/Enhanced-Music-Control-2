#pragma once

struct MusicPathPointer {
	char wildcardMusicSearchPattern[8];  //'%s*.mp3'
	char pathBattleMusic[20];
	char pathDungeonMusic[20];
	char pathPublicMusic[20];
	char pathExploreMusic[20];
};