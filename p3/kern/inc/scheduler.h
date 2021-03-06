/** @file scheduler.h
 *  @brief Function declarations for scheduler
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */

#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include <linklist.h>
#include <proc.h>

extern linklist_t scheduler_queue;

int scheduler_init();
void scheduler_tick(unsigned ticks);
int context_switch(tcb_t *tcb);
int deschedule_kern(int *flag, bool user);
int make_runnable_kern(tcb_t *tcb, bool user);


#endif /* _SCHEDULER_INIT_H */
