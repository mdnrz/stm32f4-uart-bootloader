#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    char *build_target;
    char *compiler;
    char *header_path;
    char *lib_path;
    char *output;
    char *lflags;

    // skip first argument: program name
    nob_shift_args(&argc, &argv);

    Nob_Cmd build_app = {0};

    if (argc > 0) build_target = nob_shift_args(&argc, &argv);
    else  build_target = "LINUX";

    nob_log(NOB_INFO, "build target: %s", build_target);

    if (strcmp(build_target, "LINUX") == 0) {
        nob_cmd_append(&build_app, "gcc");
        nob_cmd_append(&build_app, "-Wall", "-Wextra", "-ggdb");
        nob_cmd_append(&build_app, "-D", build_target);
        nob_cmd_append(&build_app, "-o", "./build/bl", "./src/bl.c", "./src/core/ser.c", "./src/core/comm.c");
        nob_cmd_append(&build_app, "-I./src/core/", "-I./ncurses-6.3/include/", "-L./ncurses-6.3/lib", "-lncurses");
    } else if (strcmp(build_target, "WIN32") == 0) {
        nob_cmd_append(&build_app, "x86_64-w64-mingw32-gcc");
        nob_cmd_append(&build_app, "-Wall", "-Wextra", "-s");
        nob_cmd_append(&build_app, "-D", build_target);
        nob_cmd_append(&build_app, "-o", "./build/classifier.exe", "./win-scandir/win-scandir.c", "classifier.c");
        nob_cmd_append(&build_app, "-I./win-scandir", "-I./raylib-win/include", "-L./raylib-win/mylib-opengl-21/", "-lraylib", "-lgdi32", "-lwinmm");
    }

    /*nob_cmd_append(&build_app, "-lm");*/
    if (!nob_cmd_run_sync(build_app)) return 1;
    return 0;
}
