#include <stdio.h>

extern int PPCAllocMem;

int main(int argc,char *argv[])
{
	int i;
	FILE *f,*f2;
	char *p;
	int c;
	for(i=0;i<argc;i++)
		printf("%i. %s\n",i,argv[i]);
	f=fopen("test.c","rb");
	f2=fopen("te","wb");
	while((c=getc(f))!=EOF)
		putc(c,f2);
	fclose(f2);
	fclose(f);
	return (12345l);
}
