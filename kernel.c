#include "vga.h"
#include "printf.h"
#include "pci.h"
#include "multiboot.h"
#include "highmem_mapping.h"
#include "acpi.h"
#include "lapic.h"
#include "ioapic.h"
//#include "ebda.h"
#include "segment.h"
#include "interrupt.h"
#include "mm.h"
//#include "task.h"
#include "sched.h"

extern void* _kernel_start_virtual;
extern void* _kernel_end_virtual;
extern void* data;
struct task_t * th_u,* th_k;
struct multiboot;

void dump_multiboot_modules(struct multiboot *mb) {
    if (!mb) {
        printf("no multiboot info\n");
        return;
    }
    if (!(mb->flags & MULTIBOOT_FLAG_MODS)) {
        printf("no modules\n");
        return;
    }

    uint32_t count = mb->mods_count;
    uint32_t mods_addr = mb->mods_addr; // 这通常是物理地址 (GRUB)
    multiboot_module_t *mods = (multiboot_module_t *)phys_to_virt(mods_addr);

    printf("multiboot modules count=%u\n", count);
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t start = mods[i].mod_start;
        uint32_t end   = mods[i].mod_end;
        uint32_t strp  = mods[i].string;
        const char *name = (const char *)phys_to_virt(strp); // 字符串地址也可能需要转
        printf("mod %u: start=0x%x end=0x%x name=%s\n", i, start, end, name);
    }
}


int
kernel_main(struct multiboot * mb)
{
        vga_init();
        //disable_cursor();
        vga_setcolor(COLOR_GREEN, COLOR_BLACK);
        printf("Kernel Booted!\n");
        printf("Physical: 0x%x\n", 0x100000);
        printf("Virtual:  0x%x\n", 0xC0000000);      
        //mod = (multiboot_module_t *)phys_to_virt(mb->mods_addr);
        
        mpinit();
        init_highmem_mapping();
        //acpi_init();
        //if(km_init()==0){
          //printf("mem init is ok\n");  
        //}

        pci_init();
        lapicinit();
        ioapicinit();
        seginit();
        tss_init();
        tvinit();
        printf("pci lapic ioapic segment idt , init is ok\n");
        idtinit();

        task_t *th_k=init_task(0);
        printf("start kernel task\n");  
        start_task_kernel(th_k,kernel_task_main);
        task_t *th_u=init_task(1);

        dump_multiboot_modules(mb);
        multiboot_info=mb;
        
        printf("start user task \n");  
        start_task_user(th_u,user_task_main);
        printf("user task 0x%x kernel task 0x%x\n",th_u,th_k);

        efficient_scheduler_loop();
	return (42);
}

