/** @file fork.c
 *  @brief Implements fork
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */
#include <fork.h>
#include <scheduler.h>
#include <proc.h>
#include <simics.h>
#include <proc.h>
#include <syscall.h>
#include <context_switch.h>
#include <asm.h>
#include <cr.h>
#include <vm.h>
#include <string.h>
#include "driver_core.h"

int fork()
{
    tcb_t *old_tcb;
    hashtable_get(&tcbs, gettid(), (void**)&old_tcb);

    disable_interrupts();
    
    unsigned old_esp0 = get_esp0();
    
    int tid = proc_new_process();
    if (tid < 0) {
        return -1;
    }
        
    if (store_regs(&old_tcb->regs)) {
        cur_tid = tid;
    
        unsigned new_esp0 = get_esp0();

        int i;
        for (i = 0; i < KERNEL_STACK_SIZE; i++)
            ((char *)(new_esp0 - KERNEL_STACK_SIZE))[i] = ((char *)(old_esp0 - KERNEL_STACK_SIZE))[i];
        // memcpy((void *)(new_esp0 - KERNEL_STACK_SIZE), (void *)(old_esp0 - KERNEL_STACK_SIZE), KERNEL_STACK_SIZE);

        set_cr3((unsigned)vm_copy());

        enable_interrupts();
        return 0;
    } else {
        cur_tid = old_tcb->tid;
    
        notify_interrupt_complete(); //we are returning from timer but didn't come from timer
        enable_interrupts();
        return tid;
    }
}
