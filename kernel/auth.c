#include "auth.h"
#include "fs.h"

User users[MAX_USERS];
int logged_in = 0;
char current_user[MAX_ULEN];

static int scmp(const char *a, const char *b){
    while(*a&&*b&&*a==*b){a++;b++;}return *a-*b;
}
static void scopy(char *d, const char *s){
    int i=0;while(s[i]){d[i]=s[i];i++;}d[i]=0;
}

int users_count = 0;

void auth_init(){
    logged_in=0;
    users_count=0;
    for(int i=0;i<MAX_USERS;i++) users[i].active = 0;
    auth_load_users();
}

void auth_add_user(const char *user, const char *pass) {
    if (users_count < MAX_USERS) {
        scopy(users[users_count].username, user);
        scopy(users[users_count].password, pass);
        users[users_count].active=1;
        users_count++;
        auth_save_users();
    }
}

void auth_save_users() {
    char buf[512];
    int pos = 0;
    for(int i=0; i<users_count; i++) {
        if(users[i].active) {
            int j=0; while(users[i].username[j]){buf[pos++]=users[i].username[j++];} buf[pos++]=':';
            j=0; while(users[i].password[j]){buf[pos++]=users[i].password[j++];} buf[pos++]='\n';
        }
    }
    buf[pos]=0;
    fs_write_file("/sys/users.dat", buf);
}

void auth_load_users() {
    char buf[512];
    if (fs_read_file("/sys/users.dat", buf) == 0) {
        int pos = 0;
        while(buf[pos]) {
            char u[32], p[32];
            int j=0; while(buf[pos] && buf[pos] != ':') u[j++] = buf[pos++]; u[j]=0;
            if (buf[pos] == ':') pos++;
            j=0; while(buf[pos] && buf[pos] != '\n') p[j++] = buf[pos++]; p[j]=0;
            if (buf[pos] == '\n') pos++;
            if (u[0] && p[0]) {
                scopy(users[users_count].username, u);
                scopy(users[users_count].password, p);
                users[users_count].active = 1;
                users_count++;
            }
        }
    }
}

int auth_login(const char *user, const char *pass){
    for(int i=0;i<MAX_USERS;i++){
        if(users[i].active &&
           scmp(users[i].username, user)==0 &&
           scmp(users[i].password, pass)==0){
            logged_in=1;
            scopy(current_user, user);
            return 1;
        }
    }
    return 0;
}
