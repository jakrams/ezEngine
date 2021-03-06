#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Plugin.h>

// Configure the DLL Import/Export Define
#if EZ_ENABLED(EZ_COMPILE_ENGINE_AS_DLL)
#  ifdef BUILDSYSTEM_BUILDING_EDITORPLUGINKRAUT_LIB
#    define EZ_EDITORPLUGINKRAUT_DLL __declspec(dllexport)
#  else
#    define EZ_EDITORPLUGINKRAUT_DLL __declspec(dllimport)
#  endif
#else
#  define EZ_EDITORPLUGINKRAUT_DLL
#endif
