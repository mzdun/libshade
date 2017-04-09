#pragma once

#define VERSION_STRINGIFY_HELPER(n) #n
#define VERSION_STRINGIFY(n) VERSION_STRINGIFY_HELPER(n)

#define SHADE_VERSION_MAJOR @PROJECT_VERSION_MAJOR@
#define SHADE_VERSION_MINOR @PROJECT_VERSION_MINOR@
#define SHADE_VERSION_PATCH @PROJECT_VERSION_PATCH@
#define SHADE_VERSION_BUILD @PROJECT_VERSION_BUILD@
#define SHADE_VERSION_STABILITY "@PROJECT_VERSION_STABILITY@" // "-alpha", "-beta", "-rc", and ""

#define SHADE_VERSION_STRING \
     VERSION_STRINGIFY(SHADE_VERSION_MAJOR) "." \
     VERSION_STRINGIFY(SHADE_VERSION_MINOR) "." \
     VERSION_STRINGIFY(SHADE_VERSION_PATCH)

#define SHADE_VERSION_FULL \
     VERSION_STRINGIFY(SHADE_VERSION_MAJOR) "." \
     VERSION_STRINGIFY(SHADE_VERSION_MINOR) "." \
     VERSION_STRINGIFY(SHADE_VERSION_PATCH) \
	 SHADE_VERSION_STABILITY "+" \
	 VERSION_STRINGIFY(SHADE_VERSION_BUILD)

#define SHADE_VERSION_STRING_SHORT \
     VERSION_STRINGIFY(SHADE_PROGRAM_VERSION_MAJOR) "." \
     VERSION_STRINGIFY(SHADE_PROGRAM_VERSION_MINOR)
