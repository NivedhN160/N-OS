#include "fs.h"
#include "auth.h"
#include "process.h"

Directory fs[MAX_DIRS];
int dir_count = 0;
int current_dir = 0;

static int slen(const char *s){ int i=0; while(s[i])i++; return i; }
static void scopy(char *d, const char *s){ int i=0; while(s[i]){d[i]=s[i];i++;} d[i]=0; }
static int scmp(const char *a, const char *b){
    while(*a&&*b&&*a==*b){a++;b++;} return *a-*b;
}

void fs_init(){
    scopy(fs[0].name, "root");
    fs[0].used=1; fs[0].file_count=0; dir_count=1; current_dir=0;

    fs_mkdir("home"); fs_mkdir("bin"); fs_mkdir("etc"); fs_mkdir("proc"); fs_mkdir("tmp");

    fs_cd("etc");
    fs_touch("version.txt", "N-OS v1.2\n");
    fs_cd("proc");
    fs_touch("cpuinfo", "CPU: x86\n");
    fs[current_dir].files[0].is_proc = 1;
    fs_touch("meminfo", "Mem: 640KB\n");
    fs[current_dir].files[1].is_proc = 1;
    fs_cd("root");
}

int fs_mkdir(const char *name){
    if(dir_count>=MAX_DIRS) return -1;
    for(int i=0;i<dir_count;i++)
        if(fs[i].used && scmp(fs[i].name,name)==0) return -1;
    scopy(fs[dir_count].name, name);
    fs[dir_count].used=1; fs[dir_count].file_count=0; dir_count++;
    return 0;
}

int fs_find_dir(const char *name){
    for(int i=0;i<dir_count;i++)
        if(fs[i].used && scmp(fs[i].name,name)==0) return i;
    return -1;
}

int fs_cd(const char *name){
    if(scmp(name,"..")==0){ current_dir=0; return 0; }
    int i=fs_find_dir(name);
    if(i<0) return -1;
    current_dir=i;
    return 0;
}

int fs_touch(const char *name, const char *content){
    Directory *d=&fs[current_dir];
    int idx=fs_find_file(name);
    if (idx >= 0) {
        if (content) scopy(d->files[idx].content, content);
        return 0;
    }
    if(d->file_count>=MAX_FILES) return -1;
    idx=d->file_count;
    scopy(d->files[idx].name, name);
    if(content) scopy(d->files[idx].content, content);
    else d->files[idx].content[0]=0;
    scopy(d->files[idx].owner, current_user);
    d->files[idx].perms = 7;
    d->files[idx].is_proc = 0;
    d->files[idx].used=1;
    d->file_count++;
    return 0;
}

int fs_find_file(const char *name){
    Directory *d=&fs[current_dir];
    for(int i=0;i<d->file_count;i++)
        if(d->files[i].used && scmp(d->files[i].name,name)==0) return i;
    return -1;
}

int fs_cat(const char *name){
    int i=fs_find_file(name);
    if(i<0) return -1;
    
    File *f = &fs[current_dir].files[i];
    if (scmp(f->owner, current_user)!=0 && scmp("admin", current_user)!=0 && (f->perms & 4) == 0) {
        return -2; // Permission denied
    }

    if (f->is_proc) {
        if (scmp(name, "cpuinfo")==0) scopy(f->content, "processor: 0\nvendor_id: GenuineIntel\n");
        else if (scmp(name, "meminfo")==0) scopy(f->content, "MemTotal: 640 kB\nMemFree: 256 kB\n");
    }
    return i;
}

int fs_rm(const char *name){
    int i=fs_find_file(name);
    if(i<0) return -1;
    File *f = &fs[current_dir].files[i];
    if (scmp(f->owner, current_user)!=0 && scmp("admin", current_user)!=0 && (f->perms & 2) == 0) return -2;

    fs[current_dir].files[i].used=0;
    Directory *d=&fs[current_dir];
    for(int j=i;j<d->file_count-1;j++) d->files[j]=d->files[j+1];
    d->file_count--;
    return 0;
}

void fs_ls(char *out, int maxlen){
    Directory *d=&fs[current_dir];
    int pos=0;
    for(int i=0;i<dir_count;i++){
        if(i==current_dir||!fs[i].used) continue;
        const char *n=fs[i].name; int j=0;
        while(n[j]&&pos<maxlen-3) out[pos++]=n[j++];
        out[pos++]='/'; out[pos++]=' ';
    }
    for(int i=0;i<d->file_count;i++){
        if(!d->files[i].used) continue;
        const char *n=d->files[i].name; int j=0;
        while(n[j]&&pos<maxlen-2) out[pos++]=n[j++];
        out[pos++]=' ';
    }
    if(pos==0){out[pos++]='-';}
    out[pos]=0;
}



void fs_write_file(const char *path, const char *content) {
    int old_dir = current_dir;
    current_dir = 0; 
    const char *name = path;
    if (path[0] == '/') {
        if (scmp(path, "/sys/users.dat") == 0) name = "users.dat";
    }
    fs_touch(name, content);
    current_dir = old_dir;
}

int fs_read_file(const char *path, char *out_content) {
    int old_dir = current_dir;
    current_dir = 0;
    const char *name = path;
    if (path[0] == '/') {
        if (scmp(path, "/sys/users.dat") == 0) name = "users.dat";
    }
    int f = fs_find_file(name);
    if (f >= 0) {
        scopy(out_content, fs[0].files[f].content);
        current_dir = old_dir;
        return 0; // Success
    }
    current_dir = old_dir;
    return -1; // Not found
}
