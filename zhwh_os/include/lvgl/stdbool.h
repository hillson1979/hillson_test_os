/**
 * @file stdbool.h
 * @brief stdbool.h 兼容层
 */

#ifndef _STDBOOL_H
#define _STDBOOL_H

#ifdef __cplusplus
#define bool  bool
#define false false
#define true  true
#else
typedef enum {
    false = 0,
    true  = 1
} bool;
#endif

#define __bool_true_false_are_defined 1

#endif /* _STDBOOL_H */
