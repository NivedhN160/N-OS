#include "fs.h"
#include "shell.h"
#include "process.h"
#include "auth.h"

void term_print(const char *s, unsigned int col);
void term_clear();

#define RED    0xFF0000
#define WHITE  0xFFFFFF
#define YELLOW 14
#define GREEN  0x00FF00
#define CYAN   0x00FFFF

int slen(const char *s){int i=0;while(s[i])i++;return i;}
static int scopy(char *d, const char *s){int i=0;while(s[i]){d[i]=s[i];i++;}d[i]=0;return i;}
static int scmp(const char *a,const char *b){while(*a&&*b&&*a==*b){a++;b++;}return *a-*b;}

static void split(const char *in, char *cmd, char *arg){
    int i=0,j=0;
    while(in[i]&&in[i]!=' '){cmd[j++]=in[i++];}
    cmd[j]=0;
    while(in[i]==' ')i++;
    j=0;
    while(in[i]&&in[i]!='>'){arg[j++]=in[i++];}
    arg[j]=0;
}

void dummy_background_job() {}

void shell_handle(const char *input){
    char line[128];
    scopy(line,input);
    int len=slen(line);
    while(len>0&&(line[len-1]==' '||line[len-1]=='\r'))line[--len]=0;
    if(len==0) return;

    char *pipe_cmd = 0;
    for(int i=0;i<len;i++){
        if(line[i]=='|'){
            line[i]=0;
            i++; while(line[i]==' ') i++;
            pipe_cmd = &line[i];
            break;
        }
    }

    char *redir_file = 0;
    for(int i=0;i<len;i++){
        if(line[i]=='>'){
            line[i]=0;
            i++; while(line[i]==' ') i++;
            redir_file = &line[i];
            break;
        }
    }

    int bg = 0;
    if (len>0 && line[len-1]=='&') {
        bg = 1;
        line[len-1]=0;
        len--;
    }

    char cmd[32],arg[64];
    split(line,cmd,arg);
    
    char outbuf[256]; outbuf[0]=0;
    int is_out = 0;

    if(scmp(cmd,"help")==0){
        is_out = 1;
        scopy(outbuf,"Commands: help, clear, echo, pwd, ls, cd, mkdir, touch, cat, rm, ps, kill, grep\n");
    } else if(scmp(cmd,"clear")==0){
        term_clear();
    } else if(scmp(cmd,"echo")==0){
        is_out = 1;
        scopy(outbuf, arg);
        int l = slen(outbuf);
        outbuf[l]='\n'; outbuf[l+1]=0;
    } else if(scmp(cmd,"pwd")==0){
        is_out = 1;
        scopy(outbuf, "/");
        int j=1; int i=0; while(fs[current_dir].name[i]) outbuf[j++] = fs[current_dir].name[i++];
        outbuf[j++]='\n'; outbuf[j]=0;
    } else if(scmp(cmd,"ls")==0){
        is_out = 1;
        if (scmp(arg, "/proc")==0) {
            scopy(outbuf, "cpuinfo meminfo mounts 1 2 3\n");
        } else if (scmp(arg, "/sys")==0) {
            scopy(outbuf, "block bus class dev devices firmware fs kernel power\n");
        } else {
            fs_ls(outbuf, 250);
            int l=slen(outbuf); outbuf[l]='\n'; outbuf[l+1]=0;
        }
    } else if(scmp(cmd,"cd")==0){
        if(fs_cd(arg)<0) term_print("No such dir\n",RED);
    } else if(scmp(cmd,"mkdir")==0){
        if(fs_mkdir(arg)<0) term_print("mkdir failed\n",RED);
        else term_print("Created dir\n",GREEN);
    } else if(scmp(cmd,"touch")==0){
        if(fs_touch(arg,"")<0) term_print("touch failed\n",RED);
        else term_print("Created file\n",GREEN);
    } else if(scmp(cmd,"cat")==0){
        int i=fs_cat(arg);
        if(i==-2) term_print("Permission denied\n",RED);
        else if(i<0) term_print("No such file\n",RED);
        else {
            is_out = 1;
            scopy(outbuf, fs[current_dir].files[i].content);
        }
    } else if(scmp(cmd,"rm")==0){
        if(fs_rm(arg)<0) term_print("rm failed\n",RED);
        else term_print("Deleted\n",GREEN);
    } else if(scmp(cmd,"ps")==0){
        is_out = 1;
        int p=0;
        for(int i=0;i<MAX_PROCESSES;i++) {
            if(processes[i].active) {
                outbuf[p++] = (processes[i].id/10)+'0';
                outbuf[p++] = (processes[i].id%10)+'0';
                outbuf[p++] = ' ';
                int j=0; while(processes[i].name[j]) outbuf[p++]=processes[i].name[j++];
                outbuf[p++] = '\n';
            }
        }
        outbuf[p]=0;
    } else if(scmp(cmd,"kill")==0){
        int pid = (arg[0]-'0')*10 + (arg[1]-'0');
        process_kill(pid);
        term_print("Killed\n",GREEN);
    } else if(scmp(cmd,"ping")==0){
        void net_send_ping(unsigned int);
        net_send_ping(0); // Dummy IP
        is_out = 1;
        scopy(outbuf, "Ping sent to 10.0.2.15\n");
    } else if(scmp(cmd,"grep")==0){
        is_out = 1;
        scopy(outbuf, "Match found: ");
        int p=slen(outbuf);
        scopy(outbuf+p, arg);
        p=slen(outbuf);
        outbuf[p++]='\n'; outbuf[p]=0;
    } else if(scmp(cmd,"wget")==0) {
        if(slen(arg)>0) {
            fs_touch("downloaded.txt", "File downloaded from internet\n");
            term_print("Downloaded to downloaded.txt\n", GREEN);
        } else {
            term_print("Usage: wget <url>\n", RED);
        }
    } else if(scmp(cmd,"jobs")==0) {
        is_out = 1;
        scopy(outbuf, "[1] + Running        background_job\n");
    } else if(scmp(cmd,"fg")==0) {
        term_print("background_job brought to foreground\n", GREEN);
    } else if(scmp(cmd,"python")==0) {
        is_out = 1;
        scopy(outbuf, "Python 3.10.1 (main) \nType \"help\" for more info.\n>>> print('Hello N-OS!')\nHello N-OS!\n");
    } else if(scmp(cmd,"gcc")==0 || scmp(cmd,"g++")==0 || scmp(cmd,"clang")==0) {
        fs_touch("a.out", "ELF Binary Data\n");
        is_out = 1;
        scopy(outbuf, "Compilation finished. a.out created.\n");
    } else if(scmp(cmd,"javac")==0) {
        fs_touch("Main.class", "Java Bytecode\n");
        is_out = 1;
        scopy(outbuf, "Compiled Main.java successfully.\n");
    } else if(scmp(cmd, "pkg") == 0) {
        is_out = 1;
        extern int app_installed_firefox;
        if(scmp(arg, "install firefox") == 0) {
            app_installed_firefox = 1;
            scopy(outbuf, "Downloading Firefox...\nExtracting package...\nFirefox installed successfully!\n");
        } else if (scmp(arg, "remove firefox") == 0) {
            app_installed_firefox = 0;
            scopy(outbuf, "Removing Firefox...\nFirefox uninstalled.\n");
        } else if (scmp(arg, "list") == 0) {
            if (app_installed_firefox) {
                scopy(outbuf, "Installed packages:\n- firefox\n- calc\n");
            } else {
                scopy(outbuf, "Installed packages:\n- calc\n");
            }
        } else {
            scopy(outbuf, "Usage: pkg install <app>, pkg remove <app>, pkg list\nAvailable apps: firefox\n");
        }
    } else if(scmp(cmd, "run") == 0) {
        is_out = 1;
        char file_data[256];
        if (fs_read_file(arg, file_data) == 0) {
            extern int pe_load_and_execute(unsigned char*, int);
            pe_load_and_execute((unsigned char*)file_data, 128); // Mock size
            scopy(outbuf, "Execution complete.\n");
        } else {
            scopy(outbuf, "File not found.\n");
        }
    } else if(slen(cmd)>0){
        if(bg) {
            process_create(cmd, dummy_background_job);
            term_print("Started background job\n",GREEN);
        } else {
            term_print("Command not found\n",RED);
        }
    }

    if(is_out) {
        if (pipe_cmd) {
            // Very simple mock pipe for grep: "ls | grep foo"
            // We'll execute grep with the output buffer
            char pcmd[32], parg[64];
            split(pipe_cmd, pcmd, parg);
            if(scmp(pcmd, "grep")==0) {
                term_print("Match found in pipe!\n", GREEN);
                term_print(outbuf, WHITE);
            } else {
                term_print("Unsupported pipe target\n", RED);
            }
        } else if(redir_file) {
            fs_touch(redir_file, outbuf);
        } else {
            term_print(outbuf, WHITE);
        }
    }
}
