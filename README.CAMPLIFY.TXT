C-Amplify notes:

A new language backend has been added as Source\Modules\camplify.cxx.

A Visual Studio 2008 project is available in MSVC\swig\swig.sln.

Run with:
	swig.exe -Ilib -camplify Tests\Test1.i

	-Ilib is for finding swig.swg
	-camplify is the language module to use

