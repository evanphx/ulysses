
static inline u64 rdtsc() {
  u32int hi, lo;
  // Use this instead of =A so it works properly on x86_64
  __asm__ volatile("lfence; rdtsc; lfence" : "=a"(lo), "=d"(hi));
  return (((u64)hi) << 32) | lo;
}

void init_clock();
void update_clock();

