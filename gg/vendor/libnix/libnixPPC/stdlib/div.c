/*  Keine Ahnung, ob das ok ist */

#include <stdlib.h>

div_t div(int num,int denom)
{
    div_t r;
    r.quot=num/denom;
    r.rem=num%denom;
    if(denom<0) r.rem=-r.rem;
    return(r);
}

