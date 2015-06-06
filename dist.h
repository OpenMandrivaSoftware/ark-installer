#ifndef DIST
#define DIST "Ark Linux"
#endif
#ifndef VER
#define VER "H2O (1.0)"
#endif
#if !defined(XP) && !defined(NOXP)
#define XP "snapshot"
#endif
#ifdef XP
#define VERSION " "VER" "XP
#else
#define VERSION VER
#endif
#define DISTSIZE 2048 // Size of unpacked distribution in Megabytes
#ifndef EDITION
#define EDITION "Desktop"
#endif
