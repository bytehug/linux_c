#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#define ERROR_NONE  (0)
#define ERROR_FAIL  (-1)

#define ARGC        (4)
#define MAX_LEN     (256)
#define MAX_PATH    (4096)
#define BUFF_SIZE   (1024)
#define D_SMALL     (0)         /* ��΢���� */
#define D_GREAT     (1)         /* ���ʲ��� */
#define D_NONE      (2)         /* �޶�Ӧ */

#define CHK_RET(ret, fmt, args...) do { \
    if (ret != 0) { \
        printf(fmt, ##args); \
        return ERROR_FAIL; \
    } \
} while (0)

typedef enum cmp_op {
    TREE = 0,   /* ȫ����� */
    DIFF = 1,   /* ������� */
    OMAX = 2,
} op_e;

char g_result[3][11] = {
    "-> D_SMALL",
    "-> D_GREAT",
    "-> D_NONE"
};

typedef struct file_node {
    char *name;
    int32_t result;
    struct file_node *pnode;
    struct file_node *son;
    struct file_node *bro;
} fnode_t;

fnode_t g_root;
char *g_dir2;
int32_t g_dir1_len;
int32_t g_dir2_len;

static int32_t fnode_init(fnode_t *node, char *name)
{
    if (name != NULL) {
        node->name = (char *)malloc(strlen(name) + 1);
        memset(node->name, 0, strlen(name) + 1);
        strcpy(node->name, name);
    }
    node->result = 0;
    node->pnode = NULL;
    node->son = NULL;
    node->bro = NULL;

    return ERROR_NONE;
}

static int32_t get_full_path(char *fpath, char *ldir, char *fname)
{
    int32_t ret;
    
    if (fpath == NULL || ldir == NULL || fname == NULL) {
        printf("wrong args to get full path\n");
        return ERROR_FAIL;
    }

    ret = sprintf(fpath , "%s/%s", ldir, fname);
    if (ret < 0) {
        printf("get full path fail, ldir=%s, fname=%s\n", ldir, fname);
        return ERROR_FAIL;
    }

    return ERROR_NONE;
}

static int32_t get_short_name(char *fname, char *fpath) {
    char *c;

    if (fpath == NULL) {
        printf("wrong fpath arg\n");
        return ERROR_FAIL;
    }
    strcpy(fname, (c = strrchr(fpath, '/')) ? (c + 1) : fpath);

    return ERROR_NONE;
}

static int32_t compare_files(fnode_t *fnode)        /* �ļ�������·�� */
{
    int32_t ret, rv;
    char *f2_path;
    struct stat st1, st2;
    FILE *f1, *f2;
    char buffer1[BUFF_SIZE+1], buffer2[BUFF_SIZE+1];
    char lnkbuff1[MAX_PATH+1], lnkbuff2[MAX_PATH+1];
    
    f2_path = (char *)malloc(g_dir2_len - g_dir1_len + strlen(fnode->name) + 1);
    sprintf(f2_path, "%s/%s", g_dir2, (fnode->name) + g_dir1_len + 1);

    memset(&st1, 0, sizeof(struct stat));
    memset(&st2, 0, sizeof(struct stat));
    ret = lstat(fnode->name, &st1);                 /* lstat�ܶ��������ļ��������� */
    if (ret != ERROR_NONE) {
        printf("get lstat fail, node name:%s\n", fnode->name);
        return ERROR_FAIL;
    }
    ret = lstat(f2_path, &st2);
    if (ret != ERROR_NONE) {
        /* �򲻿���Ϊ�޶�Ӧ */
        fnode->result = D_NONE;
        return ERROR_NONE;
    }
    /* ��߿��ǲ�ͬ���͵��ļ��Ƿ����ִ��� */
    if ((st1.st_mode & S_IFMT) == S_IFREG) {
        /* normal file�����ٶ��������Բ��죬Ҫ��Ƚ��������ԣ�����ʽ�ϱȽ��� */
        if ((st1.st_dev     != st2.st_dev) || \
            (st1.st_ino     != st2.st_ino) || \
            (st1.st_mode    != st2.st_mode) || \
            (st1.st_nlink   != st2.st_nlink) || \
            (st1.st_uid     != st2.st_uid) || \
            (st1.st_gid     != st2.st_gid) || \
            (st1.st_rdev    != st2.st_rdev) || \
            (st1.st_size    != st2.st_size) || \
            (st1.st_blocks  != st2.st_blocks) || \
            (st1.st_blksize != st2.st_blksize) || \
            (st1.st_atime   != st2.st_atime) || \
            (st1.st_ctime   != st2.st_ctime) || \
            (st1.st_mtime   != st2.st_mtime)) {
            fnode->result = D_SMALL;

            /* �ļ���С�в��������ݱ��в��죬ֱ�ӷ��� */
            if (st1.st_size != st2.st_size) {
                fnode->result = D_GREAT;
                return ERROR_NONE;
            } else {
                f1 = fopen(fnode->name, "rb");
                f2 = fopen(f2_path, "rb");
                if (f1 == NULL || f2 == NULL) {
                    printf("open file fail, f1:%s, f2:%s\n", fnode->name, f2_path);
                    fnode->result = D_GREAT;
                    return ERROR_FAIL;
                }
                memset(buffer1, 0, sizeof(buffer1));
                memset(buffer2, 0, sizeof(buffer2));
                while ((ret = fread(buffer1, 1, BUFF_SIZE, f1))) {
                    if(ret == fread(buffer2, 1, BUFF_SIZE, f2)) {
                        if(memcmp(buffer1, buffer2, ret)) {
                            fnode->result = D_GREAT;
                            rv = fclose(f1);
                            rv |= fclose(f2);
                            if (rv != 0) {
                                printf("close file fail\n");
                                return ERROR_FAIL;
                            }
                            return ERROR_NONE;
                        } else {
                            continue;
                        }
                    } else {
                        fnode->result = D_GREAT;
                        fclose(f1);
                        fclose(f2);
                        return ERROR_NONE;
                    }
                    
                }
                fclose(f1);
                fclose(f2);
            }
        }

    } else if ((st1.st_mode & S_IFMT) == S_IFLNK) {
        /* symbolic link */
        if ((st1.st_dev     != st2.st_dev) || \
            (st1.st_ino     != st2.st_ino) || \
            (st1.st_mode    != st2.st_mode) || \
            (st1.st_nlink   != st2.st_nlink) || \
            (st1.st_uid     != st2.st_uid) || \
            (st1.st_gid     != st2.st_gid) || \
            (st1.st_rdev    != st2.st_rdev) || \
            (st1.st_size    != st2.st_size) || \
            (st1.st_blocks  != st2.st_blocks) || \
            (st1.st_blksize != st2.st_blksize) || \
            (st1.st_atime   != st2.st_atime) || \
            (st1.st_ctime   != st2.st_ctime) || \
            (st1.st_mtime   != st2.st_mtime)) {
            fnode->result = D_SMALL;

            /* �ļ���С�в��������ݱ��в��죬ֱ�ӷ��� */
            if (st1.st_size != st2.st_size) {
                fnode->result = D_GREAT;
                return ERROR_NONE;
            } else {
                readlink(lnkbuff1, fnode->name, MAX_PATH);
                readlink(lnkbuff2, f2_path, MAX_PATH);
                if (memcmp(lnkbuff1, lnkbuff2, MAX_PATH)) {
                    fnode->result = D_GREAT;
                    return ERROR_NONE;
                }
            }
        }
    } else if ((st1.st_mode & S_IFMT) == S_IFDIR) {
        /* dir,�߲�������߼� */
    } else {
        /* unknown type, ignoring these files */
    }

    return ERROR_NONE;
}

static int32_t compare_ftree(char *ftree1, char *ftree2, fnode_t *lnode)
{
    DIR *dir1, *dir2;
    char *fname1, *fname2;
    struct dirent *ptr1, *ptr2;
    fnode_t *fnode, *pbro_tail;
    char *f2_path;
    int32_t lresult;

    /* ftree�Ǹ�Ŀ¼·����ͨ�������ļ�������sprintf�õ��ļ�·�� */
    dir1 = opendir(ftree1);
    dir2 = opendir(ftree2);
    if (dir1 == NULL) {
        printf("fail to opendir, dir1:%s\n", ftree1);
        return ERROR_FAIL;
    }
    /* �򲻿���Ϊ�޶�Ӧ */
    if (dir2 == NULL) {
        lnode->result = D_NONE;
        return ERROR_NONE;
    }

    /* ����·���µ������ļ� */
    pbro_tail = NULL;
    lresult = lnode->result;
    while ((ptr1 = readdir(dir1)) != NULL) {
        if (strcmp(ptr1->d_name, ".") == 0 || strcmp(ptr1->d_name, "..") == 0)
        {
            continue;
        }

        if (ptr1->d_type == DT_REG || ptr1->d_type == DT_LNK)
        {
            fnode = (fnode_t *)malloc(sizeof(fnode_t));
            fnode->name = (char *)malloc(sizeof(ptr1->d_name) + 1 + strlen(lnode->name));
            get_full_path(fnode->name, lnode->name, ptr1->d_name);
            fnode_init(fnode, NULL);
 
            fnode->pnode = lnode;
            fnode->son   = NULL;
            compare_files(fnode);
            /* �ݴ汾���ļ��Ա���������б��ʲ���������ϲ�Ŀ¼�ļ�ҲΪ���ʲ��� */
            if ((lresult != D_GREAT) && (fnode->result != D_SMALL)) {
                lresult = D_GREAT;
            }
            if (lnode->son == NULL) {
                lnode->son = fnode;
            } else {
                pbro_tail->bro = fnode;
            } 
            pbro_tail = fnode;  /* ����֮����ֵܽ������ */
        } else if (ptr1->d_type == DT_DIR) {
            fnode = (fnode_t *)malloc(sizeof(fnode_t));

            fnode->name = (char *)malloc(sizeof(ptr1->d_name) + 1 + strlen(lnode->name));
            get_full_path(fnode->name, lnode->name, ptr1->d_name);
            fnode_init(fnode, NULL);
            fnode->pnode = lnode;
            if (lnode->son == NULL) {
                lnode->son = fnode;
            } else {
                pbro_tail->bro = fnode;
            }
            pbro_tail = fnode;  /* �����ֵܽ������ */

            /* ftree2�����ֿ��ܺܳ���̣ܶ���fnode->name��һ������ô��,����ftree2�µĽ��·�� */
            f2_path = (char *)malloc(g_dir2_len - g_dir1_len + strlen(fnode->name)+1);
            sprintf(f2_path, "%s/%s", g_dir2, (fnode->name) + g_dir1_len + 1);

            compare_ftree(fnode->name, f2_path, fnode);
            if ((lresult != D_GREAT) && (fnode->result != D_SMALL)) {
                lresult = D_GREAT;
            }
            free(f2_path);

        } else {
            printf("some not record file type\n");
        }
        
    }
    lnode->result = lresult;
    closedir(dir1);
    closedir(dir2);

    return ERROR_NONE;
}

static void output_tree(fnode_t *root, op_e op)
{
    static int32_t i;
    fnode_t *tmp;
    char table[1000], fname[256], csv[5010];
    FILE *output;

    if (root == NULL) {
        printf("empty tree!\n");
        return;
    }

    if (op == DIFF) {
        output = fopen("./output.csv", "a");
        if (output == NULL) {
            printf("open output.csv fail\n");
            return;
        }
    }

    tmp = root;
    memset(table, '-', sizeof(table));
    while (tmp != NULL) {
        if (op == DIFF) {
            goto skip_print;
        }
        table[i] = '\0';
        get_short_name(fname, tmp->name);
        printf("%s%s", table, fname);
        printf("%s\n", g_result[tmp->result]);

    /* ��ֻ������ӡ�Ǳ��ʲ��������һ�����⣬��ʵ�ʱ��ʲ��������ֵܽ�㣬��ӡ�����������ж���֦辵��������ֵܽ�� */
skip_print:
        /* ������ͨ��diffѡ�������csv�ļ���ܣ���ʱ���н�㶼�����д�ӡ��������table�Ĳ���������� */
        if (op == DIFF && tmp->result != D_SMALL) {
            sprintf(csv, "%s,%s\n", tmp->name, g_result[tmp->result]);
            fwrite(csv, 1, strlen(csv), output);
        }

        if (tmp->son != NULL) {
            table[i] = '|';
            if (tmp->bro == NULL) {
                if (i > 1) {
                    table[i-1] = ' ';
                    table[i-2] = ' ';
                }
            } else {
                if (i > 1) {
                    table[i-1] = ' ';
                }
            }
            tmp = tmp->son;
            i++;
            i++;
            continue;
        }
        if (tmp->bro != NULL) {
            tmp = tmp->bro;
            table[i-1] = '-';
            continue;
        }
        /* ������һ�����ֵܽ��Ĳ� */
        while((tmp->bro) == NULL && tmp->pnode != &g_root) {
            table[i] = '-';
            if (i > 1) {
                table[i-1] = '-';
                table[i-2] = '-';
            }
            tmp = tmp->pnode;
            i--;
            i--;
        }
        tmp = tmp->bro;
        table[i-1] = '-';
    }

    if (op == DIFF) { 
        fclose(output);
    }

    return;
}

int main(int argc, char **argv) {
    op_e op;
    int32_t ret;
    char fname[256];
    fnode_t *uper;

    if (argc > ARGC) {
        printf("Too many argvs(only requires 3), ignored the extra ones.\n");
    } else if (argc < ARGC) {
        printf("Not enough argv, requires 3.\n");
        return ERROR_FAIL;
    } else if (strcmp(argv[3], "tree") && strcmp(argv[3], "diff")) {
        printf("Wrong output option, usage: cmp_ftree dir1 dir2 option(tree/diff)\n");
        return ERROR_FAIL;
    }

    switch (argv[3][0]) {
    case 't':
        op = TREE;
        break;
    case 'd':
        op = DIFF;
        break;
    default:
        return ERROR_FAIL;
        break;
    }

    fnode_init(&g_root, argv[1]);
    g_dir1_len = strlen(argv[1]);
    g_dir2_len = strlen(argv[2]);
    g_dir2 = (char *)malloc(g_dir2_len + 1);
    strcpy(g_dir2, argv[2]);

    /* �����ȥ���⻯ */
    uper = (fnode_t *)malloc(sizeof(fnode_t));
    fnode_init(uper, "..");
    g_root.pnode = uper;
    uper->son = &g_root;

    ret = compare_ftree(argv[1], argv[2], &g_root);
    if (ret != ERROR_NONE) {
        printf("compare fail.\n");
        return ERROR_FAIL;
    }
    output_tree(&g_root, op);

    return ERROR_NONE;
}
