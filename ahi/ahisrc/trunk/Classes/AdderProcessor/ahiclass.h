#ifndef AHI_Classes_AdderProcessor_ahiclass_h
#define AHI_Classes_AdderProcessor_ahiclass_h

#include <classes/ahi_types.h>
#include <exec/lists.h>

#include "boopsi.h"

struct ClassData {
    struct CommonBase common;
};

#define MAX_CHILDREN 64

struct ObjectData {
    ULONG   channels;
    Object* buffer;
    float*  data;
    ULONG   length;
    Object* children[MAX_CHILDREN];
    Object* buffers[MAX_CHILDREN];
    float*  datas[MAX_CHILDREN];
};

#endif /* AHI_Classes_AdderProcessor_ahiclass_h */
