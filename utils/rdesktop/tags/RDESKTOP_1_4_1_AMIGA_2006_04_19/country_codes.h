#ifndef RDESKTOP_COUNTRY_CODES_H
#define RDESKTOP_COUNTRY_CODES_H

#include <exec/types.h>

struct ContryCodes {
    CONST_STRPTR	Country;
    CONST_STRPTR	Alpha3;
    CONST_STRPTR	DistinguishingSign;
    CONST_STRPTR	Alpha2;
    CONST_STRPTR	Extra;
    CONST_STRPTR	Language;
    LONG		WindowsLocaleCode;
    LONG		WindowsCodePage;
};

extern struct ContryCodes amiga_country_codes[];

#endif /* RDESKTOP_COUNTRY_CODES_H */
