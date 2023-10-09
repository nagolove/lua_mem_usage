#include <stddef.h>
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
#include <unistd.h>

static void term_clear() {
    printf("\033[2J");
}

static _Atomic(int) current_i = 0;
static _Atomic(long int) rss_start = 0;

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
    unsigned int size, resident, shared, text, lib, data, dt;
    FILE *file = fopen("/proc/self/statm", "r");
    char *line = NULL;
    assert(file);

    size_t len = 0;
    size_t ret = getline(&line, &len, file);
    assert(line);
    sscanf(
        line, "%u %u %u %u %u %u %u",
        &size, &resident, &shared, &text, &lib, &data, &dt
    );
    rss_start = resident;
    free(line);
    fclose(file);

    for (;;) {
        term_clear();

        file = fopen("/proc/self/statm", "r");
        size_t ret = getline(&line, &len, file);
        assert(line);
        sscanf(
            line, "%u %u %u %u %u %u %u",
            &size, &resident, &shared, &text, &lib, &data, &dt
        );

        int _current_i = current_i ? current_i : 1;
        float average_vm_size = (resident - rss_start) / (float)_current_i;
        const int pgsz = 4096;
        const float megabt = 1024. * 1024.;
        printf("rss_start %lu\n", rss_start);
        printf("current_i: %d\n", _current_i);
        printf(
            "size %.3f\n"
            "resident %.3f\n"
            "shared %.3f\n"
            "text %.3f\n"
            "lib %.3f\n"
            "data %.3f\n"
            "dt %.3f\n"
            "average vm size %.3f\n" 
            "rss %.3f\n",
            size * pgsz / megabt, 
            resident * pgsz / megabt,
            shared * pgsz / megabt,
            text * pgsz / megabt, 
            lib * pgsz / megabt, 
            data * pgsz / megabt, 
            dt * pgsz / megabt, 
            average_vm_size,
            (resident + text + data) * pgsz / megabt
        );

        pid_t pid = getpid();
        printf("pid %d\n", pid);

        if (line) {
            free(line);
            line = NULL;
        }
        fclose(file);
        usleep(1000);
    }
    return 0;
}

static const char *code = 
"local arr = {}\n"
"for i = 1, 1024 * 1 do\n"
" local r = {}\n"
" for j = 1, 1 do\n"
"  r[j] = j + i\n"
" end\n"
" arr[i] = r\n"
"end";

static int thread_mem_allocating(void *arg) {

    while (!rss_start) {
        usleep(100);
    }
    
    /*
    const int num = 1024 * 100;
    lua_State **vms = calloc(num, sizeof(vms[0]));
    for (int i = 0; i < num; i++) {
        current_i = i;
        vms[i] = luaL_newstate();
        //luaL_openlibs(vms[i]);
        if (luaL_dostring(vms[i], code) != LUA_OK) {
            printf("err '%s'\n", lua_tostring(vms[i], -1));
            //abort();
            exit(EXIT_FAILURE);
        }
        lua_gc(vms[i], LUA_GCCOLLECT);
        assert(vms[i]);
        usleep(10000);
    }
    //*/

    const int num = 1024 * 100;
    char **vms = calloc(num, sizeof(vms[0]));
    for (int i = 0; i < num; i++) {
        current_i = i;
        size_t sz = 1024 * 8;
        vms[i] = malloc(sz);
        assert(vms[i]);
        for (int j = 0; j < sz; j++) {
            vms[i][j] = j * 2;
        }
        assert(vms[i]);
        usleep(10000);
    }
    // */

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
