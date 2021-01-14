#include <stdint.h>
#include <stdio.h>

typedef enum op_type {
    alt1,
    alt2,
    max
} op_type_e;

typedef int32_t (*print_cb)(char *content);
typedef int32_t (*func_type)(op_type_e op, print_cb func, void* arg);

int32_t hello_world(char *content)
{
    printf("%s\n", content);

    return 0;
}

int32_t test(op_type_e op, print_cb func, void* arg)
{
    func(((char*)arg));

    return 0;
}

int32_t main(void)
{
    char arg[] = "hello world!";

    test(alt1, hello_world, &arg);

    return 0;
}
