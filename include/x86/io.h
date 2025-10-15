#ifndef X86_IO_H
#define X86_IO_H

#include "types.h"

#include "mmu.h"

#define IRQ(n) (n+32)

#define CR0_PG  (1U<<31)  /* Paging enable */
#define CR0_WP  (1U<<16)  /* Write-protect - allow page faults in kernel mode */

static inline void outb(uint16_t port, uint8_t value) {
  __asm__ volatile ("outb %1, %0" : : "dN" (port), "a" (value));
}

static inline void outw(uint16_t port, uint16_t value) {
  __asm__ volatile ("outw %1, %0" : : "dN" (port), "a" (value));
}

static inline void outl(uint16_t port, uint32_t value) {
  __asm__ volatile ("outl %1, %0" : : "dN" (port), "a" (value));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

static inline uint16_t inw(uint16_t port) {
  uint16_t ret;
  __asm__ volatile ("inw %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

static inline uint32_t inl(uint16_t port) {
  uint32_t ret;
  __asm__ volatile ("inl %1, %0" : "=a" (ret) : "dN" (port));
  return ret;
}

static inline uint32_t read_cr0() {
  uint32_t ret;
  __asm__ volatile("mov %%cr0, %0" : "=r" (ret));
  return ret;
}
static inline uint32_t read_cr2() {
  uint32_t ret;
  __asm__ volatile("mov %%cr2, %0" : "=r" (ret));
  return ret;
}
static inline uint32_t read_cr3() {
  uint32_t ret;
  __asm__ volatile("mov %%cr3, %0" : "=r" (ret));
  return ret;
}

static inline void write_cr0(uint32_t val) {
  __asm__ volatile("mov %0, %%cr0" : : "r" (val));
}
static inline void write_cr2(uint32_t val) {
  __asm__ volatile("mov %0, %%cr2" : : "r" (val));
}
static inline void write_cr3(uint32_t val) {
  __asm__ volatile("mov %0, %%cr3" : : "r" (val));
}


// Routines to let C code use special x86 instructions.

/*static inline uint8_t
inb(uint16_t port)
{
  uint8_t data;

  asm volatile("in %1,%0" : "=a" (data) : "d" (port));
  return data;
}*/

static inline void
insl(int port, void *addr, int cnt)
{
  asm volatile("cld; rep insl" :
               "=D" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "memory", "cc");
}

/*
static inline void
outb(uint16_t port, uint8_t data)
{
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}

static inline void
outw(uint16_t port, uint16_t data)
{
  asm volatile("out %0,%1" : : "a" (data), "d" (port));
}*/

static inline void
outsl(int port, const void *addr, int cnt)
{
  asm volatile("cld; rep outsl" :
               "=S" (addr), "=c" (cnt) :
               "d" (port), "0" (addr), "1" (cnt) :
               "cc");
}

static inline void
stosb(void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosb" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}

static inline void
stosl(void *addr, int data, int cnt)
{
  asm volatile("cld; rep stosl" :
               "=D" (addr), "=c" (cnt) :
               "0" (addr), "1" (cnt), "a" (data) :
               "memory", "cc");
}

struct segdesc;

static inline void
lgdt(struct segdesc *p, int size)
{
  volatile uint16_t pd[3];

  pd[0] = size-1;
  pd[1] = (uint32_t)p;
  pd[2] = (uint32_t)p >> 16;

  asm volatile("lgdt (%0)" : : "r" (pd));
}

struct gatedesc;

static inline void
lidt(struct gatedesc *p, int size)
{
  volatile uint16_t pd[3];

  pd[0] = size-1;
  pd[1] = (uint32_t)p;
  pd[2] = (uint32_t)p >> 16;

  asm volatile("lidt (%0)" : : "r" (pd));
}

static inline void
ltr(uint16_t sel)
{
  asm volatile("ltr %0" : : "r" (sel));
}

static inline uint32_t
readeflags(void)
{
  uint32_t eflags;
  asm volatile("pushfl; popl %0" : "=r" (eflags));
  return eflags;
}

static inline void
loadgs(uint16_t v)
{
  asm volatile("movw %0, %%gs" : : "r" (v));
}

static inline void
cli(void)
{
  asm volatile("cli");
}

static inline void
sti(void)
{
  asm volatile("sti");
}

static inline uint32_t
xchg(volatile uint32_t *addr, uint32_t newval)
{
  uint32_t result;

  // The + in "+m" denotes a read-modify-write operand.
  asm volatile("lock; xchgl %0, %1" :
               "+m" (*addr), "=a" (result) :
               "1" (newval) :
               "cc");
  return result;
}

static inline uint32_t
rcr2(void)
{
  uint32_t val;
  asm volatile("movl %%cr2,%0" : "=r" (val));
  return val;
}

static inline void
lcr3(uint32_t val)
{
  asm volatile("movl %0,%%cr3" : : "r" (val));
}

// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe_src {
  // registers as pushed by pusha
  uint32_t edi;
  uint32_t esi;
  uint32_t ebp;
  uint32_t oesp;      // useless & ignored
  uint32_t ebx;
  uint32_t edx;
  uint32_t ecx;
  uint32_t eax;

  // rest of trap frame
  uint16_t gs;
  uint16_t padding1;
  uint16_t fs;
  uint16_t padding2;
  uint16_t es;
  uint16_t padding3;
  uint16_t ds;
  uint16_t padding4;
  uint32_t trapno;

  // below here defined by x86 hardware
  uint32_t err;
  uint32_t eip;
  uint16_t cs;
  uint16_t padding5;
  uint32_t eflags;

  // below here only when crossing rings, such as from user to kernel
  uint32_t esp;
  uint16_t ss;
  uint16_t padding6;
};

#endif
