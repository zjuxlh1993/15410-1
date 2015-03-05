/** @file kernel.c
 *  @brief An initial kernel.c
 *
 *  You should initialize things in kernel_main(),
 *  and then run stuff.
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */

#include <common_kern.h>
#include <console.h>
#include <vm.h>
#include <thread.h>
#include <syscall.h>
#include <idt.h>

/* libc includes. */
#include <stdio.h>
#include <simics.h>                 /* lprintf() */

/* multiboot header file */
#include <multiboot.h>              /* boot_info */

/* x86 specific includes */
#include <x86/asm.h>                /* enable_interrupts() */

#include <x86/cr.h>
#include <malloc.h>

/** @brief Kernel entrypoint.
 *
 *  This is the entrypoint for the kernel.
 *
 * @return Does not return
 */
int kernel_main(mbinfo_t *mbinfo, int argc, char **argv, char **envp)
{
    /*
     * When kernel_main() begins, interrupts are DISABLED.
     * You should delete this comment, and enable them --
     * when you are ready.
     */

    //FIXME: hardcoded
    set_esp0((unsigned)malloc(2*PAGE_SIZE));

    idt_init();
    vm_init();

    thread_init();
    new_process();

    clear_console();

    char *arg[] = {NULL};

    if (exec("ck1", arg) < 0)
        lprintf("not idle");

    return 0;
}
