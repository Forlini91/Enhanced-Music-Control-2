#pragma once

#include <string>
#include "GameAPI.h"



using namespace std;



extern const string currentOblivionPath;



bool exists (const string& path);

bool isDirectory (const string& path);

string getFileName (const string& path);

string getFolderPath (const string& path);

string cleanPath (const string& path, bool relativize);

string trim (const string& str);

bool endsWith (const string& str, const string& ending);
