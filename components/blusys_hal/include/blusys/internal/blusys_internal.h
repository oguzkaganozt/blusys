#ifndef BLUSYS_INTERNAL_H
#define BLUSYS_INTERNAL_H

#define BLUSYS_VERSION_PACK(major, minor, patch) \
    ((((unsigned int) (major) & 0xffu) << 16) | \
     (((unsigned int) (minor) & 0xffu) << 8) | \
     ((unsigned int) (patch) & 0xffu))

#endif
