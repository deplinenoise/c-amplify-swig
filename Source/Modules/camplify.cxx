/* -----------------------------------------------------------------------------
*
* camplify.cxx
*
* A c-amplify generator.
*
* ----------------------------------------------------------------------------- */

#include "swigmod.h"

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
	String *CAMPLIFY::get_ffi_type(Node *n, SwigType *ty);

private:
	String *module;
	File *f_cl;
	List *entries;
	int extern_all_flag;
	int is_function;
};

CAMPLIFY::CAMPLIFY()
{
	module = 0;
	f_cl = 0;
	entries = 0;
	extern_all_flag = 0;
	is_function = 0;
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

	// Write out all collected data
	Iterator i;

	//long len = Len(entries);
	//if (len > 0) {
	//	Printf(header, "\n  (:export");
	//}
	////else nothing to export

	//for (i = First(entries); i.item; i = Next(i)) {
	//	Printf(header, "\n\t:%s", i.item);
	//}

	//if (len > 0) {
	//	Printf(header, ")");
	//}

	//Printf(header, ")\n");
	//Printf(header, "\n(in-package :%s)\n", module);
	//Printf(header, "\n(default-foreign-language :stdc)\n");

	//len = Tell(f_cl);

	//Printf(f_cl, "%s", header);

	//long end = Tell(f_cl);

	//for (len--; len >= 0; len--) {
	//	end--;
	//	Seek(f_cl, len, SEEK_SET);
	//	int ch = Getc(f_cl);
	//	Seek(f_cl, end, SEEK_SET);
	//	Putc(ch, f_cl);
	//}

	//Seek(f_cl, 0, SEEK_SET);
	//Write(f_cl, Char(header), Len(header));

	Close(f_cl);
	Delete(f_cl);			// Deletes the handle, not the file
	return SWIG_OK;
}

int CAMPLIFY::functionWrapper(Node *n)
{
	is_function = 1;
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

		String *ffitype = get_ffi_type(n, Getattr(p, "type"));

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
	String *ffitype = get_ffi_type(n, Getattr(n, "type"));
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
    is_function = 0;
    Printf(f_cl, "\n(def-c-type %s %s)\n", Getattr(n, "name"), get_ffi_type(n, Getattr(n, "type")));
  //}

  return Language::typedefHandler(n);
}

String *CAMPLIFY::get_ffi_type(Node *n, SwigType *ty) 
{
	Node *node = NewHash();
	Setattr(node, "type", ty);
	Setfile(node, Getfile(n));
	Setline(node, Getline(n));
	const String *tm = Swig_typemap_lookup("in", node, "", 0);
	Delete(node);

	if (tm) {
		return NewString(tm);
	} else if (SwigType_ispointer(ty)) {
		SwigType *cp = Copy(ty);
		SwigType_del_pointer(cp);
		String *inner_type = get_ffi_type(n, cp);

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
	} else if (SwigType_isarray(ty)) {
		SwigType *cp = Copy(ty);
		String *array_dim = SwigType_array_getdim(ty, 0);

		if (!Strcmp(array_dim, "")) {	//dimension less array convert to pointer
			Delete(array_dim);
			SwigType_del_array(cp);
			SwigType_add_pointer(cp);
			String *str = get_ffi_type(n, cp);
			Delete(cp);
			return str;
		} else {
			SwigType_pop_arrays(cp);
			String *inner_type = get_ffi_type(n, cp);
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
	} else if (SwigType_isfunction(ty)) {
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
			String *ffitype = get_ffi_type(n, argtype);

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
		String *ffitype = get_ffi_type(n, cp);
		String *str = NewStringf("(cfunction %s \t\t\t\t(return %s))", args, ffitype);
		Delete(fn);
		Delete(args);
		Delete(cp);
		Delete(ffitype);
		return str;
	}
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
extern "C" Language *swig_camplify(void) 
{
	return new CAMPLIFY();
}
