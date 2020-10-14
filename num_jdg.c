#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define ERROR_NONE  (0)
#define MAX_LEN     (10000)

static inline bool is_num(char c) {
    if (c < '0' || c > '9') {
        return false;
    }
    return true;
}

int main(void) {
    char input[MAX_LEN+1];
    bool num_jdg;
    int32_t i, cnt_e, cnt_dot;

    memset(input, 0, sizeof(input));
    /* 不用scanf保证空格输入，使用fget */
    while (fgets(input, MAX_LEN, stdin) != NULL) {
        cnt_e   = 0;
        cnt_dot = 0;
        num_jdg = true;
        /* fget会把结尾的回车符读入，作为结束标志 */
        for (i = 0; (input[i] != '\n' && input[i] != '\0') && i < MAX_LEN; i++) {
            switch (input[i]) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                break;
            case '.':
                cnt_dot++;
                if (cnt_dot > 1) {
                    num_jdg = false;
                    goto jdg_done;
                }
                /* .出现在开头需要另行处理，否则下标可能越界 */
                if (i == 0) {
                    if (!is_num(input[i+1])) {
                        num_jdg = false;
                        goto jdg_done;
                    }
                    break;
                }
                /* .前后必有数字，且前面不应出现e */
                if (cnt_e != 0 || (!is_num(input[i-1]) && !is_num(input[i+1]))) {
                    num_jdg = false;
                    goto jdg_done;
                }
                break;
            case '+':
            case '-':
                if (i == 0) {
                    break;
                } else if (input[i-1] == 'e') {
                    break;
                } else {
                    num_jdg = false;
                    break;
                }
            case 'e':
                cnt_e++;
                if (i == 0) {
                    num_jdg = false;
                    goto jdg_done;
                }

                /* 无需考虑.e，该情况能够被.判断规则规避 */
                if ((!is_num(input[i-1]) && input[i-1] != '.') || \
                    ((!is_num(input[i+1]) && input[i+1] != '+' && input[i+1] != '-'))) {
                    num_jdg = false;
                    goto jdg_done;
                }
                break;
            default: 
                num_jdg = false;
                goto jdg_done;
                break;
            }

            if (cnt_e > 1 || cnt_dot > 1) {
                num_jdg = false;
                goto jdg_done;
            }
        }

jdg_done:
        printf("%d\n", num_jdg);
    }

    return ERROR_NONE;
}

