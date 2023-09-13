#pragma once

#include <stdint.h>

uint32_t GetCurrentSeed();
void RestartSeed();
void IncrementSeed(int diff);
void SetSeed(uint32_t seed);
uint32_t GetRandomNumber();
int GetRandomInteger(uint32_t max_value);
int GetRandomInteger(uint32_t min_value, uint32_t max_value);
float GetRandomFloat(float max_value);
float GetRandomFloat(float min_value, float max_value);
