#pragma once

namespace moducom {

struct SemVer
{
    const unsigned short major;
    const unsigned short minor;
    const unsigned short patch;

    const char* prerelease;

    SemVer& operator=(const SemVer& copyFrom) = default;
};

}