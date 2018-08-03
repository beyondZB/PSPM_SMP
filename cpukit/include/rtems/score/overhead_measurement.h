#ifndef OVERHEAD_MEASUREMENT
#define OVERHEAD_MEASUREMENT

/* Use for obtain overheads of kernel codes */
void pspm_smp_start_count();
void pspm_smp_end_count();
void pspm_smp_print_count();

#endif
