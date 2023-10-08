#include <stdlib.h>
#include <stdio.h>
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <dirent.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <threads.h>

static void term_clear() {
    printf("\033[2J");
}

static _Atomic(int) current_i = 0;

static void file_list(const char *path, int deep_counter) {
    assert(path);
    DIR *dir = opendir(path);
    assert(dir);
    struct dirent *entry = NULL;
    while ((entry = readdir(dir))) {
        switch (entry->d_type) {
            case DT_DIR: {
                if (!strcmp(entry->d_name, ".") || 
                    !strcmp(entry->d_name, ".."))
                    break;

                if (deep_counter == 0)
                    break;

                //printf("file_list: directory %s\n", entry->d_name);

                char new_path[1024] = {};

                strcat(new_path, path);
                strcat(new_path, "/");
                strcat(new_path, entry->d_name);

                file_list(new_path, deep_counter - 1);
                break;
            }
            case DT_REG: {
                //printf("search_files_rec: regular %s\n", entry->d_name);

                char fname[1024] = {};
                strcat(fname, path);
                strcat(fname, "/");
                strcat(fname, entry->d_name);
                printf("%s\n", fname);

                break;
            }
        }

    }

}

static int thread_mem_usage_printing(void *arg) {
    for (;;) {
        term_clear();
        FILE *file = fopen("/proc/self/statm", "r");
        assert(file);
        //char line[256] = {};
        char *line = NULL;
        size_t len = 0;
        //size_t ret = getline((char**)&line, &len, file);
        size_t ret = getline(&line, &len, file);
        assert(line);
        //printf("line '%s'\n", line);
        //printf("len %zu\n", len);
        //printf("ret %zu\n", ret);
        unsigned int size, resident, shared, text, lib, data, dt;
        sscanf(
            line, "%u %u %u %u %u %u %u",
            &size, &resident, &shared, &text, &lib, &data, &dt
        );
        int _current_i = current_i ? current_i : 1;
        float average_vm_size = data / (float)_current_i;
        printf(
            "size %u\n"
            "resident %u\n"
            "shared %u\n"
            "text %u\n"
            "lib %u\n"
            "data %u\n"
            "dt %u\n"
            "average vm size %.3f\n", 
            size, resident, shared, text, lib, data, dt, average_vm_size
        );
        free(line);
        fclose(file);
        usleep(1000);
    }
    return 0;
}

static int thread_mem_allocating(void *arg) {
    const int num = 1024 * 100;
    lua_State **vms = calloc(num, sizeof(vms[0]));
    for (int i = 0; i < num; i++) {
        current_i = i;
        vms[i] = luaL_newstate();
        assert(vms[i]);
        usleep(10000);
    }
    return 0;
}

int main() {

    //luaL_dostring(vm, 
//"local file = io.open(/proc/"
    //);
    file_list("/proc/self", 15);
    //lua_close(vm);
    thrd_t job_printing, job_allocating;
    int ret;
    ret = thrd_create(&job_printing, thread_mem_usage_printing, NULL);
    assert(ret == thrd_success);
    ret = thrd_create(&job_allocating, thread_mem_allocating, NULL);
    assert(ret == thrd_success);
    for (;;) {
        usleep(100);
    }
    return EXIT_SUCCESS;
}
