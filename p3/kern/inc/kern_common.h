/** @file kern_common.h
 *  @brief An interface for commonly used macros and functions.
 *
 *  @author Patrick Koenig (phkoenig)
 *  @author Jack Sorrell (jsorrell)
 *  @bug No known bugs.
 */

#ifndef _KERN_COMMON_H
#define _KERN_COMMON_H

#include <syscall.h>
#include <string.h>

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#define PAGE_MASK (~(PAGE_SIZE - 1))
#define ROUND_DOWN_PAGE(ADDR) ((unsigned)(ADDR) & PAGE_MASK)
#define ROUND_UP_PAGE(ADDR) (ROUND_DOWN_PAGE((ADDR) + PAGE_SIZE - 1))

typedef enum {
    false = 0,
    true
} bool;

int null_arr_check(char *arr[]);
int str_check(char *str);

#endif /* _KERN_COMMON_H */