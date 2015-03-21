/**
 * The 15-410 kernel project.
 * @name loader.c
 *
 * Functions for the loading
 * of user programs from binary
 * files should be written in
 * this file. The function
 * elf_load_helper() is provided
 * for your use.
 */
/*@{*/

/* --- Includes --- */
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <exec2obj.h>
#include <elf_410.h>
#include <eflags.h>
#include <seg.h>
#include <syscall.h>
#include <common_kern.h>
#include <loader.h>
#include <exec_run.h>
#include <macros.h>
#include <cr.h>
#include <vm.h>
#include <simics.h>

// TODO change this (top of stack?)
#define USER_ARGV_TOP ((char*)0xD0000000)
#define USER_STACK_TOP 0xBFFFFFFF
#define USER_STACK_SIZE PAGE_SIZE
#define USER_MODE_CPL 3


//TODO limit arg length

/* --- Local function prototypes --- */


/**
 * Copies data from a file into a buffer.
 *
 * @param filename   the name of the file to copy data from
 * @param offset     the location in the file to begin copying from
 * @param size       the number of bytes to be copied
 * @param buf        the buffer to copy the data into
 *
 * @return returns the number of bytes copied on success; -1 on failure
 */
int getbytes(const char *filename, int offset, int size, char *buf)
{
    int i;
    for (i = 0; i < MAX_NUM_APP_ENTRIES; i++) {
        const exec2obj_userapp_TOC_entry *entry = &exec2obj_userapp_TOC[i];
        if (!strncmp(entry->execname, filename, MAX_EXECNAME_LEN)) {
            if (offset >= entry->execlen) {
                return -1;
            }
            int len = MIN(entry->execlen - offset, size);
            int j;
            for (j = 0; j < size; j++) {
                buf[j] = entry->execbytes[offset+j];
            }
            //memcpy(buf, entry->execbytes + offset, len);
            return len;
        }
    }

    return -1;
}

/**
 * @brief Performs simple checks to determine if a ELF file is valid
 *
 * @param se_hdr simple_elf_t
 * @return False iff the elf fails a validity check.
 */
static bool elf_valid(const simple_elf_t *se_hdr)
{
    if (se_hdr->e_entry < se_hdr->e_txtstart || se_hdr->e_entry >= se_hdr->e_txtstart + se_hdr->e_txtlen) {
        return false;
    }

    if (se_hdr->e_txtstart < USER_MEM_START) {
        return false;
    }

    if (se_hdr->e_datstart < USER_MEM_START && se_hdr->e_datlen != 0) {
        return false;
    }

    if (se_hdr->e_rodatstart < USER_MEM_START && se_hdr->e_rodatlen != 0) {
        return false;
    }

    if (se_hdr->e_bssstart < USER_MEM_START && se_hdr->e_bsslen != 0) {
        return false;
    }

    return true;
}

/** @brief Allocate page-aligned memory one page at a time.

 *  Allows for the allocation of pages to overlap previously allocated memory.
 *
 *  @param start The start of the memory region to allocate.
 *  @param su The length of the memory region to allocate.
 *  @return Void.
 */
static void alloc_pages(unsigned start, unsigned len)
{
    unsigned base;
    for (base = ROUND_DOWN_PAGE(start); base < start + len; base += PAGE_SIZE) {
        new_pages((void*)base, PAGE_SIZE);
    }
}

static int argv_check(char **argv)
{
    int len = 0;
    char **cur = argv;
    while (vm_is_present(cur)) {
        if (*cur == NULL)
            return len;
        len++;
        cur++;
    }

    return -1;
}

static int str_check(char *str)
{
    int len = 0;
    char *cur = str;
    while (vm_is_present(cur)) {
        if (*cur == '\0')
            return len;
        len++;
        cur++;
    }

    return -1;
}


/** @brief Fill memory regions needed to run a program.
 *
 *  @param se_efl The ELF header.
 *  @param argv The function arguments.
 *  @return The inital value of the stack pointer for the function
 *  or 0 on error.
 */
static unsigned fill_mem(const simple_elf_t *se_hdr, char *argv[], bool kernel_mode)
{
    int arglen;
    if ( (arglen = argv_check(argv)) < 0)
        return 0;


    char *bottom_arg_ptr = USER_ARGV_TOP;
    char **tmp_argv;
    if ( (tmp_argv = malloc(sizeof(char*)*arglen)) == NULL)
        return 0;

    new_pages((void*)USER_ARGV_TOP - PAGE_SIZE, PAGE_SIZE);

    int i, j;
    for (i = arglen - 1; i >= 0; i--) {
        if (!kernel_mode && (unsigned)argv[i] < USER_MEM_START)
            return 0; //TODO dealloc

        int stringlen = str_check(argv[i]);

        //if arg is invalid string
        if (stringlen < 0)
            return 0; //TODO dealloc

        // //TODO: alloc all in one?
        // alloc_pages((unsigned)bottom_arg_ptr - (stringlen + 1)*sizeof(char), (stringlen + 1)*sizeof(char));

        j = 0;
        //copy string to memory
        for (j = 0; j < stringlen + 1; j++) {
            bottom_arg_ptr -= sizeof(char);
            *bottom_arg_ptr = argv[i][stringlen - j];
        }
        tmp_argv[i] = bottom_arg_ptr;
    }

    char **new_argv = (char**)(((unsigned)bottom_arg_ptr - sizeof(char*)*arglen) & (~(sizeof(char*)-1))); //align
    // alloc_pages(ROUND_DOWN_PAGE(new_argv), (unsigned)ROUND_UP_PAGE(new_argv - arglen*sizeof(char*)));
    for (j = 0; j < arglen; j++) {
        new_argv[j] = tmp_argv[j];
    }
    // memcpy(new_argv, tmp_argv, sizeof(char*)*arglen);
    free(tmp_argv);

    lprintf("new argv: %p", new_argv);

    alloc_pages(se_hdr->e_txtstart, se_hdr->e_txtlen);
    alloc_pages(se_hdr->e_datstart, se_hdr->e_datlen);
    alloc_pages(se_hdr->e_rodatstart, se_hdr->e_rodatlen);
    alloc_pages(se_hdr->e_bssstart, se_hdr->e_bsslen);

    // TODO make txt and rodata read only?
    if (getbytes(se_hdr->e_fname, se_hdr->e_txtoff, se_hdr->e_txtlen,
        (char *)se_hdr->e_txtstart) < 0) {
        return 0;
    }
    if (getbytes(se_hdr->e_fname, se_hdr->e_datoff, se_hdr->e_datlen,
        (char *)se_hdr->e_datstart) < 0) {
        return 0;
    }
    if (getbytes(se_hdr->e_fname, se_hdr->e_rodatoff, se_hdr->e_rodatlen,
        (char *)se_hdr->e_rodatstart) < 0) {
        return 0;
    }

    memset((char*)se_hdr->e_bssstart, 0, se_hdr->e_bsslen);

    unsigned stack_low = USER_STACK_TOP - USER_STACK_SIZE + 1;
    new_pages((void*)(stack_low), USER_STACK_SIZE);

    unsigned esp = USER_STACK_TOP + 1;

    //push arguments for _main
    esp -= sizeof(int);
    *(int *)esp = stack_low;

    esp -= sizeof(int);
    *(int *)esp = USER_STACK_TOP;

    esp -= sizeof(char**);
    *(char ***)esp = new_argv;

    esp -= sizeof(int);
    *(int*)esp = arglen;

    //push return address (isn't one)
    esp -= sizeof(int);

    return esp;
}

/** @brief Begins running a user program.
 *
 *  @param eip The initial value of the instruction pointer.
 *  @param esp The inital value of the stack pointer.
 *  @return Does not return.
 */
static void user_run(unsigned eip, unsigned esp)
{
    //TODO: should this look at currently set eflags?
    //TODO: more flags?
    set_cr0((get_cr0() & ~CR0_AM & ~CR0_WP) | CR0_PE);
    set_cr4(get_cr4() | CR4_PGE);
    unsigned eflags = EFL_RESV1 | EFL_IF | EFL_IOPL_RING1;
    exec_run(SEGSEL_USER_DS, eip, SEGSEL_USER_CS, eflags, esp, SEGSEL_USER_DS);
}

/** @brief Replaces the program currently running in the invoking task with
 *  the program stored in the file named execname.  The argument points to a
 *  null-terminated vector of null-terminated string arguments.
 *
 *  @param filename The program file name.
 *  @param argv The argument vector.
 *  @return Does not return, returns a negative error code on failure.
 */
int load(char *filename, char *argv[], bool kernel_mode)
{
    if (elf_check_header(filename) != ELF_SUCCESS) {
        return -1;
    }
    simple_elf_t se_hdr;
    if (elf_load_helper(&se_hdr, filename) != ELF_SUCCESS) {
        return -2;
    }
    if (!elf_valid(&se_hdr)) {
        return -3;
    }
    unsigned esp = fill_mem(&se_hdr, argv, kernel_mode);
    if (esp == 0) {
        return -4;
    }

    user_run(se_hdr.e_entry, esp);

    return -5;
}

int exec(char *filename, char *argv[])
{
    //check filename valid
    //FIXME: strnlen?
    int len = strlen(filename) + 1;
    char *file;
    if ( (file = calloc(sizeof(char), len)) == NULL )
        return -6;
    strncpy(file, filename, len);
    int ret = load(file, argv, false);
    free(file);
    return ret;
}

/*@}*/
