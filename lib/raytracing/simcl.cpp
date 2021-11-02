#include <simcl.h>

static thread_local int workItemId[16];

void set_global_id(uint dimindx, uint value) {
    workItemId[dimindx] = value;
}

int get_global_id(uint dimindx) {
    return workItemId[dimindx];
}
