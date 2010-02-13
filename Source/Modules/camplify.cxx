/* -----------------------------------------------------------------------------
*
* camplify.cxx
*
* A c-amplify generator.
*
* ----------------------------------------------------------------------------- */

#include "swigmod.h"

static String *get_type_string(Node *n, SwigType *ty, int is_function);

class CAMPLIFY : public Language 
{
public:
	CAMPLIFY();
	virtual void main(int argc, char *argv[]);
	virtual int top(Node *n);
	virtual int functionWrapper(Node *n);
	virtual int variableWrapper(Node *n);
	virtual int constantWrapper(Node *n);
	virtual int enumDeclaration(Node *n);
	virtual int typedefHandler(Node *n);

private:
	String *module;
	File *f_cl;
	List *entries;
	int extern_all_flag;
};

CAMPLIFY::CAMPLIFY()
{
	module = 0;
	f_cl = 0;
	entries = 0;
	extern_all_flag = 0;
}

void CAMPLIFY::main(int argc, char *argv[])
{
}

int CAMPLIFY::top(Node *n)
{
	// Set up an output file and initialize swig

	module = Getattr(n, "name");

	String *output_filename;
	entries = NewList();

	/* Get the output file name */
	String *outfile = Getattr(n, "outfile");

	if (!outfile) {
		output_filename = outfile;
	} else {
		output_filename = NewString("");
		Printf(output_filename, "%s%s.ca", SWIG_output_directory(), module);
	}

	f_cl = NewFile(output_filename, "w", SWIG_output_files());
	if (!f_cl) {
		FileErrorDisplay(output_filename);
		SWIG_exit(EXIT_FAILURE);
	}

	File *f_null = NewString("");
	Swig_register_filebyname("header", f_null);
	Swig_register_filebyname("begin", f_null);
	Swig_register_filebyname("runtime", f_null);
	Swig_register_filebyname("wrapper", f_null);

	String *header = NewString("");

	Swig_banner_target_lang(header, ";;");

	Printf(header, "\n(defpackage :%s\n  (:use :common-lisp :ffi)", module);

	// Parse the input and invoke all callbacks defined in this class
	Language::top(n);

	Close(f_cl);
	Delete(f_cl);
	return SWIG_OK;
}

int CAMPLIFY::functionWrapper(Node *n)
{
	String *storage = Getattr(n, "storage");
	if (!extern_all_flag && (!storage || (Strcmp(storage, "extern") && Strcmp(storage, "externc"))))
		return SWIG_OK;

	String *func_name = Getattr(n, "sym:name");

	ParmList *pl = Getattr(n, "parms");

	int argnum = 0, first = 1;

	Printf(f_cl, "\n(defun/extern %s\n\t(:name \"%s\")\n", func_name, func_name);

	Append(entries, func_name);

	if (ParmList_len(pl) != 0) {
		Printf(f_cl, "\t(:arguments ");
	}
	for (Parm *p = pl; p; p = nextSibling(p), argnum++) {

		String *argname = Getattr(p, "name");
		//    SwigType *argtype;

		String *ffitype = get_type_string(n, Getattr(p, "type"), 1);

		int tempargname = 0;

		if (!argname) {
			argname = NewStringf("arg%d", argnum);
			tempargname = 1;
		}

		if (!first) {
			Printf(f_cl, "\n\t\t");
		}
		Printf(f_cl, "(%s %s)", argname, ffitype);
		first = 0;

		Delete(ffitype);

		if (tempargname)
			Delete(argname);
	}
	if (ParmList_len(pl) != 0) {
		Printf(f_cl, ")\n");	/* finish arg list */
	}
	String *ffitype = get_type_string(n, Getattr(n, "type"), 1);
	if (Strcmp(ffitype, "NIL")) {	//when return type is not nil
		Printf(f_cl, "\t(:return %s))\n", ffitype);
	} else {
		Printf(f_cl, ")\n", ffitype);
	}

	return SWIG_OK;
}

int CAMPLIFY::variableWrapper(Node *n)
{
	return SWIG_OK;
}

int CAMPLIFY::constantWrapper(Node *n)
{
	String *name = Getattr(n, "name");
//	Printf(f_cl, "Constant: %s\n", name);

	return SWIG_OK;
}

int CAMPLIFY::enumDeclaration(Node *n)
{
	return SWIG_OK;
}

int CAMPLIFY::typedefHandler(Node *n)
{
  //if (generate_typedef_flag) {
    Printf(f_cl, "\n(def-c-type %s %s)\n", Getattr(n, "name"), get_type_string(n, Getattr(n, "type"), 0));
  //}

  return Language::typedefHandler(n);
}

String * get_class_or_struct( SwigType * ty )
{
	String *str = SwigType_str(ty, 0);
	if (str) {
		char *st = Strstr(str, "struct");
		if (st) {
			st += 7;
			return NewString(st);
		}
		char *cl = Strstr(str, "class");
		if (cl) {
			cl += 6;
			return NewString(cl);
		}
	}
	return str;
}

const String* find_in_typemap( SwigType * ty, Node * n )
{
	Node *node = NewHash();
	Setattr(node, "type", ty);
	Setfile(node, Getfile(n));
	Setline(node, Getline(n));
	const String *tm = Swig_typemap_lookup("in", node, "", 0);
	Delete(node);
	return tm;
}

String * get_function_type_string( SwigType * ty, Node * n, int is_function )
{
	SwigType *cp = Copy(ty);
	SwigType *fn = SwigType_pop_function(cp);
	String *args = NewString("");
	ParmList *pl = SwigType_function_parms(fn);
	if (ParmList_len(pl) != 0) {
		Printf(args, "(:arguments ");
	}
	int argnum = 0, first = 1;
	for (Parm *p = pl; p; p = nextSibling(p), argnum++) {
		String *argname = Getattr(p, "name");
		SwigType *argtype = Getattr(p, "type");
		String *ffitype = get_type_string(n, argtype, is_function);

		int tempargname = 0;

		if (!argname) {
			argname = NewStringf("arg%d", argnum);
			tempargname = 1;
		}
		if (!first) {
			Printf(args, "\n\t\t");
		}
		Printf(args, "(%s %s)", argname, ffitype);
		first = 0;
		Delete(ffitype);
		if (tempargname)
			Delete(argname);
	}
	if (ParmList_len(pl) != 0) {
		Printf(args, ")\n");	/* finish arg list */
	}
	String *ffitype = get_type_string(n, cp, is_function);
	String *str = NewStringf("(cfunction %s \t\t\t\t(return %s))", args, ffitype);
	Delete(fn);
	Delete(args);
	Delete(cp);
	Delete(ffitype);
	return str;
}

String * get_array_type_string( SwigType * ty, Node * n, int is_function )
{
	SwigType *cp = Copy(ty);
	String *array_dim = SwigType_array_getdim(ty, 0);

	if (!Strcmp(array_dim, "")) {	//dimension less array convert to pointer
		Delete(array_dim);
		SwigType_del_array(cp);
		SwigType_add_pointer(cp);
		String *str = get_type_string(n, cp, is_function);
		Delete(cp);
		return str;
	} else {
		SwigType_pop_arrays(cp);
		String *inner_type = get_type_string(n, cp, is_function);
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

String * get_pointer_type_string( SwigType * ty, Node * n, int is_function )
{
	SwigType *cp = Copy(ty);
	SwigType_del_pointer(cp);
	String *inner_type = get_type_string(n, cp, is_function);

	if (SwigType_isfunction(cp)) {
		return inner_type;
	}

	SwigType *base = SwigType_base(ty);
	String *base_name = SwigType_str(base, 0);

	String *str;
	if (!Strcmp(base_name, "int") || !Strcmp(base_name, "float") || !Strcmp(base_name, "short")
		|| !Strcmp(base_name, "double") || !Strcmp(base_name, "long") || !Strcmp(base_name, "char")) {

			str = NewStringf("(ptr %s)", inner_type);
	} else {
		str = NewStringf("(ptr %s)", inner_type);
	}
	Delete(base_name);
	Delete(base);
	Delete(cp);
	Delete(inner_type);
	return str;
}

String *get_type_string(Node *n, SwigType *ty, int is_function) 
{
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
extern "C" Language *swig_camplify(void) 
{
	return new CAMPLIFY();
}
