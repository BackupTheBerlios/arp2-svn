/*  Keine Ahnung, ob das ok ist */

#include <stdlib.h>

ldiv_t ldiv(long num,long denom)
{
    ldiv_t r;
    r.quot=num/denom;
    r.rem=num%denom;
    if(denom<0) r.rem=-r.rem;
    return(r);
}

