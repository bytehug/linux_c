#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#define ERROR_NONE  (0)
#define ERROR_FAIL  (-1)

#define ARGC        (4)
#define MAX_LEN     (256)
#define MAX_PATH    (4096)
#define BUFF_SIZE   (1024)
#define MAX_TAB     (4096)
#define MAX_CSV     (5010)
#define D_SMALL     (0)         /* ��΢���� */
#define D_GREAT     (1)         /* ���ʲ��� */
#define D_NONE      (2)         /* �޶�Ӧ */
#define TAB         (2)

typedef enum cmp_op {
    TREE = 0,   /* ȫ����� */
    DIFF = 1,   /* ������� */
    OMAX = 2,
} op_t;

char g_result[3][11] = {    /* ������ʾ���, 3��������������10 */
    "-> D_SMALL",
    "-> D_GREAT",
    "-> D_NONE"
};

typedef struct fnode {
    char *name;
    int32_t result;
    struct fnode *pnode;
    struct fnode *son;
    struct fnode *bro;
} fnode_t;

fnode_t g_root;
char *g_dir2;
int32_t g_dir1_len;
int32_t g_dir2_len;

static int32_t fnode_init(fnode_t *node, char *name)
{
    if (name != NULL) {
        node->name = (char *) malloc (strlen(name) + 1);
        if (node->name == NULL) {
            printf("malloc node name fail, no memory!\n");
            return ERROR_FAIL;
        }

        memset(node->name, 0, strlen(name) + 1);
        strcpy(node->name, name);
    }

    node->result = 0;
    node->pnode = NULL;
    node->son = NULL;
    node->bro = NULL;

    return ERROR_NONE;
}

static void free_fnode(fnode_t *node)
{
    if (node->name != NULL) {
        free(node->name);
        node->name = NULL;
    }

    free(node);

    return;
}

static bool stat_equal(struct stat st1, struct stat st2)
{
    if ((st1.st_dev != st2.st_dev) \
            || (st1.st_ino != st2.st_ino) \
            || (st1.st_mode != st2.st_mode) \
            || (st1.st_nlink != st2.st_nlink) \
            || (st1.st_uid != st2.st_uid) \
            || (st1.st_gid != st2.st_gid) \
            || (st1.st_rdev != st2.st_rdev) \
            || (st1.st_size != st2.st_size) \
            || (st1.st_blocks != st2.st_blocks) \
            || (st1.st_blksize != st2.st_blksize) \
            || (st1.st_atime != st2.st_atime) \
            || (st1.st_ctime != st2.st_ctime) \
            || (st1.st_mtime != st2.st_mtime)) {
        return false;
    }

    return true;
}

static int32_t compare_files(fnode_t *fnode)        /* �ļ��������·�� */
{
    int32_t ret;
    struct stat st1, st2;
    FILE *f1, *f2;
    char *f2_path, buffer1[BUFF_SIZE + 1], buffer2[BUFF_SIZE + 1];
    char lnkbuff1[MAX_PATH + 1], lnkbuff2[MAX_PATH + 1];

    f2_path = (char *) malloc (g_dir2_len - g_dir1_len + strlen(fnode->name) + 1);
    if (f2_path == NULL) {
        printf("fail to malloc f2_path, no memory\n");
        return ERROR_FAIL;
    }

    /* ��fnode->name(��һ��Ŀ¼���ļ�·��)��ǰ������·�������滻�ɵڶ���Ŀ¼�������·�� */
    sprintf(f2_path, "%s/%s", g_dir2, (fnode->name) + g_dir1_len + 1);  /* mallocʱ��֤��С��Խ�� */
    memset(&st1, 0, sizeof(struct stat));
    memset(&st2, 0, sizeof(struct stat));
    ret = lstat(fnode->name, &st1);
    if (ret != ERROR_NONE) {
        printf("get lstat fail, node name:%s\n", fnode->name);
        free(f2_path);
        f2_path = NULL;
        return ERROR_FAIL;
    }

    ret = lstat(f2_path, &st2);
    if (ret != ERROR_NONE) {
        /* �򲻿���Ϊ�޶�Ӧ */
        fnode->result = D_NONE;
        free(f2_path);
        f2_path = NULL;
        return ERROR_NONE;
    }

    /* ��ͬ���͵��ļ����ִ��� */
    if ((st1.st_mode & S_IFMT) == S_IFREG) {
        /* normal file�����ٶ��������Բ��죬Ҫ��Ƚ��������ԣ���ʽ�ϱȽ��� */
        if (!stat_equal(st1, st2)) {
            fnode->result = D_SMALL;

            /* �ļ���С�в��������ݱ��в��죬ֱ�ӷ��� */
            if (st1.st_size != st2.st_size) {
                fnode->result = D_GREAT;
                free(f2_path);
                f2_path = NULL;
                return ERROR_NONE;
            }

            f1 = fopen(fnode->name, "rb");
            if (f1 == NULL) {
                printf("open file fail, f1:%s\n", fnode->name);
                fnode->result = D_GREAT;
                free(f2_path);
                f2_path = NULL;
                return ERROR_FAIL;
            }

            f2 = fopen(f2_path, "rb");
            if (f2 == NULL) {
                printf("open file fail, f2:%s\n", f2_path);
                fnode->result = D_GREAT;
                free(f2_path);
                f2_path = NULL;
                fclose(f1);
                return ERROR_FAIL;
            }

            memset(buffer1, 0, sizeof(buffer1));
            memset(buffer2, 0, sizeof(buffer2));
            while ((ret = fread(buffer1, 1, BUFF_SIZE, f1))) {
                if (ret != fread(buffer2, 1, BUFF_SIZE, f2)) {
                    fnode->result = D_GREAT;
                    break;
                }

                if (memcmp(buffer1, buffer2, ret)) {
                    fnode->result = D_GREAT;
                    break;
                } else {
                    continue;
                }
            }
            fclose(f1);
            fclose(f2);
        }    /* END OF IF, regular file compare done, �����Զ��޲��������账������ʼ��ֵ */
    } else if ((st1.st_mode & S_IFMT) == S_IFLNK) {
        /* symbolic link */
        if (!stat_equal(st1, st2)) {
            fnode->result = D_SMALL;

            if (st1.st_size != st2.st_size) {
                fnode->result = D_GREAT;
            } else {
                memset(lnkbuff1, 0, sizeof(lnkbuff1));
                memset(lnkbuff2, 0, sizeof(lnkbuff2));
                if (readlink(fnode->name, lnkbuff1, MAX_PATH) == ERROR_FAIL \
                        || readlink(f2_path, lnkbuff2, MAX_PATH) == ERROR_FAIL) {
                    printf("read link fail, link1:%s, link2:%s, errono:%d\n", \
                        fnode->name, f2_path, errno);
                    fnode->result = D_GREAT;
                    free(f2_path);
                    f2_path = NULL;
                    return ERROR_FAIL;
                }

                if (memcmp(lnkbuff1, lnkbuff2, MAX_PATH)) {
                    fnode->result = D_GREAT;
                }
            }
        }
    }   /* END OF ELSE IF, �������ļ�������� */

    free(f2_path);
    f2_path = NULL;

    return ERROR_NONE;
}

static int32_t compare_ftree(char *ftree1, char *ftree2, fnode_t *lnode)
{
    DIR *dir1, *dir2;
    struct dirent *ptr1;
    fnode_t *fnode, *pbro_tail;
    char *f2_path;
    int32_t lresult;

    dir1 = opendir(ftree1);
    if (dir1 == NULL) {
        printf("fail to opendir, dir1:%s\n", ftree1);
        return ERROR_FAIL;
    }

    dir2 = opendir(ftree2);
    /* �򲻿���Ϊ�޶�Ӧ */
    if (dir2 == NULL) {
        lnode->result = D_NONE;
        return ERROR_NONE;
    }

    /* ����·���µ������ļ� */
    pbro_tail = NULL;
    lresult = lnode->result;
    while ((ptr1 = readdir(dir1)) != NULL) {
        if (strcmp(ptr1->d_name, ".") == 0 || strcmp(ptr1->d_name, "..") == 0) {
            continue;
        }

        if (ptr1->d_type == DT_REG || ptr1->d_type == DT_LNK) {
            fnode = (fnode_t *) malloc (sizeof(fnode_t));
            if (fnode == NULL) {
                printf("malloc fnode fial, no memory\n");
                continue;
            }

            fnode->name = (char *) malloc (sizeof(ptr1->d_name) + 1 + strlen(lnode->name));
            if (fnode->name == NULL) {
                printf("malloc fnode name fail, no memory\n");
                continue;
            }

            sprintf(fnode->name, "%s/%s", lnode->name, ptr1->d_name);   /* mallocʱ��֤��Խ�� */
            (void)fnode_init(fnode, NULL);
            fnode->pnode = lnode;
            fnode->son   = NULL;
            (void)compare_files(fnode);
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
            fnode = (fnode_t *) malloc (sizeof(fnode_t));
            if (fnode == NULL) {
                printf("malloc fnode fail, no memory\n");
                continue;
            }

            fnode->name = (char *) malloc (sizeof(ptr1->d_name) + 1 + strlen(lnode->name));
            if (fnode->name == NULL) {
                printf("fail to malloc fnode name, no memory\n");
                continue;
            }

            sprintf(fnode->name, "%s/%s", lnode->name, ptr1->d_name);   /* mallocʱ��֤��С����Խ�� */
            (void)fnode_init(fnode, NULL);
            fnode->pnode = lnode;
            if (lnode->son == NULL) {
                lnode->son = fnode;
            } else {
                pbro_tail->bro = fnode;
            }

            pbro_tail = fnode;  /* �����ֵܽ������ */
            /* ftree2�����ֿ��ܺܳ���̣ܶ���fnode->name��һ������ô��,����ftree2�µĽ��·�� */
            f2_path = (char *) malloc (g_dir2_len - g_dir1_len + strlen(fnode->name) + 1);
            if (f2_path == NULL) {
                printf("fail to malloc f2_path, no memory\n");
                continue;
            }

            sprintf(f2_path, "%s/%s", g_dir2, (fnode->name) + g_dir1_len + 1);  /* mallocʱ��֤��С��Խ�� */
            (void)compare_ftree(fnode->name, f2_path, fnode);
            if ((lresult != D_GREAT) && (fnode->result != D_SMALL)) {
                lresult = D_GREAT;
            }

            free(f2_path);
            f2_path = NULL;
        } else {
            printf("some not record file type\n");
        }
    } /* END OF WHILE, compare tree build done*/
    lnode->result = lresult;
    closedir(dir1);
    closedir(dir2);

    return ERROR_NONE;
}

static void output_tree(fnode_t *root, op_t op)
{
    static int32_t i = 0;   /* ���������ʽ���� */
    fnode_t *tmp;
    char table[MAX_TAB + 1], fname[MAX_LEN + 1], csv[MAX_CSV + 1], *c;
    FILE *output;

    if (root == NULL) {
        printf("empty tree!\n");
        return;
    }

    output = NULL;
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
        memset(fname, 0, sizeof(fname));
        /* ��·���н�ȡ�ļ���,����ʾ��� */
        strncpy(fname, (c = strrchr(tmp->name, '/')) ? (c + 1) : tmp->name, MAX_LEN);
        printf("%s%s", table, fname);
        printf("%s\n", g_result[tmp->result]);
skip_print:
        if (op == DIFF && tmp->result != D_SMALL) {
            snprintf(csv, MAX_CSV, "%s,%s\n", tmp->name, g_result[tmp->result]);
            fwrite(csv, 1, strlen(csv), output);
        }

        /* ��������table�����i����ز�������Ϊ�˴�ӡ������Ŀ¼��֦辺ÿ���������߼��޹� */
        if (tmp->son != NULL) {
            table[i] = '|';
            if (tmp->bro == NULL) {
                if (i >= 2) {
                    table[i - 1] = ' ';
                    table[i - 2] = ' ';
                }
            } else {
                if (i > 1) {
                    table[i - 1] = ' ';
                }
            }

            tmp = tmp->son;

            if (i <= (MAX_TAB - TAB)) {
                i = i + TAB;
            }
            continue;
        }

        if (tmp->bro != NULL) {
            tmp = tmp->bro;
            if (i > 0) {
                table[i - 1] = '-'; /* ����������λ�� */
            }
            continue;
        }

        /* ������Ϊ�������Ϊ��Ŀ¼��ֱ��tmpָ���˳� */
        if (tmp == &g_root) {
            tmp = NULL;
            continue;
        }

        /* ������һ�����ֵܽ��Ĳ�, ����û���ֵܽ���������Ŀ¼���˳� */
        while ((tmp->bro) == NULL && tmp->pnode != &g_root) {
            if (i <= MAX_TAB && i >= 0) {
                table[i] = '-';
            }

            if (i >= 2) {   /* ����������λ��һ��TAB������ */
                table[i - 1] = '-';
                table[i - 2] = '-'; /* ��ԭtable���� */
                i = i - TAB;
            }
            tmp = tmp->pnode;
        }
        tmp = tmp->bro;
        if (i > 0) {
            table[i - 1] = '-';
        }
    }   /* END OF WHILE, print compare tree done */

    if (op == DIFF) {
        fclose(output);
    }

    return;
}

static void free_ftree(fnode_t *node_p)
{
    if (node_p->son != NULL) {
        free_ftree(node_p->son);
        node_p->son = NULL;
    }

    if (node_p->bro != NULL) {
        free_ftree(node_p->bro);
        node_p->bro = NULL;
    }

    if (node_p != &g_root) {
        free_fnode(node_p);
    }

    return;
}

int32_t main(int argc, char **argv)
{
    op_t op;
    int32_t ret;
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

    switch (argv[3][0]) {   /* ������������ĵ����������ַ�����diff��tree�ĵ�0���ַ� */
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

    ret = fnode_init(&g_root, argv[1]); /* ������������ĵ�1������������һ��Ŀ¼·�� */
    if (ret != ERROR_NONE) {
        printf("root node init fail, no memory.");
        return ERROR_FAIL;
    }

    /* �ֱ�Ϊ��Ŀ¼������ĵ�1��2������������1��2��Ŀ¼·�� */
    g_dir1_len = strlen(argv[1]);
    g_dir2_len = strlen(argv[2]);
    g_dir2 = (char *) malloc (g_dir2_len + 1);
    if (g_dir2 == NULL) {
        printf("fail to malloc g_dir2, no memory\n");
        free(g_root.name);
        g_root.name = NULL;
        return ERROR_FAIL;
    }

    strcpy(g_dir2, argv[2]);

    /* �����ȥ���⻯ */
    uper = (fnode_t *) malloc (sizeof(fnode_t));
    if (uper == NULL) {
        printf("fail to malloc uper node, no memory\n");
        free(g_root.name);
        g_root.name = NULL;
        return ERROR_FAIL;
    }

    ret = fnode_init(uper, "..");
    if (ret != ERROR_NONE) {
        printf("root pnode init fail, no memory.");
        free(g_root.name);
        g_root.name = NULL;
        free_fnode(uper);
        uper = NULL;
        return ERROR_FAIL;
    }

    g_root.pnode = uper;
    uper->son = &g_root;

    ret = compare_ftree(argv[1], argv[2], &g_root);
    if (ret != ERROR_NONE) {
        printf("compare fail.\n");
        free(g_root.name);
        g_root.name = NULL;
        free_fnode(uper);
        uper = NULL;
        free_ftree(&g_root);
        return ERROR_FAIL;
    }

    (void)output_tree(&g_root, op);

    printf("cleaning fnode tree...\n");
    free_fnode(uper);
    uper = NULL;
    free_ftree(&g_root);
    free(g_root.name);
    g_root.name = NULL;

    return ERROR_NONE;
}
