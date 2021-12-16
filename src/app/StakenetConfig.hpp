#ifndef STAKENET_CONFIG_HPP
#define STAKENET_CONFIG_HPP

#include <QString>

extern const char STAKENET_GIT_SHA1_BUILD[];

extern const unsigned int STAKENET_MAJOR_VERSION;
extern const unsigned int STAKENET_MINOR_VERSION;
extern const unsigned int STAKENET_PATCH_VERSION;
extern const unsigned int STAKENET_TWEAK_VERSION;

extern const bool STAKENET_DEFAULT_STAGING;

QString StakenetVersion();
unsigned int StakenetNumericVersion();

#endif
