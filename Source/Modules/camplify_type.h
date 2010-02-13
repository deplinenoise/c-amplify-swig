#ifndef CAMPLIFY_TYPE_H
#define CAMPLIFY_TYPE_H

#include "swigmod.h"

// Return a c-amplify type string from a SwigType
extern String *camplify_type_string(Node *n, SwigType *ty, int is_function);

#endif