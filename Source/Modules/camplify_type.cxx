/* -----------------------------------------------------------------------------
*
* camplify_type.cxx
*
* ----------------------------------------------------------------------------- */

#include "camplify_type.h"

static String * 
get_class_or_struct(SwigType *ty)
{
	String *str = SwigType_str(ty, 0);
	if (str) {
		const char *s;
		static const char struct_str[] = "struct";
		if (s = Strstr(str, struct_str))
			return NewString(s + sizeof(struct_str));
		static const char class_str[] = "class";
		if (s = Strstr(str, class_str)) 
			return NewString(s + sizeof(class_str));
	}
	return str;
}

static const String * 
find_in_typemap(SwigType *ty, Node *n)
{
	Node *node = NewHash();
	Setattr(node, "type", ty);
	Setfile(node, Getfile(n));
	Setline(node, Getline(n));
	const String *tm = Swig_typemap_lookup("in", node, "", 0);
	Delete(node);
	return tm;
}

static String *
get_function_type_string(SwigType *ty, Node *n, int is_function)
{
	SwigType *cp = Copy(ty);
	SwigType *fn = SwigType_pop_function(cp);
	String *args = NewString("");
	ParmList *pl = SwigType_function_parms(fn);
	if (ParmList_len(pl) != 0) 
		Printf(args, "(:arguments ");
	int argnum = 0, first = 1;
	for (Parm *p = pl; p; p = nextSibling(p), argnum++) {
		String *argname = Getattr(p, "name");
		SwigType *argtype = Getattr(p, "type");
		String *ffitype = camplify_type_string(n, argtype, is_function);

		int tempargname = 0;

		if (!argname) {
			argname = NewStringf("arg%d", argnum);
			tempargname = 1;
		}
		if (!first) 
			Printf(args, "\n\t\t");
		Printf(args, "(%s %s)", argname, ffitype);
		first = 0;
		Delete(ffitype);
		if (tempargname)
			Delete(argname);
	}
	if (ParmList_len(pl) != 0) 
		Printf(args, ")\n");	/* finish arg list */
	String *ffitype = camplify_type_string(n, cp, is_function);
	String *str = NewStringf("(cfunction %s \t\t\t\t(return %s))", args, ffitype);
	Delete(fn);
	Delete(args);
	Delete(cp);
	Delete(ffitype);
	return str;
}

static String *
get_array_type_string(SwigType *ty, Node *n, int is_function)
{
	SwigType *cp = Copy(ty);
	String *array_dim = SwigType_array_getdim(ty, 0);

	if (!Strcmp(array_dim, "")) {	//dimension less array convert to pointer
		Delete(array_dim);
		SwigType_del_array(cp);
		SwigType_add_pointer(cp);
		String *str = camplify_type_string(n, cp, is_function);
		Delete(cp);
		return str;
	} else {
		SwigType_pop_arrays(cp);
		String *inner_type = camplify_type_string(n, cp, is_function);
		Delete(cp);

		int ndim = SwigType_array_ndim(ty);
		String *dimension;
		if (ndim == 1) {
			dimension = array_dim;
		} else {
			dimension = array_dim;
			for (int i = 1; i < ndim; i++) {
				array_dim = SwigType_array_getdim(ty, i);
				Append(dimension, " ");
				Append(dimension, array_dim);
				Delete(array_dim);
			}
			String *temp = dimension;
			dimension = NewStringf("(%s)", dimension);
			Delete(temp);
		}
		String *str;
		if (is_function)
			str = NewStringf("(ptr (array %s %s))", inner_type, dimension);
		else
			str = NewStringf("(array %s %s)", inner_type, dimension);

		Delete(inner_type);
		Delete(dimension);
		return str;
	}
}

static String *
get_pointer_type_string(SwigType *ty, Node *n, int is_function)
{
	SwigType *cp = Copy(ty);
	SwigType_del_pointer(cp);
	String *inner_type = camplify_type_string(n, cp, is_function);

	if (SwigType_isfunction(cp))
		return inner_type;

	SwigType *base = SwigType_base(ty);
	String *base_name = SwigType_str(base, 0);

	String *str = NewStringf("(ptr %s)", inner_type);
	
	Delete(base_name);
	Delete(base);
	Delete(cp);
	Delete(inner_type);
	return str;
}

String *
camplify_type_string(Node *n, SwigType *ty_in, int is_function) 
{
	//SwigType *ty = SwigType_typedef_resolve_all(ty_in);
	SwigType *ty = ty_in;
	const String *tm = 0;
	if (tm = find_in_typemap(ty, n))
		return NewString(tm);
	else if (SwigType_ispointer(ty))
		return get_pointer_type_string(ty, n, is_function);
	else if (SwigType_isarray(ty))
		return get_array_type_string(ty, n, is_function);
	else if (SwigType_isfunction(ty))
		return get_function_type_string(ty, n, is_function);
	else
		return get_class_or_struct(ty);
}
