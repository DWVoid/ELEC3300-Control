#pragma once

#include "main.h"

#ifdef __cplusplus
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdbool>
#define C_BEGIN extern "C" {
#define C_END }
#else
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <memory.h>
#include <stdbool.h>
#define C_BEGIN
#define C_END
#endif
