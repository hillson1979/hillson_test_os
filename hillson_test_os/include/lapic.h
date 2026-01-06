// lapic.c
//void            cmostime(struct rtcdate *r);
int             lapicid(void);
extern volatile uint32_t*    lapic;
void            lapiceoi(void);
void            lapicinit(void);
void            lapicstartap(uint8_t, uint32_t);
void            microdelay(int);

uint8_t logical_cpu_id(void);
uint32_t cpu_id(void);
/** @brief Local APIC ID Register (read-only). */
#define APIC_ID         0x0020
#define INVALID_CPU_ID 255
