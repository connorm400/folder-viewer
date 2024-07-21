#include <ncurses.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

#define ERR_QUIT(msg, ...) \
    { fprintf(stderr, "ERROR: " msg "\n" __VA_OPT__(,) ##__VA_ARGS__); \
      endwin(); \
      exit(-1); }

#define CHECK_ALLOC(ptr) \
    { if ((ptr) == NULL) \
        { ERR_QUIT("null pointer: most likely ran out of memory - file %s line %zu", __FILE__, __LINE__); } }

int _dirent_name_cmp(const void*, const void*);
bool is_directory(char*);
char* get_working_dir_name_with_slash(size_t* len_out);
char* concat_directory(char* original_part, size_t original_size, char* to_concat, bool free_original, size_t* new_len_out);

char* current_directory;
size_t current_directory_len;

int main(int argc, char** argv)
{
    size_t working_directory_len;
    char* working_directory = get_working_dir_name_with_slash(&working_directory_len);
    assert(working_directory != NULL && "go into a valid directory and/or dont run out of memory");
    
    current_directory = concat_directory(working_directory, working_directory_len, ".", true, &current_directory_len);    
    current_directory_len = strlen(current_directory);

    initscr();
    keypad(stdscr, true);
    noecho();

    for (;;) {
        size_t cursorline = 0; // well make the lines start at 0 itll make things work better trust me take my hand
        DIR* directory = opendir(current_directory);
        if (directory == NULL) ERR_QUIT("directory not found: %s", current_directory);

        struct {
            size_t len, capacity;
            struct dirent** ptr;
        } files = { .len = 0, .capacity = 0, .ptr = NULL};

        struct dirent* dir_entry;
        while ((dir_entry = readdir(directory)) != NULL) {
            if (files.len >= files.capacity) {
                files.capacity = files.capacity == 0 ? 1 : files.capacity * 2;
                files.ptr = realloc(files.ptr, files.capacity * sizeof(struct dirent*));
                CHECK_ALLOC(files.ptr);
            }
            files.ptr[files.len++] = dir_entry;
        }

        qsort(files.ptr, files.len, sizeof(struct dirent*), _dirent_name_cmp);

        for (;;) {
            wmove(stdscr, 0, 0);
            for (size_t i = 0; i < files.len; i++) {
                if (i == cursorline)
                    attron(A_STANDOUT);

                if (is_directory(files.ptr[i]->d_name))
                    attron(A_UNDERLINE);

                printw("%s\n", files.ptr[i]->d_name);
                attroff(A_UNDERLINE | A_STANDOUT);
            }
            
            
            int ch;
            ch = getch();
            
            switch (ch) {
                case KEY_UP: {
                    if (cursorline > 0) cursorline--;
                } break;

                case KEY_DOWN: {
                    if (cursorline < files.len - 1) cursorline++;
                } break;

                case '\n' : case KEY_ENTER: {
                    if (is_directory(files.ptr[cursorline]->d_name) 
                        && strcmp(".", files.ptr[cursorline]->d_name) != 0) {

                        current_directory = concat_directory(current_directory, current_directory_len, files.ptr[cursorline]->d_name, true, &current_directory_len);
                        
                        goto change_directory;
                    }
                } break;
                
                case 'q' : {
                    // quit the program :)
                    endwin();
                    free(files.ptr);
                    free(current_directory);
                    (void) closedir(directory);
                    
                    exit(0);
                } break;
            }
        }
        change_directory:

        clear();
        free(files.ptr);
        (void) closedir(directory);
        refresh();
    }
    
    free(current_directory);
    endwin();
    return 0;
}

bool is_directory(char* filename) 
{
    char* filepath = concat_directory(current_directory, current_directory_len, filename, false, NULL);
    struct stat buf;
    errno = 0;
    if (stat(filepath, &buf) != 0) ERR_QUIT("stat-ing filepath %s gave error %s", filepath, strerror(errno));
    free(filepath);
    return __S_ISTYPE(buf.st_mode, __S_IFDIR);
}

int _dirent_name_cmp(const void* d1, const void* d2) 
{
    char* s1 = (*(struct dirent**)d1)->d_name;
    char* s2 = (*(struct dirent**)d2)->d_name;

    if (*s1 == '.') s1++;
    if (*s2 == '.') s2++;

    return strcmp(s1, s2);
}

char* get_working_dir_name_with_slash(size_t* len_out) 
{
    char pwd[FILENAME_MAX];
    if (getcwd(pwd, FILENAME_MAX) == NULL) return NULL;
    size_t len = strlen(pwd);
    char* ret = malloc(sizeof(char) * (len + 2));
    if (ret == NULL) return NULL;
    memcpy(ret, pwd, len);
    ret[len++] = '/';
    ret[len] = '\0';
    if (len_out != NULL) *len_out = len;
    return ret;
}

char* concat_directory(char* original_part, size_t original_size, char* to_concat, bool free_original, size_t* new_len_out)
{
    /*
     * this function basically makes a new string with a format of this:
     * "<original part>/<to_concat>".
     * I do this a lot in this function so
     **/
    size_t to_concat_len = strlen(to_concat);
    char* temp = malloc((original_size + to_concat_len + 2) * sizeof(char));
    CHECK_ALLOC(temp);
    sprintf(temp, "%s/%s", original_part, to_concat);
    if (free_original) free(original_part);
    if (new_len_out != NULL) *new_len_out = original_size + to_concat_len + 1;

    return temp;
}