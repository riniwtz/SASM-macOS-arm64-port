; C symbols on Darwin have a leading underscore. Keep SASM's main/CMAIN
; convention while also accepting programs that already use _main.
%ifndef SASM_MACOS_INC
%define SASM_MACOS_INC
; Identify the object as macOS code for Apple's linker (LC_BUILD_VERSION).
%pragma macho build_version macos,11,0
default rel
%define main _main
%define CMAIN _main
%define printf _printf
%define scanf _scanf
%define getchar _getchar
%define putchar _putchar
%define fgets _fgets
%define puts _puts
%define fputs _fputs
%define fflush _fflush
%define malloc _malloc
%define free _free
%define exit _exit
%define get_stdin _get_stdin
%define get_stdout _get_stdout
%endif
