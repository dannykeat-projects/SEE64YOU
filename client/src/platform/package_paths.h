#ifndef SEE64YOU_PLATFORM_PACKAGE_PATHS_H
#define SEE64YOU_PLATFORM_PACKAGE_PATHS_H

/*
 * User-visible application installation directory.
 * Runtime package files are absolute so module transitions never depend on the
 * current Ultimate-DOS working directory left by documents/settings activity.
 *
 * Persistent SEE64YOU.CFG and session journals deliberately remain at their
 * hardware-proven locations until their separate transactional migration.
 */
#define S64_SD_ROOT_DIRECTORY "/sd/"
#define S64_PACKAGE_DIRECTORY "/sd/SEE64YOU/"

#endif
