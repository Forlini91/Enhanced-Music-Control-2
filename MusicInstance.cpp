#include "MusicInstance.h"

#include "Globals.h"
#include "IniData.h"
#include "MusicTimes.h"
#include "TimeManagement.h"


using namespace std;

map<MusicType, MusicInstance> musicInstances;



void MusicInstance::setData (const string& curPlaylistName, const string& curTrackPath, LONGLONG curPosition, int maxRestoreTime) {
	lastPlayedTime = now + milliseconds (maxRestoreTime);
	playlistName = curPlaylistName;
	trackPath = curTrackPath;
	position = curPosition;
}




void initMusicInstances () {
	musicInstances.emplace (constructInPlace (MusicType::mtExplore));
	musicInstances.emplace (constructInPlace (MusicType::mtPublic));
	musicInstances.emplace (constructInPlace (MusicType::mtDungeon));
	musicInstances.emplace (constructInPlace (MusicType::mtCustom));
	musicInstances.emplace (constructInPlace (MusicType::mtBattle));
}


bool saveMusicInstance (MusicType musicType, const string& playlistName, const string& trackPath, LONGLONG position, LONGLONG duration) {
	//Assume the caller will lock the hMusicPlayerMutex
	if (musicType <= 4 && position <= duration*previousTrackRememberRatio) {
		auto result = musicInstances.find (musicType);
		if (result != musicInstances.end ()) {
			int maxRestoreTime = musicType != MusicType::mtBattle ? musicTimes.getMusicRememberTime () : musicTimes.getBattleMusicRememberTime ();
			result->second.setData (playlistName, trackPath, position, maxRestoreTime);
			return true;
		}
	}
	return false;
	//Assume the caller will unlock the hMusicPlayerMutex
}


bool getMusicInstance (MusicType musicType, string *varPlaylistName, string *varTrackPath, LONGLONG *varPosition) {
	if (musicType <= 4) {
		auto result = musicInstances.find (musicType);
		if (result != musicInstances.end ()) {
			MusicInstance& musicInstance = result->second;
			if (musicInstance.position >= 0) {
				PlaylistsMap::const_iterator cit = playlists.find (musicInstance.playlistName);
				if (cit != playlists.end () && musicInstance.lastPlayedTime >= now && cit->second.hasTrack (musicInstance.trackPath)) {
					*varPlaylistName = musicInstance.playlistName;
					*varTrackPath = musicInstance.trackPath;
					*varPosition = musicInstance.position;
					return true;
				}
				musicInstance.position = -1;
			}
		}
	}
	return false;
}