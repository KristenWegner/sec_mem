uint64_t splitmix64_x; // The state.

static inline void splitmix64_seed(uint64_t seed) { splitmix64_x = seed; }

static inline uint64_t splitmix64(void) {
  uint64_t z = (splitmix64_x += UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}

static inline uint32_t splitmix64_cast32(void) { return (uint32_t)splitmix64(); }

static inline uint64_t splitmix64_stateless(uint64_t index) 
{
  uint64_t z = (index * UINT64_C(0x9E3779B97F4A7C15));
  z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
  z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
  return z ^ (z >> 31);
}
