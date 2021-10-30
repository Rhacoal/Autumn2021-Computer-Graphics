#include <simcl.h>

static thread_local int workItemId[16];

int get_global_id(uint dimindx) {
    return workItemId[dimindx];
}
