#include "render/local.h"

// This file's entire purpose is to house the render module globals.

uint16_t wallminx;
uint16_t wallmaxx;
uint16_t sectorxmin;
uint16_t sectorxmax;
const sector_t *rendersector;
vector_t renderpos;
fixed_t rendereyeheight;

// TODO
uint8_t detaillevel = 1;
