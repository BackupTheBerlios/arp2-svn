#include <stdio.h>
#include <string.h>

#define fprintf(args...)

int decompress(signed char *data, int clen, signed char ctype,
               signed char *dict, int *roff, int *rlen)
{
	int k, k1 = 0, k2;
	int i = 0;
	int i1, i2;
	int l1;
	int j1, j2;

	fprintf(stderr, "Offset: %d\n", i);
	fprintf(stderr, "Compressed Length: %d\n", clen);
	fprintf(stderr, "Compress Type: %d\n", ctype);
	fprintf(stderr, "Expected Length: %d\n", rlen);

	if ((ctype & 0x20) == 0) {
		fprintf(stderr, "not compressed\n");
		*roff = 0;
		*rlen = clen;
		return 0;
	}
	
	if ((ctype & 0x40) != 0) {
		dict[8196] = 0;
		dict[8197] = 0;
		dict[8198] = 0;
		dict[8199] = 0;
	}

	if ((ctype & 0x80) != 0)
		memset(dict, 0, 8200);

	*roff = 0;
	*rlen = 0;

	k2 = ((dict[8199] & 0xff) << 24) | ((dict[8198] & 0xff) << 16) | ((dict[8197] & 0xff) << 8) | (dict[8196] & 0xff);
	fprintf(stderr, "k2=%d\n", k2);
	i1 = k2;
	j1 = i1;
	*roff = j1;
	if (clen == 0)
		return 0;
	clen += i;
	
	do {
		if (k1 == 0) {
			if (i >= clen)
				break;
			k2 = data[i++] << 24;
			k1 = 8;
		}
		if (k2 >= 0) {
			if (k1 < 8) {
				if (i >= clen) {
					if (k2 != 0)
						return -1;
					break;
				} 
				k2 |= (data[i++] & 0xff) << (24 - k1);
				k1 += 8;
			}
			if (i1 >= 8192)
				return -1;
			dict[i1++] = (((unsigned int)k2) >> ((unsigned int)24));
			k2 <<= 8;
			k1 -= 8;
			continue;
		}
		k2 <<= 1;
		if (--k1 == 0) {
			if (i >= clen)
				return -1;
			k2 = data[i++] << 24;
			k1 = 8;
		}
		if (k2 >= 0) {
			if (k1 < 8) {
				if (i >= clen)
					return -1;
				k2 |= (data[i++] & 0xff) << (24 - k1);
				k1 += 8;
			}
			if (i1 >= 8192)
				return -1;
			dict[i1++] = (signed char)(k2 >> 24 | 0x80);
			k2 <<= 8;
			k1 -= 8;
			continue;
		}
		k2 <<= 1;
		if (--k1 < 2) {
			if (i >= clen)
				return -1;
			k2 |= (data[i++] & 0xff) << (24 - k1);
			k1 += 8;
		}
		switch (((unsigned int)k2) >> ((unsigned int)30)) {
			case 3:
				if (k1 < 8) {
					if (i >= clen)
						return -1;
					k2 |= (data[i++] & 0xff) << (24 - k1);
					k1 += 8;
				}
				k2 <<= 2;
				i2 = ((unsigned int)k2) >> ((unsigned int)26);
				k2 <<= 6;
				k1 -= 8;
				break;

			case 2:
				for (; k1 < 10; k1 += 8) {
					if (i >= clen)
						return -1;
					k2 |= (data[i++] & 0xff) << (24 - k1);
				}

				k2 <<= 2;
				i2 = (((unsigned int)k2) >> ((unsigned int)24)) + 64;
				k2 <<= 8;
				k1 -= 10;
				break;

			default:
				for (; k1 < 14; k1 += 8) {
					if (i >= clen)
						return -1;
					k2 |= (data[i++] & 0xff) << (24 - k1);
				}

				i2 = (k2 >> 18) + 320;
				k2 <<= 14;
				k1 -= 14;
				break;
		}
		if (k1 == 0) {
			if (i >= clen)
				return -1;
			k2 = data[i++] << 24;
			k1 = 8;
		}
		l1 = 0;
		if (k2 >= 0) {
			l1 = 3;
			k2 <<= 1;
			k1--;
		} else {
			j2 = 11;
			do {
				k2 <<= 1;
				if (--k1 == 0) {
					if (i >= clen)
						return -1;
					k2 = data[i++] << 24;
					k1 = 8;
				}
				if (k2 >= 0)
					break;
				if (--j2 == 0) {
					return -1;
				}
			} while (1);
			l1 = 13 - j2;
			k2 <<= 1;
			if (--k1 < l1) {
				for (; k1 < l1; k1 += 8) {
					if (i >= clen) {
						return -1;
					}
					k2 |= (data[i++] & 0xff) << (24 - k1);
				}
			}

			j2 = l1;
			l1 = k2 >> 32 - j2 & ~( -1 << j2) | 1 << j2;
			k2 <<= j2;
			k1 -= j2;
		}
		if (i1 + l1 >= 8192) {
			return -1;
		}
		k = i1 - i2 & 0x1fff;
		do {
			dict[i1++] = dict[k++];
		} while (--l1 != 0);
	} while (1);

	dict[8196] = (signed char)(i1 & -1);
	dict[8197] = (signed char)(i1 >> 8 & -1);
	dict[8198] = (signed char)(i1 >> 16 & -1);
	dict[8199] = (signed char)(i1 >> 24 & -1);

	*roff = j1;
	*rlen = i1 - j1;

	return 0;
}
