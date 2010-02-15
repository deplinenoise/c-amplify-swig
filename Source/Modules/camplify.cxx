/* -----------------------------------------------------------------------------
*
* camplify.cxx
*
* A c-amplify generator.
*
* ----------------------------------------------------------------------------- */

#include "swigmod.h"
#include "camplify_type.h"

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
	virtual int classDeclaration(Node *n);

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

		String *ffitype = camplify_type_string(n, Getattr(p, "type"), 1);

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
	String *ffitype = camplify_type_string(n, Getattr(n, "type"), 1);
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
    Printf(f_cl, "\n(def-c-type %s %s)\n", Getattr(n, "name"), camplify_type_string(n, Getattr(n, "type"), 0));
  //}

  return Language::typedefHandler(n);
}

// Includes structs
int CAMPLIFY::classDeclaration(Node *n) 
{
	String *name = Getattr(n, "sym:name");
	String *kind = Getattr(n, "kind");

	// TODO: union?
	if (Strcmp(kind, "struct")) {
		Printf(stderr, "Don't know how to deal with %s kind of class yet.\n", kind);
		Printf(stderr, " (name: %s)\n", name);
		SWIG_exit(EXIT_FAILURE);
	}

	Printf(f_cl, "\n(def-c-struct %s", name);

	for (Node *c = firstChild(n); c; c = nextSibling(c)) {

		if (Strcmp(nodeType(c), "cdecl")) {
			Printf(stderr, "Structure %s has a slot that we can't deal with.\n", name);
			Printf(stderr, "nodeType: %s, name: %s, type: %s\n", nodeType(c), Getattr(c, "name"), Getattr(c, "type"));
			SWIG_exit(EXIT_FAILURE);
		}

		String *temp = Copy(Getattr(c, "decl"));
		Append(temp, Getattr(c, "type"));	//appending type to the end, otherwise wrong type
		String *lisp_type = camplify_type_string(n, temp, 0);
		Delete(temp);

		String *slot_name = Getattr(c, "sym:name");
		Printf(f_cl, "\n\t(%s %s)", slot_name, lisp_type);

		Delete(lisp_type);
	}

	Printf(f_cl, ")\n");

	return SWIG_OK;
}

extern "C" Language *swig_camplify(void) 
{
	return new CAMPLIFY();
}
