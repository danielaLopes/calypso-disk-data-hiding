#ifndef DEBUG_H
#define DEBUG_H

#include "global.h"

/* debug messages */
#define debug_args(kern_lvl, func, fmt, ...) printk(kern_lvl "%s: [%s] " fmt, CALYPSO_DEV_NAME, func, ##__VA_ARGS__);
#define debug(kern_lvl, func, fmt) printk(kern_lvl "%s: [%s] " fmt, CALYPSO_DEV_NAME, func);
// #define debug_args(kern_lvl, func, fmt, ...) trace_printk("%s: [%s] " fmt, CALYPSO_DEV_NAME, func, ##__VA_ARGS__);
// #define debug(kern_lvl, func, fmt) trace_printk("%s: [%s] " fmt, CALYPSO_DEV_NAME, func);

#endif