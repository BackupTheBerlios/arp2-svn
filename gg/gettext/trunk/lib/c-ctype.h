/* Character handling in C locale.

   These functions work like the corresponding functions in <ctype.h>,
   except that they have the C (POSIX) locale hardwired, whereas the
   <ctype.h> functions' behaviour depends on the current locale set via
   setlocale.

   Copyright (C) 2000, 2001 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef C_CTYPE_H
#define C_CTYPE_H

#ifndef PARAMS
# if defined (__GNUC__) || __STDC__
#  define PARAMS(args) args
# else
#  define PARAMS(args) ()
# endif
#endif


/* Check whether the ASCII optimizations apply. */

#if ('0' <= '9') \
    && ('0' + 1 == '1') && ('1' + 1 == '2') && ('2' + 1 == '3') \
    && ('3' + 1 == '4') && ('4' + 1 == '5') && ('5' + 1 == '6') \
    && ('6' + 1 == '7') && ('7' + 1 == '8') && ('8' + 1 == '9')
#define C_CTYPE_CONSECUTIVE_DIGITS 1
#endif

#if ('A' <= 'Z') \
    && ('A' + 1 == 'B') && ('B' + 1 == 'C') && ('C' + 1 == 'D') \
    && ('D' + 1 == 'E') && ('E' + 1 == 'F') && ('F' + 1 == 'G') \
    && ('G' + 1 == 'H') && ('H' + 1 == 'I') && ('I' + 1 == 'J') \
    && ('J' + 1 == 'K') && ('K' + 1 == 'L') && ('L' + 1 == 'M') \
    && ('M' + 1 == 'N') && ('N' + 1 == 'O') && ('O' + 1 == 'P') \
    && ('P' + 1 == 'Q') && ('Q' + 1 == 'R') && ('R' + 1 == 'S') \
    && ('S' + 1 == 'T') && ('T' + 1 == 'U') && ('U' + 1 == 'V') \
    && ('V' + 1 == 'W') && ('W' + 1 == 'X') && ('X' + 1 == 'Y') \
    && ('Y' + 1 == 'Z')
#define C_CTYPE_CONSECUTIVE_UPPERCASE 1
#endif

#if ('a' <= 'z') \
    && ('a' + 1 == 'b') && ('b' + 1 == 'c') && ('c' + 1 == 'd') \
    && ('d' + 1 == 'e') && ('e' + 1 == 'f') && ('f' + 1 == 'g') \
    && ('g' + 1 == 'h') && ('h' + 1 == 'i') && ('i' + 1 == 'j') \
    && ('j' + 1 == 'k') && ('k' + 1 == 'l') && ('l' + 1 == 'm') \
    && ('m' + 1 == 'n') && ('n' + 1 == 'o') && ('o' + 1 == 'p') \
    && ('p' + 1 == 'q') && ('q' + 1 == 'r') && ('r' + 1 == 's') \
    && ('s' + 1 == 't') && ('t' + 1 == 'u') && ('u' + 1 == 'v') \
    && ('v' + 1 == 'w') && ('w' + 1 == 'x') && ('x' + 1 == 'y') \
    && ('y' + 1 == 'z')
#define C_CTYPE_CONSECUTIVE_LOWERCASE 1
#endif

#if (' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
    && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
    && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
    && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
    && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
    && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
    && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
    && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
    && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
    && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
    && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
    && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
    && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
    && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
    && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
    && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
    && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
    && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
    && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
    && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
    && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
    && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
    && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126)
/* The character set is ISO-646, not EBCDIC. */
#define C_CTYPE_ASCII 1
#endif


/* Function declarations. */

extern int c_isascii PARAMS ((int c)); /* not locale dependent */

extern int c_isalnum PARAMS ((int c));
extern int c_isalpha PARAMS ((int c));
extern int c_isblank PARAMS ((int c));
extern int c_iscntrl PARAMS ((int c));
extern int c_isdigit PARAMS ((int c));
extern int c_islower PARAMS ((int c));
extern int c_isgraph PARAMS ((int c));
extern int c_isprint PARAMS ((int c));
extern int c_ispunct PARAMS ((int c));
extern int c_isspace PARAMS ((int c));
extern int c_isupper PARAMS ((int c));
extern int c_isxdigit PARAMS ((int c));

extern int c_tolower PARAMS ((int c));
extern int c_toupper PARAMS ((int c));


#if defined __GNUC__ && defined __OPTIMIZE__ && !defined __OPTIMIZE_SIZE__

/* ASCII optimizations. */

#define c_isascii(c) \
  ({ int __c = (c); \
     ((__c & ~0x7f) == 0); \
   })

#if C_CTYPE_CONSECUTIVE_DIGITS \
    && C_CTYPE_CONSECUTIVE_UPPERCASE && C_CTYPE_CONSECUTIVE_LOWERCASE
#define c_isalnum(c) \
  ({ int __c = (c); \
     ((__c >= '0' && __c <= '9') \
      || (__c >= 'A' && __c <= 'Z') \
      || (__c >= 'a' && __c <= 'z')); \
   })
#endif

#if C_CTYPE_CONSECUTIVE_UPPERCASE && C_CTYPE_CONSECUTIVE_LOWERCASE
#define c_isalpha(c) \
  ({ int __c = (c); \
     ((__c >= 'A' && __c <= 'Z') || (__c >= 'a' && __c <= 'z')); \
   })
#endif

#define c_isblank(c) \
  ({ int __c = (c); \
     (__c == ' ' || __c == '\t'); \
   })

#if C_CTYPE_ASCII
#define c_iscntrl(c) \
  ({ int __c = (c); \
     ((__c & ~0x1f) == 0 || __c == 0x7f); \
   })
#endif

#if C_CTYPE_CONSECUTIVE_DIGITS
#define c_isdigit(c) \
  ({ int __c = (c); \
     (__c >= '0' && __c <= '9'); \
   })
#endif

#if C_CTYPE_CONSECUTIVE_LOWERCASE
#define c_islower(c) \
  ({ int __c = (c); \
     (__c >= 'a' && __c <= 'z'); \
   })
#endif

#if C_CTYPE_ASCII
#define c_isgraph(c) \
  ({ int __c = (c); \
     (__c >= '!' && __c <= '~'); \
   })
#endif

#if C_CTYPE_ASCII
#define c_isprint(c) \
  ({ int __c = (c); \
     (__c >= ' ' && __c <= '~'); \
   })
#endif

#if C_CTYPE_ASCII
#define c_ispunct(c) \
  ({ int _c = (c); \
     (c_isgraph (_c) && ! c_isalnum (_c)); \
   })
#endif

#define c_isspace(c) \
  ({ int __c = (c); \
     (__c == ' ' || __c == '\t' \
      || __c == '\n' || __c == '\v' || __c == '\f' || __c == '\r'); \
   })

#if C_CTYPE_CONSECUTIVE_UPPERCASE
#define c_isupper(c) \
  ({ int __c = (c); \
     (__c >= 'A' && __c <= 'Z'); \
   })
#endif

#if C_CTYPE_CONSECUTIVE_DIGITS \
    && C_CTYPE_CONSECUTIVE_UPPERCASE && C_CTYPE_CONSECUTIVE_LOWERCASE
#define c_isxdigit(c) \
  ({ int __c = (c); \
     ((__c >= '0' && __c <= '9') \
      || (__c >= 'A' && __c <= 'F') \
      || (__c >= 'a' && __c <= 'f')); \
   })
#endif

#if C_CTYPE_CONSECUTIVE_UPPERCASE && C_CTYPE_CONSECUTIVE_LOWERCASE
#define c_tolower(c) \
  ({ int __c = (c); \
     (__c >= 'A' && __c <= 'Z' ? __c - 'A' + 'a' : __c); \
   })
#define c_toupper(c) \
  ({ int __c = (c); \
     (__c >= 'a' && __c <= 'z' ? __c - 'a' + 'A' : __c); \
   })
#endif

#endif /* optimizing for speed */

#endif /* C_CTYPE_H */
