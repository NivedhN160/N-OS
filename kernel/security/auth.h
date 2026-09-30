#ifndef AUTH_H
#define AUTH_H

#define MAX_USERS 4
#define MAX_ULEN  16
#define MAX_PLEN  16

typedef struct {
    char username[MAX_ULEN];
    char password[MAX_PLEN];
    int active;
} User;

extern User users[MAX_USERS];
extern int logged_in;
extern char current_user[MAX_ULEN];

extern int users_count;

void auth_init();
void auth_add_user(const char *user, const char *pass);
int auth_login(const char *user, const char *pass);
void auth_load_users();
void auth_save_users();

#endif
