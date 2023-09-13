#include "random.h"

#include <stdint.h>

#include "pw_random/random.h"
#include "pw_random/xor_shift.h"

#define RANDOM_TYPE_PRNG 0
#define RANDOM_TYPE_PW_PRNG 1

namespace {

constexpr uint64_t kRandomSeed = 314159265358979;
static pw::random::XorShiftStarRng64 rng(kRandomSeed);

uint32_t random_seed_offset = 0;
uint8_t current_random_source = RANDOM_TYPE_PW_PRNG;
uint32_t current_random_seed = 0x64063701;
uint32_t prng_lfsr = 0;
const uint16_t prng_tap = 0x74b8;

}  // namespace

uint32_t GetCurrentSeed() { return current_random_seed; }

void RestartSeed() {
  prng_lfsr = current_random_seed;
  rng = pw::random::XorShiftStarRng64(kRandomSeed + random_seed_offset);
}

void IncrementSeed(int diff) {
  current_random_seed += diff;
  random_seed_offset += diff;
  RestartSeed();
}

void SetSeed(uint32_t seed) {
  current_random_seed = seed;
  RestartSeed();
}

uint32_t GetRandomNumber() {
  if (current_random_source == RANDOM_TYPE_PRNG) {
    uint8_t lsb = prng_lfsr & 1;
    prng_lfsr >>= 1;

    if (lsb) {
      prng_lfsr ^= prng_tap;
    }
    return prng_lfsr;
  } else if (current_random_source == RANDOM_TYPE_PW_PRNG) {
    int random_value = 0;
    rng.GetInt(random_value);
    return (uint32_t)random_value;
  }
  return 0;
}

int GetRandomInteger(uint32_t max_value) {
  return (int)(GetRandomNumber() % max_value);
}

int GetRandomInteger(uint32_t min_value, uint32_t max_value) {
  int diff = max_value - min_value;
  if (diff < 0)
    diff *= -1;

  int r = GetRandomNumber() % (uint32_t)(diff);
  r += min_value;

  return r;
}

float GetRandomFloat(float max_value) {
  uint32_t r = GetRandomNumber() % (uint32_t)(max_value);
  uint32_t d = GetRandomNumber() % 1000000;
  float decimal_part = (float)d / 1000000.0f;
  float x = (float)r + decimal_part;
  return x;
}

float GetRandomFloat(float min_value, float max_value) {
  float diff = max_value - min_value;
  float r = GetRandomFloat(diff);
  r += min_value;
  return r;
}
