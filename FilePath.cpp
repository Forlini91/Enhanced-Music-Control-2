#include "FilePath.h"

#include "shlwapi.h"
#include <algorithm>

#include "Globals.h"



const string currentOblivionPath = GetOblivionDirectory ();
const string supportedExtensions[] = { ".mp3", ".wav", ".wma" };


bool exists (const string &path) {
	return PathFileExists (path.c_str ()) != 0;
}


bool isDirectory (const string &path) {
	return PathIsDirectory (path.c_str ()) != 0;
}


bool isExtensionSupported (const string &path) {
	return endsWithAny (path, supportedExtensions);
}


string getFileName (const string &path) {
	size_t pos = path.find_last_of ('\\');
	if (pos != string::npos) {
		return path.substr (pos);
	} else {
		return path;
	}
}


string getFolderPath (const string &path) {
	size_t pos = path.find_last_of ('\\');
	if (pos != string::npos) {
		return path.substr (0, pos + 1);
	} else {
		return path;
	}
}


string cleanPath (const string &path, bool relativize) {
	string pathC = trim (path);
	replace (pathC.begin (), pathC.end (), '/', '\\');
	if (relativize && PathIsRelative (pathC.c_str ()) != 0) {
		pathC = currentOblivionPath + pathC;
	}
	return pathC;
}


string trim (const string &str) {
	size_t first = str.find_first_not_of (" \t\n\r");
	if (first == string::npos)
		return "";
	size_t last = str.find_last_not_of (" \t\n\r");
	return str.substr (first, (last - first + 1));
}


bool endsWith (const string &str, const string &ending) {
	int strLen = str.length ();
	int endLen = ending.length ();
	if (strLen >= endLen) {
		return str.compare (strLen - endLen, endLen, ending) == 0;
	} else {
		return false;
	}
}



bool endsWithAny (const string &str, const string endings[]) {
	for (int i = 0, n = arraySize (endings, string); i < n; i++) {
		if (endsWith (str, endings[i])) {
			return true;
		}
	}
	return false;
}



bool endsNotWith (const string &str, const string &ending) {
	int strLen = str.length ();
	int endLen = ending.length ();
	if (strLen >= endLen) {
		return str.compare (strLen - endLen, endLen, ending) != 0;
	} else {
		return true;
	}
}



bool endsNotWithAll (const string &str, const string endings[]) {
	for (int i = 0, n = arraySize (endings, string); i < n; i++) {
		if (!endsNotWith (str, endings[i])) {
			return false;
		}
	}
	return true;
}