#include <base.h>
#include <tool.h>

#include "liftlibcontrol.h"
#include "liftlibio.h"

#define WC_BASE 0xa2000000u
#define ORDERED_BASE 0xa3000000u
#define DEVICE_DOORBELL 0xa0001000u
#define LIFT_STEPS 1001u
#define WARMUP_STEPS 32u
#define LIFT_CHECKSUM 4005888

void lift_controller(void);
void lift_init(void);

enum run_mode {
  RUN_RAM_ONLY,
  RUN_ORDERED,
  RUN_WRITE_COMBINE,
};

struct run_result {
  uint32_t cycles;
  int checksum;
};

static inline uint32_t read_cycles(void)
{
  uint32_t value;
  __asm__ volatile ("csrr %0, mcycle" : "=r"(value));
  return value;
}

static void reset_controller(void)
{
  unsigned int i;

  for (i = 0; i < 10; ++i)
    lift_ctrl_io_in[i] = 0;
  for (i = 0; i < 4; ++i) {
    lift_ctrl_io_out[i] = 0;
    lift_ctrl_io_analog[i] = 0;
  }
  for (i = 0; i < 16; ++i) {
    lift_ctrl_io_led[i] = 0;
    lift_levelPos[i] = 0;
  }

  lift_ctrl_dly1 = 0;
  lift_ctrl_dly2 = 0;
  lift_simio_in = 0;
  lift_simio_out = 0;
  lift_simio_led = 0;
  lift_simio_adc1 = 0;
  lift_simio_adc2 = 0;
  lift_simio_adc3 = 0;

  lift_cntValid = 0;
  lift_cnt = 0;
  lift_level = 0;
  lift_loadLevel = 0;
  lift_loadPending = 0;
  lift_loadSensor = 0;
  lift_cmd = 0;
  lift_timMotor = 0;
  lift_timImp = 0;
  lift_directionUp = 0;
  lift_lastImp = 0;
  lift_dbgCnt = 0;
  lift_endCnt = 0;
  lift_checksum = 0;

  lift_init();
}

static inline void set_zero_stimulus(void)
{
  lift_simio_in = 0;
  lift_simio_adc1 = 0;
  lift_simio_adc2 = 0;
  lift_simio_adc3 = 0;
}

static inline void submit_actuator_state(uintptr_t base, uint32_t sequence)
{
  volatile uint32_t *descriptor = (volatile uint32_t *)base;
  volatile uint32_t *doorbell =
    (volatile uint32_t *)(uintptr_t)DEVICE_DOORBELL;

  descriptor[0] = sequence;
  descriptor[1] = (uint32_t)lift_simio_out;
  descriptor[2] = (uint32_t)lift_simio_led;
  descriptor[3] = (uint32_t)lift_level;
  descriptor[4] = (uint32_t)lift_cnt;
  descriptor[5] = (uint32_t)lift_cmd;
  descriptor[6] = (uint32_t)lift_loadLevel;
  descriptor[7] = (uint32_t)lift_checksum;
  __asm__ volatile ("fence w,w" ::: "memory");
  *doorbell = sequence;
}

static struct run_result run_controller(enum run_mode mode,
                                        uint32_t steps)
{
  uintptr_t descriptor_base = 0;
  struct run_result result;
  uint32_t start;
  uint32_t i;

  if (mode == RUN_ORDERED)
    descriptor_base = ORDERED_BASE;
  else if (mode == RUN_WRITE_COMBINE)
    descriptor_base = WC_BASE;

  reset_controller();
  start = read_cycles();
  for (i = 0; i < steps; ++i) {
    set_zero_stimulus();
    lift_controller();
    if (descriptor_base != 0)
      submit_actuator_state(descriptor_base, i);
  }
  if (descriptor_base != 0)
    __asm__ volatile ("fence w,w" ::: "memory");

  result.cycles = read_cycles() - start;
  result.checksum = lift_checksum;
  return result;
}

static void print_ratio(const char *label, uint32_t numerator,
                        uint32_t denominator)
{
  if (denominator == 0) {
    printf("%s=n/a\n", label);
    return;
  }
  printf("%s=%u.%02ux\n", label, numerator / denominator,
         (numerator % denominator) * 100u / denominator);
}

int main(int argc, char **argv)
{
  struct run_result ram;
  struct run_result ordered;
  struct run_result combined;
  uint32_t ordered_submit;
  uint32_t combined_submit;
  int pass;

  (void)argc;
  (void)argv;

  run_controller(RUN_RAM_ONLY, WARMUP_STEPS);
  run_controller(RUN_ORDERED, WARMUP_STEPS);
  run_controller(RUN_WRITE_COMBINE, WARMUP_STEPS);

  ram = run_controller(RUN_RAM_ONLY, LIFT_STEPS);
  ordered = run_controller(RUN_ORDERED, LIFT_STEPS);
  combined = run_controller(RUN_WRITE_COMBINE, LIFT_STEPS);

  pass = ram.checksum == LIFT_CHECKSUM &&
         ordered.checksum == LIFT_CHECKSUM &&
         combined.checksum == LIFT_CHECKSUM;
  ordered_submit = ordered.cycles - ram.cycles;
  combined_submit = combined.cycles - ram.cycles;

  printf("TACLeBench lift: %s\n", pass ? "PASS" : "FAIL");
  printf("checksum: ram=%d ordered=%d combined=%d expected=%d\n",
         ram.checksum, ordered.checksum, combined.checksum, LIFT_CHECKSUM);
  printf("cycles: control=%u ordered=%u combined=%u\n",
         ram.cycles, ordered.cycles, combined.cycles);
  printf("submission: ordered=%u combined=%u\n",
         ordered_submit, combined_submit);
  print_ratio("submission speedup", ordered_submit, combined_submit);
  print_ratio("end-to-end speedup", ordered.cycles, combined.cycles);

  return pass ? 0 : 1;
}
