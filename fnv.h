#pragma once
#include "common.h"

u64 hash64(byte *p, int len, u64 h=0);
u64 hash64(const char *p);
