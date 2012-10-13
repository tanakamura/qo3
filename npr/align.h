#ifndef NPR_ALIGN_H
#define NPR_ALIGN_H

#define NPR_ALIGN_UP(x,to) (((x)+((to)-1U))&~((to)-1U))
#define NPR_ALIGN_DOWN(x,to) ((x)&~((to)-1U))

#define NPR_CEIL_DIV(num,denom) (((num)+(denom-1))/denom)

#endif
