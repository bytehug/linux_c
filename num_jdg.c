#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define ERROR_NONE  (0)
#define MAX_LEN     (10000)
#define MAX_CNT     (1)

static inline bool is_num(char c)
{
    if (c < '0' || c > '9') {
        return false;
    }

    return true;
}

int32_t main(void)
{
    char input[MAX_LEN + 1];
    bool num_jdg;
    int32_t i, cnt_e, cnt_dot;

    memset(input, 0, sizeof(input));
    /* 使用fget输入 */
    while (fgets(input, MAX_LEN, stdin) != NULL) {
        cnt_e   = 0;    /* 记录出现的e个数 */
        cnt_dot = 0;    /* 记录出现的.个数 */
        num_jdg = true;
        /* fget会把结尾的回车符读入作为结束标志 */
        for (i = 0; i < MAX_LEN; i++) {
            if ((input[i] == '\n' || input[i] == '\0')) {
                break;
            }

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
                if (cnt_dot > MAX_CNT) {
                    num_jdg = false;
                    goto jdg_done;
                }

                if (!is_num(input[i + 1])) {
                    num_jdg = false;
                    goto jdg_done;
                }

                /* .出现在开头需要另行处理，以防下标越界 */
                if (i == 0) {
                    break;
                }

                /* .前后必有数字，且前面不应出现e */
                if (cnt_e != 0 || (!is_num(input[i - 1]) && !is_num(input[i + 1]))) {
                    num_jdg = false;
                    goto jdg_done;
                }
                break;
            case '+':
            case '-':
                if (i == 0) {
                    break;
                } else if (input[i - 1] == 'e') {
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

                /* .e情况能够被.判断规则规避, e前可能为数、点, e后可能为数或正负号 */
                if ((!is_num(input[i - 1]) && input[i - 1] != '.') \
                        || ((!is_num(input[i + 1]) && input[i + 1] != '+' \
                        && input[i + 1] != '-'))) {
                    num_jdg = false;
                    goto jdg_done;
                }
                break;
            default:
                num_jdg = false;
                goto jdg_done;
                break;
            }   /* END OF SWITCH */

            if (cnt_e > MAX_CNT || cnt_dot > MAX_CNT) {
                num_jdg = false;
                goto jdg_done;
            }
        }   /* END OF FOR */

jdg_done:
        printf("%d\n", num_jdg);
    }   /* END OF WHILE */

    return ERROR_NONE;
}

