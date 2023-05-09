#ifndef DYN_ARRAY
#define DYN_ARRAY

#include "pe_common.h"

#define INITIAL_CAPACITY (2)

extern void dyn_resize(dyn_array *dyn);
extern dyn_array* dyn_array_init();
extern void dyn_array_add(dyn_array *dyn, void* value);
extern void dyn_array_delete(dyn_array *dyn, int index);
extern void* dyn_array_get(dyn_array *dyn, int index);
extern void dyn_array_free(dyn_array *dyn);
extern void dyn_array_free_values(dyn_array* dyn); // free malloced pointers in dyn
extern void dyn_array_free_traders(dyn_array* dyn); // free everything inside traders
extern void dyn_array_delete_trader(dyn_array* dyn, int index);

#endif
