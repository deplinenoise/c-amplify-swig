%module test1 

// functionWrapper
extern int foo(const char *c);

// SWIG does not understand __declspec
//extern __declspec(dllimport) const int** bar(char x, volatile const char *** const c);

extern const int** bar(char x, const char *** const c);

typedef int (*func)(const char*);

typedef int MyInt;

extern MyInt baz(MyInt);

extern func get_func(void);

// enumDeclaration
enum apa 
{ 
	ffff = 0
};

// variableWrapper
const int i = 10;

struct my_struct {
	int x;
	int y;
	MyInt myInt;
};

struct my_struct2 {
	struct my_struct2 *next;
	struct my_struct inline_struct;
	char array[23];
};

#define MYDEF 199

