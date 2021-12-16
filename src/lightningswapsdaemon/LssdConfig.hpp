#ifndef LSSD_CONFIG_HPP
#define LSSD_CONFIG_HPP

#include <QString>

extern const char LSSD_GIT_SHA1_BUILD[];

extern const unsigned int LSSD_MAJOR_VERSION;
extern const unsigned int LSSD_MINOR_VERSION;
extern const unsigned int LSSD_PATCH_VERSION;
extern const unsigned int LSSD_TWEAK_VERSION;

QString LssdVersion();
unsigned int LssdNumericVersion();

#endif
