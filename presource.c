#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define ERROR_NONE (0)
#define ERROR_FAIL (-1)
#define MAX_LEN    (256)
#define MAX_PATH   (1024)

typedef struct fnode {
    char name[MAX_LEN];
    int32_t rss;
    int32_t fd;
    struct fnode *next;
    struct fnode *pre;
} fnode_t;

DIR *dir, *fd_dir;
FILE *fp;
struct dirent *fptr, *fd_ptr;

static int32_t get_short_name(char *fname, char *fpath)
{
    char *c;

    if (fpath == NULL) {
        printf("wrong fpath arg\n");
        return ERROR_FAIL;
    }
    strcpy(fname, (c = strrchr(fpath, '/')) ? (c + 1) : fpath);

    return ERROR_NONE;
}

static int32_t count_fd(char *pid)
{
    int32_t fd_cnt;
    char fpath[MAX_LEN];

    memset(fpath, 0, sizeof(fpath));
    sprintf(fpath, "/proc/%s/fd", pid);

    fd_dir = opendir(fpath);
    if (fd_dir == NULL) {
        printf("open fd_dir: %s, fail\n", fpath);
        return ERROR_FAIL;
    }

    fd_cnt = 0;
    while ((fd_ptr = readdir(fd_dir))) {
        fd_cnt++;
    }
    closedir(fd_dir);

    return fd_cnt;
}

void fnode_init(fnode_t *node)
{
    memset(node->name, 0, sizeof(node->name));
    node->fd = 0;
    node->rss = 0;
    node->next = NULL;
    node->pre = NULL;
    return;
}

static void sort_presource(void)
{
    char buff[MAX_PATH], fpath[MAX_LEN], input[MAX_LEN], cmd[MAX_PATH];
    fnode_t head, *tail, *tmp, *sort_out;

    printf("please input -Om(sort by memory)/-Of(sort by fd):");
    scanf("%s", input);

    if (strcmp(input, "-Om") && strcmp(input, "-Of")) {
        printf("wrong option\n");
        return;
    }

    dir = opendir("/proc");
    if (dir == NULL) {
        printf("open dir: /proc ,fail\n");
        return;
    }

    tail = &head;
    fnode_init(&head);
    /* 创建资源信息链表 */
    while((fptr = readdir(dir)) != NULL) {
        if (fptr->d_name[0] < '0' || fptr->d_name[0] > '9') {
            continue;
        }
        memset(fpath, 0, sizeof(fpath));
        sprintf(fpath, "/proc/%s/smaps", fptr->d_name);

        fp = fopen(fpath, "r");
        memset(buff, 0, sizeof(buff));
        if(fread(buff, 1, MAX_LEN, fp) == 0) {
            printf("kernel process\n");
            break;
        }

        /* 输出结果时再一个个free */
        tmp = (fnode_t *)malloc(sizeof(fnode_t));
        if (tmp == NULL) {
            printf("no memory!\n");
            continue;
        }
        fnode_init(tmp);
        memset(cmd, 0, sizeof(cmd));
        memcpy(cmd, strchr(buff, '/'), strchr(strchr(buff, '/'), '\n')-strchr(buff, '/'));
        memset(tmp->name, 0, sizeof(tmp->name));
        get_short_name(tmp->name, cmd);
        sscanf(strstr(buff, "Rss:"), "Rss:%*[^0-9]%d kB" , &tmp->rss);

        tmp->fd = count_fd(fptr->d_name);
        if (tmp->fd == ERROR_FAIL) {
            printf("count fd of %s fail\n", tmp->name); /* 为了继续运行，只提示不返回 */
        }

        tail->next = tmp;
        tmp->pre = tail;
        tail = tmp;
        fclose(fp);
    }   /* 资源信息结点链表创建结束 */
    closedir(dir);

    /* 按序输出 */
    while(head.next != NULL) {
        tmp = head.next;
        sort_out = tmp;
        while(tmp != NULL) {
            if (!strcmp(input, "-Om")) {
                if (tmp->rss > sort_out->rss) {
                    sort_out = tmp;
                }
            } else if (!strcmp(input, "-Of")) {
                if (tmp->fd > sort_out->fd) {
                    sort_out = tmp;
                }
            }
            tmp = tmp->next;
        }
        printf("Name:%-20sRss:%-10dfd:%-10d\n", sort_out->name, sort_out->rss, sort_out->fd);
        if (sort_out->next != NULL) {
            sort_out->next->pre = sort_out->pre;
            sort_out->pre->next = sort_out->next;
        } else {
            sort_out->pre->next = NULL;
        }
        free(sort_out);
    }

    return;
}

static void proc_detect(void)
{
    char input[MAX_LEN];
    char fpath[MAX_LEN];
    char buff[MAX_PATH];
    char cmd[MAX_PATH];
    char fname[MAX_LEN];
    int rss, fd_cnt;

    printf("please input name of proc:");
    scanf("%s", input);

    dir = opendir("/proc");
    if (dir == NULL) {
        printf("open dir: /proc ,fail\n");
        return;
    }

    while((fptr = readdir(dir)) != NULL) {
        if (fptr->d_name[0] < '0' || fptr->d_name[0] > '9') {
            continue;
        }

        memset(fpath, 0, sizeof(fpath));
        strcat(fpath, "/proc/");
        strcat(fpath, fptr->d_name);
        strcat(fpath, "/smaps");

        fp = fopen(fpath, "r");
        memset(buff, 0, sizeof(buff));
        if(fread(buff, 1, MAX_LEN, fp) == 0) {
            printf("kernel process\n");
            break;
        }

        memset(cmd, 0, sizeof(cmd));
        memset(fname, 0, sizeof(fname));
        memcpy(cmd, strchr(buff, '/'), strchr(strchr(buff, '/'), '\n')-strchr(buff, '/'));
        get_short_name(fname, cmd);

        if (strcmp(fname, input)) {
            fclose(fp);
            continue;
        }
        fd_cnt = count_fd(fptr->d_name);
        sscanf(strstr(buff, "Rss:"), "Rss:%*[^0-9]%d kB" , &rss);
        printf("Name:%-15sRss:%-8dPid:%-8sFd:%-8dCmd:%s\n", \
            fname, rss, fptr->d_name, fd_cnt, cmd);

        fclose(fp);
        break;
    }   /* 查找对应进程的信息并完成输出 */
    closedir(dir);

    return;
}

int32_t main(void)
{
    char input[MAX_LEN];

    printf("please input your choice(sort/proc/quit):");
    while (scanf("%s", input) != EOF && input[0] != '\n') {
        fflush(stdin);
        if (!strcmp(input, "quit")) {
            break;
        } else if (!strcmp(input, "sort")) {
            sort_presource();
        } else if (!strcmp(input, "proc")) {
            proc_detect();
        }
        usleep(100000);
        printf("please input your choice(sort/proc/quit):");
    }

    return ERROR_NONE;
}
