#include "OBSEInterfaces.h"


PluginHandle				g_pluginHandle = kPluginHandle_Invalid;
OBSESerializationInterface	*g_serialization = nullptr;
OBSEArrayVarInterface		*g_arrayIntfc = nullptr;
OBSEStringVarInterface		*g_stringIntfc = nullptr;