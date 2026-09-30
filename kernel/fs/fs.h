#ifndef FS_H
#define FS_H

#define MAX_FILES     32
#define MAX_DIRS      16
#define MAX_NAME      32
#define MAX_CONTENT  256

typedef struct {
    char name[MAX_NAME];
    char content[MAX_CONTENT];
    int used;
    char owner[16];
    int perms; // rwx
    int is_proc; // 1 if virtual proc file
} File;

typedef struct {
    char name[MAX_NAME];
    File files[MAX_FILES];
    int file_count;
    int used;
} Directory;

extern Directory fs[MAX_DIRS];

void fs_write_file(const char *path, const char *content);
int fs_read_file(const char *path, char *out_content);
extern int dir_count;
extern int current_dir;

void fs_init();
int fs_mkdir(const char *name);
int fs_touch(const char *name, const char *content);
int fs_find_file(const char *name);
int fs_find_dir(const char *name);
void fs_ls(char *out, int maxlen);
int fs_cat(const char *name);
int fs_rm(const char *name);
int fs_cd(const char *name);
void fs_pwd();
void fs_chmod(const char *name, int perms);

#endif
