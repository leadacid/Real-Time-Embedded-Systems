#include "lcmwrapper.h"


int main(void)
{
    int a=146, b=18, gcf=0, lcm=0, quot=0, rem=0;

    gcf=gcfc(a,b);
    printf("C gcf(%d, %d)=%d\n", a, b, gcf);

    quot=divide(a,b);
    rem=remain(a,b);
    printf("C div(%d, %d)=%d, rem=%d\n", a, b, quot, rem);

    quot=divide(a*b, gcfc(a,b));
    rem=remain(a*b, gcfc(a,b));
    printf("C div(%d, %d)=%d, rem=%d\n", a*b, gcfc(a,b), quot, rem);

    lcm=lcmc(a,b);
    printf("C lcm(%d, %d)=%d\n", a, b, lcm);
}
