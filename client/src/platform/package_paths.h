#ifndef SEE64YOU_PLATFORM_PACKAGE_PATHS_H
#define SEE64YOU_PLATFORM_PACKAGE_PATHS_H

/*
 * User-visible application installation directory.
 * Runtime package files are absolute so module transitions never depend on the
 * current Ultimate-DOS working directory left by documents/settings activity.
 *
 * Keep all programs, prerequisite checks and recovery on the same device.
 * The build may select a USB mount; the root must end in a slash.
 */
#ifndef S64_STORAGE_ROOT_DIRECTORY
#define S64_STORAGE_ROOT_DIRECTORY "/sd/"
#endif
#define S64_SD_ROOT_DIRECTORY S64_STORAGE_ROOT_DIRECTORY
#define S64_PACKAGE_DIRECTORY S64_STORAGE_ROOT_DIRECTORY "SEE64YOU/"
/* Verified runtime overlays and manifests live under a separate install-only folder. */
#define S64_PACKAGE_ASSETS_DIRECTORY S64_PACKAGE_DIRECTORY "SYSTEM/"

#endif
