#ifndef MACHO_LOADER_H_
#define MACHO_LOADER_H_

#include <stdint.h>
#include "emulator.h"

Emulator* load_macho64(char* filepath);

#endif
