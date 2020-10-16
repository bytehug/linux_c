#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ERROR_NONE  (0)
#define ERROR_FAIL  (-1)
#define MAX_LEN     (256)
#define MAX_PATH    (1024)
#define CNT_INIT    (0)

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
    char fpath[MAX_LEN + 1];

    memset(fpath, 0, sizeof(fpath));
    snprintf(fpath, MAX_LEN, "/proc/%s/fd", pid);
    fd_dir = opendir(fpath);
    if (fd_dir == NULL) {
        printf("open fd_dir: %s, fail\n", fpath);
        return ERROR_FAIL;
    }

    fd_cnt = CNT_INIT;
    while ((fd_ptr = readdir(fd_dir))) {
        if (strcmp(fd_ptr->d_name, ".") == 0 || strcmp(fd_ptr->d_name, "..") == 0) {
            continue;
        }

        fd_cnt++;
    }

    closedir(fd_dir);

    return fd_cnt;
}

static void fnode_init(fnode_t *node)
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
    char buff[MAX_PATH + 1], fpath[MAX_LEN + 1], input[MAX_LEN + 1], cmd[MAX_PATH + 1];
    char *cmd_start, *cmd_end, *rss;
    fnode_t head, *tail, *tmp, *sort_out;

    printf("please input -Om(sort by memory)/-Of(sort by fd):");
    fgets(input, MAX_LEN, stdin);
    input[strcspn(input, "\n")] = '\0';    /* 安全输入fgets会将回车符读入，故将最后一个字符转为\0 */

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
    /* 遍历目录/proc */
    while ((fptr = readdir(dir)) != NULL) {
        if (fptr->d_name[0] < '0' || fptr->d_name[0] > '9') {
            continue;
        }

        memset(fpath, 0, sizeof(fpath));
        snprintf(fpath, MAX_LEN, "/proc/%s/smaps", fptr->d_name);

        fp = fopen(fpath, "r");
        if (fp == NULL) {
            printf("open smaps fail\n");
            continue;
        }

        memset(buff, 0, sizeof(buff));
        if (fread(buff, 1, MAX_LEN, fp) == 0) {
            fclose(fp);
            continue;
        }

        /* 创建资源信息链表，输出结果时再一个个free */
        tmp = (fnode_t *) malloc (sizeof(fnode_t));
        if (tmp == NULL) {
            printf("no memory!\n");
            fclose(fp);
            continue;
        }

        (void)fnode_init(tmp);
        memset(cmd, 0, sizeof(cmd));
        if ((cmd_start = strchr(buff, '/')) != NULL) {  /* 指针操作不嵌套防止传入空指针 */
            if ((cmd_end = strchr(cmd_start, '\n')) != NULL && (cmd_end - cmd_start) < MAX_PATH) {
                memcpy(cmd, cmd_start, (int) (cmd_end - cmd_start));
            } else {
                strcpy(cmd, "unknown");
            }
        } else {
            strcpy(cmd, "unknown");
        }

        memset(tmp->name, 0, sizeof(tmp->name));
        (void)get_short_name(tmp->name, cmd);
        rss = strstr(buff, "Rss:");
        if (rss != NULL) {
            sscanf(rss, "Rss:%*[^0-9]%5d kB" , &tmp->rss);
        } else {
            tmp->rss = ERROR_FAIL;  /* 解析rss失败设为-1 */
        }

        tmp->fd = count_fd(fptr->d_name);
        if (tmp->fd == ERROR_FAIL) {
            printf("count fd of %s fail\n", tmp->name); /* 继续运行，只提示不返回 */
        }

        tail->next = tmp;
        tmp->pre = tail;
        tail = tmp;
        fclose(fp);
    }   /* END OF WHILE, 资源信息结点链表创建结束 */
    closedir(dir);

    /* 按序输出 */
    while (head.next != NULL) {
        tmp = head.next;
        sort_out = tmp;
        while (tmp != NULL) {
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
        sort_out = NULL;
    }

    return;
}

static void proc_detect(void)
{
    char input[MAX_LEN + 1], buff[MAX_PATH + 1];
    char fpath[MAX_LEN + 1], fname[MAX_LEN + 1], fd_path[MAX_LEN + 1], fd[MAX_LEN + 1];
    char cmd[MAX_PATH + 1], *cmd_start, *cmd_end, *rss_ptr;
    int32_t rss, fd_cnt, link_cnt, reg_cnt, dev_cnt, unknow_cnt, ret;
    struct stat st;

    printf("please input name of proc:");
    fgets(input, MAX_LEN, stdin);
    input[strcspn(input, "\n")] = '\0';    /* fgets会将回车符读入，故将最后一个字符转为\0 */

    dir = opendir("/proc");
    if (dir == NULL) {
        printf("open dir: /proc ,fail\n");
        return;
    }

    /* 遍历目录找到对应名字的进程 */
    while ((fptr = readdir(dir)) != NULL) {
        if (fptr->d_name[0] < '0' || fptr->d_name[0] > '9') {
            continue;
        }

        memset(fpath, 0, sizeof(fpath));
        snprintf(fpath, MAX_LEN, "/proc/%s/smaps", fptr->d_name);
        fp = fopen(fpath, "r");
        if (fp == NULL) {
            printf("open smaps %s fail\n", fpath);
            continue;
        }

        memset(buff, 0, sizeof(buff));
        if (!fread(buff, 1, MAX_LEN, fp)) {
            fclose(fp);
            continue;
        }

        /* 解析smaps中的命令行，截取最后部分的进程名,不存在则识别为unknown */
        memset(cmd, 0, sizeof(cmd));
        memset(fname, 0, sizeof(fname));
        if ((cmd_start = strchr(buff, '/')) != NULL) {
            if ((cmd_end = strchr(cmd_start, '\n')) != NULL && (cmd_end - cmd_start) < MAX_PATH) {
                memcpy(cmd, cmd_start, (int) (cmd_end - cmd_start));
            } else {
                strcpy(cmd, "unknown");
            }
        } else {
            strcpy(cmd, "unknown");
        }
        (void)get_short_name(fname, cmd);

        if (strcmp(fname, input)) {
            fclose(fp);
            continue;
        }
        fd_cnt = count_fd(fptr->d_name);
        rss_ptr = strstr(buff, "Rss:");
        if (rss_ptr != NULL) {
            sscanf(rss_ptr, "Rss:%*[^0-9]%5d kB" , &rss);
        } else {
            rss = ERROR_FAIL;
        }
        printf("Name:%-15sRss:%-8dPid:%-8sFd:%-8dCmd:%s\n", \
            fname, rss, fptr->d_name, fd_cnt, cmd);

        /* 句柄分类 */
        link_cnt = CNT_INIT;
        reg_cnt = CNT_INIT;
        dev_cnt = CNT_INIT;
        unknow_cnt = CNT_INIT;
        memset(fd_path, 0, sizeof(fd_path));
        snprintf(fd_path, MAX_LEN, "/proc/%s/fd", fptr->d_name);
        fd_dir = opendir(fd_path);
        if (fd_dir == NULL) {
            printf("open fd_dir: %s, fail\n", fd_path);
            goto fd_print;
        }

        while ((fd_ptr = readdir(fd_dir)) != NULL) {
            if (strcmp(fd_ptr->d_name, ".") == 0 || strcmp(fd_ptr->d_name, "..") == 0) {
                continue;
            }

            memset(fd, 0, sizeof(fd));
            snprintf(fd, MAX_LEN, "/proc/%s/fd/%s", fptr->d_name, fd_ptr->d_name);
            ret = lstat(fd, &st);
            if (ret != ERROR_NONE) {
                printf("get fd stat fail %s\n", fd);
                unknow_cnt++;
                continue;
            }

            if ((st.st_mode & S_IFMT) == S_IFREG) {
                reg_cnt++;
                continue;
            } else if ((st.st_mode & S_IFMT) == S_IFLNK) {
                link_cnt++;
                continue;
            } else if ((st.st_mode & S_IFMT) == S_IFBLK) {
                dev_cnt++;
                continue;
            } else {
                unknow_cnt++;
                continue;
            }
        }   /* END OF WHILE */

        closedir(fd_dir);
        printf("fds:\n     reg:%-10dlink:%-10ddev:%-10dunknow:%-10d\n", reg_cnt, link_cnt, \
            dev_cnt, unknow_cnt);
fd_print:
        fclose(fp);
        break;
    }   /* END OF WHILE, 查找对应进程的信息并完成输出结束 */

    closedir(dir);

    return;
}

int32_t main(void)
{
    char input[MAX_LEN];

    printf("please input your choice(sort/proc/quit):");
    while (scanf("%s", input) != EOF && input[0] != '\n') {
        getchar();
        if (!strcmp(input, "quit")) {
            break;
        } else if (!strcmp(input, "sort")) {
            (void)sort_presource();
        } else if (!strcmp(input, "proc")) {
            (void)proc_detect();
        }

        usleep(100000);
        printf("please input your choice(sort/proc/quit):");
    }

    return ERROR_NONE;
}
