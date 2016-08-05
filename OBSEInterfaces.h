#pragma once

#include "PluginAPI.h"



extern PluginHandle					g_pluginHandle;
extern OBSESerializationInterface	*g_serialization;
extern OBSEArrayVarInterface		*g_arrayIntfc;
extern OBSEStringVarInterface		*g_stringIntfc;

typedef OBSEArrayVarInterface::Array OBSEArray;