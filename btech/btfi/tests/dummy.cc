// Just a hack to get Automake to link a C program using the C++ linker.
// Alternately, you can link with -lstdc++.  This might not do everything that
// needs to be done for linking in C++-based libraries, though.
// Even this might not be enough; some cases involving statically-initialized
// C++ objects require compiling main() with the C++ compiler as well.

extern "C" int real_main(int, char *[]);

int
main(int argc, char *argv[])
{
	return real_main(argc, argv);
}
