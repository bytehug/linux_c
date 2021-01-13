#include <stdio.h>
#include <stdint.h>

#define PRIFIX "test"
#define TEST (PRIFIX"1")

int32_t main()
{
    printf(TEST);

    return 0;
}
