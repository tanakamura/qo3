#ifndef QO3_CTYPE_H
#define QO3_CTYPE_H

#define QO3_CTYPE_

#define isprint(c) (((c)>=' ') && ((c)<='~'))
#define isdigit(c) (((c)>='0') && ((c)<='9'))
#define isspace(c) (((c)==' ') || (((c)>='\t') && ((c)<='\r')))
#define isupper(c) (((c)>='A') && ((c)<='Z'))
#define islower(c) (((c)>='a') && ((c)<='z'))
#define isalpha(c) (isupper(c) || islower(c))
#define toupper(c) (((islower(c))?((c)|0x20):(c)))
#define tolower(c) (((isupper(c))?((c)&~0x20):(c)))
#define isxdigit(c) (isdigit(c) || (((c)>='a')&&((c)<='f')) || (((c)>='A')&&((c)<='F')))

#endif
