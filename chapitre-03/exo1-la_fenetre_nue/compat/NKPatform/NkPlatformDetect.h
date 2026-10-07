#pragma once
// NKWindow's Core/NkEntry.h includes "NKPatform/NkPlatformDetect.h", a path
// that does not exist in the installed engine (the real file is the sibling
// Core/NkPlatformDetect.h). This shim forwards to the real file without
// modifying the installed, shared copy of the engine.
#include "NkPlatformDetect.h"
