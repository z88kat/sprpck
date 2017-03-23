/* bin2obj
   Take infile as input and creates and link65-object file, with
   the label outfile in outfile.
   
   written : 97/04/07
   
   CEF  = Carl Forhan (forhan@millcomm.com)
   42BS = 42Bastian Schick (elw5basc@gp.fht-esslingen.de)
   
   last change :

   04/28/97  CEF   Added '_' in front of OBJ label, and used strcat
                   instead of strcpy.
   04/29/96  42BS  Added option -U => OBJ-Label with underscore
                   
*/

#include <stdio.h>
#include <string.h>

#ifndef strupr

#include <ctype.h>
char *strupr(char *p)
{char *x = p;
  while (*p)
   *p++ = toupper(*p);
 return x;
}
#endif

int main(int argc,char *argv[])
{
FILE *in,*out;
long seg,segdata;
short i,o;
char label[34] = "";
short infile = 1;

  if ( argc < 3 )
  {
    printf("usage : bin2obj [-u] infile outfile\n");
    return (-1);
  }
  
  if ( argc == 4 )
    if ( (argv[1][0] == '-') && ( (argv[1][1] == 'u') || (argv[1][1] == 'U') ) )
    {
      label[0]='_';
      label[1]=0;
      ++infile;
    }
  if ( (in = fopen(argv[infile],"rb")) == NULL )
  {
	  printf("Couldn't open %s !\n",argv[infile]);
    return (-1);
  }
  ++infile; 
  
  strcat(label,argv[infile]);
  if ( (out = fopen(strcat(argv[infile],".obj"),"wb")) == NULL )
  {
	printf("Couldn't open %s !\n",argv[infile]);
    return (-1);
  }

  fseek(in,0L,SEEK_END);  
  seg = ftell(in);
  fseek(in,0L,SEEK_SET);
  segdata = seg + ((seg+0x1f)>>5);

/**************/

  putc(0xfd,out); putc(0xfd,out);  /*magic*/
  putc(7+(char)strlen(label),out); /*symbol-struct-len low*/
  putc(0,out);                     /*symbol-struct-len high*/
  putc(seg & 0xff,out);            /*data-len low*/
  putc(seg>>8,out);                /*data-len high*/
  putc(segdata & 0xff,out);        /*seg-len low*/
  putc(segdata>>8,out);            /*seg-len high*/
  putc(1,out);                     /*symbols low*/
  putc(0,out);                     /*symbols high*/
  putc(strlen(label),out);
/*  putc('_',out); */
/*  fprintf(out,"%s",strupr(label));*/
  fprintf(out,"%s",label);
  putc(0,out); putc(0,out);        /*symbol pos 0 rel to seg-start*/
  putc(7,out); putc(0,out);        /*symbol flag = rel to seg-start*/

  for (i = seg>>5; i ; --i)
  {
	putc(0,out);
    for (o = 32; o ; --o )
      putc(getc(in),out);
  }
  if ( (i = seg & 0x1f) )
  {
    putc(i,out);
    for ( ; i ; --i)
      putc(getc(in),out);
  }
  fclose(in);
  fclose(out);
  return 0;
}

