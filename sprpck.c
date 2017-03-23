/*  
    lynx sprite packer
    Originally written 97/02/17 42Bastian Schick

    input-formats :
    SPS  - ASCII file, every pixel represented by a hex-value
    color 0 also with SPACE.
    every line must end with LF ( or CR LF on !*nix-machines)
    RAW1 - a byte contains 8 pixels.
    RAW4 - a byte contains two pixels , each in one nibble.
    first nibble is the upper-left corner
    RAW8 - a byte is one pixel
    PCX  - 8 bit / 1plane ore 1bit/8plane type
    PI1	- 1 bit / 4 planes (ST-format)
    BMP - 4 or 8 bits per pixel, not RLE encoded

    Contributors:
    DO      = Matthias Domin
    42BS    = 42Bastian Schick  <elw5basc@gp.fht-esslingen.de>
    CEF     = Carl Forhan       <forhan@millcomm.com>
    DH      = Dave Huseby       <dave@linuxprogrammer.org>

    Changelist:
    YY/MM/DD
    17/03/15    KL      64bit Support compile on MacOS Sierra 
    97/06/09

    97/03/20    DO      Support of SPS-files
    97/04/02    42BS    last-bit-bug
                        added *nix case (no CR !)
    97/04/05    42BS    SPS: Last line may have LF (CR)
                        added -v (verbose) and -c (color index compress)
                        removed TABs from source
                        output-file is now optional, default in+".spr"
                        io separated, conversion of the hole file
                        included PCX conversion.
    97/04/06    42BS    supports now 8bit/1plane or 1bit/4 plane PCX
    97/04/07    42BS    added palette output to the PCX-part
                        also redirection for the SCB
                        (needed with option -c !!)
    97/04/28    CEF     Added '!= 0' segment to eliminate compiler warning
    97/06/09    42BS    Added O_BINARY to IO.C (works now with DOS !)
    97/09/20    42BS    rebuild the interface ( xxxyyy )
                        now with offset (-oxxxyyy)
                        build in RAW1 for monochrome sources
                        moved O_BINARY to sprpck.h
                        added line-number to error-message
    97/11/25    42BS    moved a parameter-check to io.c
                        changed default-type to PCX
    98/07/02    42BS    Started to add PI1-support
    98/07/02    42BS    finished PI1-support
                        Input file need not to be reloaded => speed up
    98/07/23    42BS    Bug in ConvertPCX with 1 bit/8 planes PCX removed
    98/07/25    DO      MS Windows BMP-file support added
    98/08/01    DO      Added splitting of one picture into several sprites -ryyyxxx
                        Auto-setting of sprite pixel size using -c _and_ -z
    98/08/07    42BS    Cleaned up BMP-loading and include bin2obj-stuff
    98/08/18	DO	    Added 24bit-BMP support (but only for up to 16 colors)
                DO	    -Pfilename: PAL-file can now be given
    09/12/09    DH      Cleaned up to fix compiler errors/warnings on Linux
*/

/* warning : This Program assumes sizeof(int) = 32 !!
             If your C-compiler uses 16bit, be sure your input-file
             will not be bigger than 32767 pixels.
             Or use a 32bit compiler ;-).
*/

#define VER "1.98"
#include "sprpck.h"

int verbose;
BYTE rgb[32];
BYTE CollRedirect[16];
BYTE rgbinput[32];

/* function-prototypes */
/* io.c */
extern  void  error(int line,char *w, ...);
extern  void  SaveRGB(char *filename,BYTE *data,int type,int size,int line);
extern  int   ReadRGB(char *filename,BYTE *palinput);
extern  void  SaveSprite(char *filename,BYTE *data,int len,int line,int tzpe);
extern  unsigned long  LoadFile(char *filename,BYTE **adr);
extern  long  ConvertFile(BYTE *in,long in_size,int type,
                          int *in_w,int *in_h,int line);
extern BYTE* HandleOffset(BYTE * original,
           			 int *in_w,int *in_h,
                 int off_x,int off_y,
                 int line);
                 
/* sprpck.c */
BYTE * intobyte(int bits,BYTE val,BYTE *where);
BYTE * packline(BYTE *in,BYTE *out,int len,int size,int dir);
BYTE * unpackline(BYTE *in,BYTE *out,int len,int size,int dir);
void   intobuffer(BYTE *in,BYTE *buf,int len,int size, BYTE *pColIndexes);
int    packit(BYTE *raw,int iw, /*int  ih,*/ BYTE **spr,
              BYTE size,int  packed,
              int  w,int h,
              int  act_x,int act_y,
              BYTE *pColIndexes);
int   CountColors(BYTE *raw, int iw, int w, int h, BYTE *pColIndexes);
int   get2val(char * s,int *a,int *b);

/* End Of Prototypes */

/*
  Count colors and compress color-index
  Done by Matthias Domin
*/

int CountColors(BYTE *raw, int iw, int w, int h, BYTE *pColIndexes)
{
  BYTE  buffer[514];         /* max. 512 pels/line */
  BYTE  bOrgColIndexes[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
  int   nColor[16];
  int   nCount,x,y;
  long  lSum;


  for ( x = 0; x < 16; ++x )      /* init. vectors */
  {
    nColor[x] = 0;
    CollRedirect[x] = 0;
    *(pColIndexes+x) = '-';
  }

  for ( y = 0 ; y < h ; ++y )
  {
    intobuffer(raw+y*iw,buffer,w,4, bOrgColIndexes);

  /* pels are now in seperate bytes in buffer */
    for ( x = 0; x < w; ++x )
      nColor[buffer[x]] += 1;
  }
  nCount = 0;
  lSum = 0;
  for ( x = 0; x < 16; ++x )
  {
    if (nColor[x])
    {
      *(pColIndexes+x) = nCount;
      CollRedirect[nCount] = x;           /* SCB-Index */
      ++nCount;
    }
    lSum += nColor[x];
    //if (verbose) printf("Color# %d is used %d times.\n", x, nColor[x]);
  }
  if (verbose)
  {
    for ( x = 0; x < 16; ++x )
       printf("%4.1d|",x);
    for ( x = 0; x < 16; ++x )
       printf("----+");
    for ( x = 0; x < 16; ++x )
       printf("%4.1d|",nColor[x]);

    printf("Pixel sum: %ld\n"
           "Colors used: %d\n"
           "Index table:\n",lSum, nCount);
    for ( x = 0; x < 16 ; ++x )
      if (*(pColIndexes+x) == '-')
        printf("-");
      else
        printf("%hhX",*(pColIndexes+x));
    printf("\n");
  }
  return nCount;	// DO, 1. Aug 1998: for auto-adjusting of # of sprite colors!
}

/*
   insert 'bits' bits from 'val' into a byte
   if a byte is complete, write it to 'where'
*/
BYTE * intobyte(int bits,BYTE val,BYTE *where)
{
static BYTE bit_counter = 8,byte =0;

/* init intobyte */
  if ( !bits )
  {
    bit_counter = 8;
    byte =0;
    return NULL;
  }
/* handle end of line */
  if ( bits == 8)
  {
    byte <<= bit_counter;
    *where++ = byte;
    if ( byte & 0x1 )
      *where++ = 0;    /* be sure the last bit is never set ! */
    return (where);
  }
/* stuff it */
  val <<= 8-bits;

  do
  {
    byte <<= 1;

    if ( val & 0x80  )
      ++byte;

    if (! (--bit_counter) )
    {
      *where++ = byte;
      byte = 0;
      bit_counter = 8;
    }

    val  <<= 1;

  }while(--bits);

  return (where);
}
/*
   pack one line either to the left or right
*/
BYTE * packline(BYTE *in,      /* src  */
                BYTE *out,     /* dest.*/
                int len,       /* ???  */
                int size,      /* bits/pel */
                int dir)       /* -1 = left */
{
  BYTE differ[16],             /* buffer for literal bytes */
       *d_ptr,                 /* ptr in this buffer       */
       *out0,                  /* to save the dest.        */
       last;                   /* last byte from src       */

  int  counter;                /* counts the bytes         */

  out0  = out++;               /* save dest. ptr           */
  *out0 = 0;                   /* set byte count to zero   */

  intobyte(0,0,0);             /* init.                    */

  /* pack-loop */
  while ( len ){
    last = *in;                /* get a byte               */
    in += dir;                 /* step                     */
    --len;
    counter = 0;               /* init counter             */
    
    if ( (last == *in) && len)
    {
    /**/
    /* count the equal bytes, up to 16 */
    /**/

      do{
        ++counter;
        in += dir;
        --len;
      }while ( last == *in && len && counter != 15);

      out = intobyte(5,counter,out);
      out = intobyte(size,last,out);

    }else{

    /**/
    /* count different bytes , up to 16 */
    /**/

      d_ptr = differ;
      while ( last != *in && len && counter != 15)
      {
        *d_ptr++ = last;
      ++counter;
        last = *in;
        in += dir;
            --len;

      }/*while*/
      if ( !len || counter == 15)
        *d_ptr = last;
      else
        if ( last == *in )
        {
          --counter;
          in -=dir;
          ++len;
        }

      out = intobyte(5,counter|0x10,out);
      d_ptr = differ;
      do{
        out = intobyte(size,*d_ptr++,out);
      }while (--counter >= 0);

    }/* if last == *in */
  }/* while */

  /* exit */
  out = intobyte(8,0,out);

  *out0 = out-out0;
  return out;
}

/*
   pack the raw data into a literal sprite line
*/
BYTE * unpackline(BYTE *in, BYTE *out, int len, int size, int dir)
{
  BYTE *out0 = out++;

  intobyte(0,0,0);

  while (len){
    out = intobyte(size,*in,out);
    in += dir;
    --len;
  }
  out = intobyte(8,0,out);

  *out0 = out-out0;
  return out;
}
/*
  convert a line of raw data into a line of pixels
  reduced to the current bit-size
*/
void intobuffer(BYTE *in, BYTE *buf, int len, int size, BYTE *pColIndexes)
{
  BYTE b;
  int bit_mask = (1<<size)-1;
 
  while ( len )
  {
    b = *in++;
    b = *(pColIndexes+b);             /* redirect Color Index */
    b &= bit_mask;
    *buf++ = b;
    --len;
  }
  
  *buf = 0xff;    /* mark EOL */
}
/*
  convert raw input into sprite data
*/
int packit(BYTE *raw,  /* input data     */
           int  iw,
/*         int  ih, */   /* input size     */
           BYTE **spr,   /* output data    */
           BYTE size,    /* bits per pixel */
           int  packed,  /* <>0 => pack it */
           int  w,
           int  h,       /* size of sprite */
           int  act_x,
           int  act_y,   /*   action point */
           BYTE  * pColIndexes)

{
  BYTE buffer[514],     /* max. 512 pels/line */
       *spr0;

  long      y;

// March 2017.  Increase malloc for larger bitmap file support
// there was an error here in the original math
// ORIG:  if ( (*spr = spr0 = malloc((w+1)*h+1)) == NULL ) return 0;
  if ( (*spr = spr0 = malloc((w+1)*(h+1))) == NULL ) return 0;

/*** down/right ***/
  for ( y = act_y ; y<h ; ++y )
  {
    intobuffer(raw+(y*iw),buffer,w,size, pColIndexes);

    if (packed)
      spr0 = packline(buffer+act_x,spr0,w-act_x,size,1);
    else
      spr0 = unpackline(buffer+act_x,spr0,w-act_x,size,1);
  }
/*** up/right ***/
  if ( act_y || act_x )
  {
    *spr0++ = 0x01;

    for (y = act_y -1 ; y >= 0 ; --y)
    {        /* ^ casting needed because act_y is int */
      intobuffer(raw+(y*iw),buffer,w,size, pColIndexes);
      
      if (packed)
        spr0 = packline(buffer+act_x,spr0,w-act_x,size,1);
      else
        spr0 = unpackline(buffer+act_x,spr0,w-act_x,size,1);
    }
/*** up/left ***/
    if (act_x)
    {
      *spr0++ = 0x01;

      for (y = act_y-1 ; y >= 0  ; --y)
      {
        intobuffer(raw+(y*iw),buffer,w,size, pColIndexes);
        
        if (packed)
          spr0 = packline(buffer+act_x-1,spr0,act_x,size,-1);
        else
          spr0 = unpackline(buffer+act_x-1,spr0,act_x,size,-1);
      }
/*** down/left ***/
      *spr0++ = 0x01;
      for (y = act_y ; y < h ; ++y)
      {    
        intobuffer(raw+(y*iw),buffer,w,size, pColIndexes);
 
        if (packed)
          spr0 = packline(buffer+act_x-1,spr0,act_x,size,-1);
        else
          spr0 = unpackline(buffer+act_x-1,spr0,act_x,size,-1);
      }
    }
  }
  
  *spr0++ = 0; /* end of sprite-data */
  return ((int)(spr0-*spr));
}

/* read two decimal values out of s */
/* format is : aaabbb               */

int get2val(char * s,int *a,int *b)
{

  if (strlen(s) != 6)
  {
    printf("Error: Parameter must be 'xxxyyy' (decimal)!\n");
    return (0);
  }

  if ( ! sscanf(s+3,"%d",b) )
    return (0);

  *(s+3) = 0;
  if ( ! sscanf(s,"%d",a) )
    return (0);

  return (1);
}

/****************************************************************/
#define CMD_OPT 14+1 /* command = argv[0] ! */

extern BYTE * original;
extern int org_w,org_h;

int main(int argc,char *argv[])
{
/* batch */
FILE *batch_handle = NULL;

char my_argv[CMD_OPT][32],cmdline[128];

/* input */
char *infile = NULL;
BYTE *in,*raw;
long in_size;
int  in_w,in_h;
int  t_x, t_y;

/* output */
char outfile[128];
char outfile2[128];
char palfile[128];
char palinputfile[128];

char * extension;

BYTE *out;
int w,h,action_x,action_y,off_x,off_y,size,packed,type,sort_colindex, tiles, setsize;
int pal_output;

/* misc */
int ret,i,val,val1,err;
int line = 1;
BYTE  bColIndexes[16];
char *c_ptr,*dot;
int t_xx, t_yy;
int orgoff_x;/*, orgoff_y; */
int org_ww, org_hh;
int nColorsUsed;
int x,y;	
BYTE col_index;
BYTE  palexchange[16];
BYTE *pexchange;
/* end of var. decl. */

   t_x = t_y = 1;

  if (argc == 1 )
  {
    printf("Usage :\n"
            "sprpck [-c][-v][-s#][-t#][-u][-p#]\n"
            "[-axxxyyy][-Swwwhhh][-oxxxyyy][-iwwwhhh]\n"
            "[-rxxxyyy] [-z] [-Pfilename] in [out]\n"
            "or\n"
            "sprpck batchfile\n"
           "-c       : compress color index\n"
           "-v       : don't be quiet\n"
           "-s       : sprite-depth 4,3,2,1 bit(s) per pixel (4 default)\n"
           "-t       : type 0 = 4bit raw,  1 = 8bit raw, 2 = SPS, 3 = PCX (3 is default)\n"
           "           type 4 = 1bit raw type 5 = PI1 (Atari ST), 6 = MS Windows BMP \n"
           "-u       : unpacked     (packed is default)\n"
           "-p       : palette output-format : 0 - C, 1 - ASM, 2 - LYXASS(default)\n"
		   "-Pfilename adjust the colour palettes of input picture and given PAL-file\n"
           "-axxxyyy : action point (e.g. -a200020)\n"
           "-Swwwhhh : sprite width and height (input-size is default)\n"
           "-oxxxyyy : offset in data (e.g. -o010200 )\n"
           "-iwwwhhh : input size (not needed for PCX)\n"
           "-rxxxyyy : split picture into yyy * xxx tiles\n"
           "-z       : (only possible with -c) auto-set sprite-depth \n"
           "in       : input data\n"
           "out      : output filename, optional, default is in.spr\n"
           "\n"
	   "Note: With -p0 the sprite is saved as cc65-Object file !\n\n"
           "In batchmode, lines must have the same format as in command\n"
           "line-mode, only if a input-file is defined in one line it can be\n"
           "omitted in the following lines.\n"
           "\n"
           "1bit raw =  8 pels per byte ( => -s1 is default )\n"
           "4bit raw => 2 pels per byte\n"
           "8bit raw => 1 pel  per byte\n"
           "SPS      => ASCII-hex-number per pel (blank = 0)\n"
           "PCX      => either 8 bits / 1 plane or 1 bit /4 planes\n"
           "PI1      => 1 bit / 4 planes , Atari ST Low Rez-format\n"
           "BMP      => either 8 bits or 4 bits not RLE encoded\n"
           );
    exit(0);
  }

  printf("---------------------------\n"
         "Lynx Sprite Packer Ver "VER"\n"
         "(c) 1997,1998 42Bastian Schick \n"
         "              Matthias  Domin  \n"
         "---------------------------\n");
  --argc;

  for (i = 0; i < CMD_OPT ; ++i)
     my_argv[i][0] = 0;

  if ( argc == 1 && (batch_handle = fopen(argv[1],"r")) == (FILE *)NULL)
    error(1,"Wrong or no batch file found :%s !\n",argv[1]);

  do
  {

    /* set up default-values/switches */
    in_w     =
    in_h     =
    action_x =
    action_y =
    off_x    =
    off_y    =
    w        =
    h        =
    setsize  =
    tiles    = 0;
    size     = 4;
    packed 	 = 1;
    type     = TYPE_PCX;
    sort_colindex = 0;
    pal_output = LYXASS_SRC;

#ifdef DEBUG
    verbose = 1;
#else
    verbose = 0;
#endif

	*palinputfile = 0;

    for ( i = 0 ; i<16 ; ++i )
    {
      CollRedirect[i]=
      bColIndexes[i] = i;
    }
/*
  clear arg-list
*/
    for (i = 0; i < CMD_OPT ; ++i)
      my_argv[i][0] = 0;

    if (batch_handle)
    {
      *cmdline = 0;

      fgets(cmdline,127,batch_handle);
      argc = sscanf(cmdline,"%s %s %s %s %s %s %s %s %s %s %s",
             my_argv[1],my_argv[2],my_argv[3],my_argv[4],my_argv[5],
             my_argv[6],my_argv[7],my_argv[8],my_argv[9],my_argv[10],
             my_argv[11]);

      if ( argc <= 0) break;

    }
    else
    {
      for(i = 1; i < CMD_OPT ; ++i)
        if ( argv[i] == NULL )
          break;
        else
          strcpy(my_argv[i],argv[i]);
    }

    i = 1;
    c_ptr = my_argv[1];

    while ( *c_ptr == '-' && argc)
    {
      switch (*(c_ptr+1)){
	  case 'P' :		// DO, 18. Aug. 1998
		  {
			strcpy(palinputfile, c_ptr+2);
			if (verbose) printf("Using color palette out of %s !\n", palinputfile);
			if (!ReadRGB(palinputfile, rgbinput))
				error(line,"Couldn't read the palette-input-file !\n");
		  }
		  break;

      case 'r' :		// DO, 1. Aug. 1998
        if ( get2val(c_ptr+2,&val,&val1) )
        {
          t_x = val;
          t_y = val1;
          if (verbose) printf("Split into (x*y) %d * %d tiles.\n",t_x,t_y);

        }
        break;

      case 'z' :    // DO, 1. Aug. 1998
        setsize = 1;
        if (verbose) printf("Automatic setting of  sprite size !\n");
        break;

      case 's' :
        err = sscanf(c_ptr+2,"%d",&val);
        if (err && val>0 && val <5)
          {
          if ( verbose ) printf("Set size to :%d\n",val);
          size = val;
          }
        break;

      case 'u' :
        packed = 0;
        if (verbose) printf("Unpacked sprite !\n");
        break;

      case 't' :
        err = sscanf(c_ptr+2,"%d",&val);
        if ( err )
        {
          if (verbose) printf("Setting Filetype :%d !\n",val);

          type = val;    /* 0 = TYPE_RAW4, 1 = TYPE_RAW8, 2 = TYPE_SPS
                            3 = TYPE_PCX, 6 = TYPE_BMP */

          if (type == TYPE_RAW1)
          {
            size = 1;
            if ( verbose ) printf("Set size to : 1\n");
          }
         }
        break;

      case 'v' :
        verbose = 1;
        break;

      case 'c' :
         sort_colindex = 1;
         break;

      case 'i' :
        if ( get2val(c_ptr+2,&val,&val1) )
        {
          in_w = val;
          in_h = val1;
          if (verbose) printf("Setting input-size : %d,%d.\n",in_w,in_h);
        }
        break;

      case 'a' :
        if ( get2val(c_ptr+2,&val,&val1) )
        {
          action_x = val;
          action_y = val1;
          if (verbose) printf("Setting action-point :(%d,%d)\n",action_x,action_y);
        }
        break;

      case 'o' :
        if ( get2val(c_ptr+2,&val,&val1) )
        {
          off_x = val;
          off_y = val1;
          if (verbose) printf("Setting offset : (%d,%d)\n",off_x,off_y);
        }
        break;

      case 'p' :
        err = sscanf(c_ptr+2,"%d",&val);
        if ( err )
        {
          if (verbose) printf("Setting palette-output type :%d !\n",val);
          
          pal_output = val;

         }
        break;

      case 'S' :
        if ( get2val(c_ptr+2,&val,&val1) )
        {
          w = val;
          h = val1;
          if (verbose) printf("Setting sprite-size : %d,%d\n",w,h);
        }
        break;

      default :
          error(line,"Unsupported option(s) ! Leaving ...\n");

      }/*switch*/

      c_ptr=my_argv[++i];
      --argc;

    } /*while*/
    

    if ( *my_argv[i] == 0 && original == 0)
      error(line,"Missing input-file !\n");


//
// get new input file if:
//  a) two filenames are given
//  b) one filename is given and no file was prev. loaded
//

    if ( (*my_argv[i] != 0 && *my_argv[i+1] != 0)  ||
         (*my_argv[i] != 0 && *my_argv[i+1] == 0 && !original)   )
    {
      infile = my_argv[i++];

      if ( ! ( type == TYPE_PCX || type == TYPE_PI1 || type == TYPE_BMP)
           && ( (in_w == 0) || ( in_h == 0) ) )
        error(line,"Input-size not set !\n");

      if ( (in_size = LoadFile(infile,&in)) != 0 )
      {
      	if ( original ) free(original);

        in_size = ConvertFile(in,in_size,type,&in_w,&in_h,line);


/* check input-filesize */

        if (in_size != org_w * org_h)
        {
          free(original);
          error(line,"Wrong picture-size !\n");
        }


// DO:  18. Aug. 1998
// adjust the picture palette and the given palette
		if (strlen(palinputfile))
		{
			printf("Adjusting colour palette!\n\n");
			for (y = 0; y < 16; y++)
				palexchange[y] = y;		// so all values are set!


			for (y = 0; y < 16; y++)
			{
				palexchange[y] = y;		// so all values are set!
				x = 0;
				while (x < 16)
				{
					if (*((short*)&rgbinput[2*x]) == *((short*)&rgb[2*y]))
					{
						if (verbose) printf("Picture colour# %d found as colour# %d in input-palette!\n", y, x);
						palexchange[y] = x;
						break;
					}
					x++;
				}

			}

			pexchange = original;
			for (y = 0; y < org_h; y++)
				for (x = 0; x < org_w; x++)
				{
					col_index = *pexchange;
					*pexchange++ = palexchange[col_index];
				}
		}


#ifdef DEBUG
      	SaveSprite("raw.spr",original,in_size,line);
				printf("w=%d, h=%d size=%ld\n",in_w,in_h,in_size);
#endif
      }
      else
      {
        error(line,"Couldn't load %s !\n",infile);
      }

    }
    else
    {
      in_w = org_w;
      in_h = org_h;
    }
//
// if no outfile if defined take the name of the input-file
//
    if ( *my_argv[i] == 0 || !sscanf(my_argv[i],"%s",outfile) )
       strcpy(outfile,infile);
//
// create output-filenames
//
    strcpy(palfile,outfile);
    dot = strrchr(outfile,'.');
    if ( dot != NULL )
    {
      *dot = 0;
      *(palfile+(dot-outfile)) = 0;
    }
   
    strcat(palfile,".pal");
    
    if ( pal_output != C_HEADER ) 
      extension = (char *)&".spr";
    else
      extension = (char *)&".obj";
//
// if sprite-size is not defined, take input-size as default
//
    if ( w == 0 ) w = in_w;
    if ( h == 0 ) h = in_h;

    if ( w > in_w || h > in_h )
      error(line,"Sprite > input picture !\nsprite = %d,%d input = %d,%d", w,h, in_w, in_h);

    tiles = t_x * t_y;

    if (tiles)
       if ((off_x + t_x * w) > in_w || (off_y + t_y * h > in_h) )
          error(line,"Can't split the picture in that much tiles !\n");

    // Split the input file into several tiles ***************************
       org_ww = org_w;
       org_hh = org_h;
       orgoff_x = off_x;
       //orgoff_y = off_y;
       for (t_yy = 0; t_yy < t_y; t_yy++)
          {
          off_x = orgoff_x;
          for (t_xx = 0; t_xx < t_x; t_xx++)
             {
             in_w = org_ww;
             in_h = org_hh;
             if (verbose) printf("Tile nr: %d\n", (t_x * t_y) - tiles + 1);
//
// do offset (original is preserved)
//
             raw = HandleOffset(original,&in_w,&in_h,off_x,off_y,line);
//
// compress colors
//
             if (sort_colindex)
               {
               nColorsUsed = CountColors(raw, in_w, w, h, bColIndexes);
#ifdef DEBUG
               printf ("Colors %d\n", nColorsUsed);
#endif
               if (setsize)
                  {
                  if (nColorsUsed <= 2)
                     size = 1;
                  else if (nColorsUsed <= 4)
                     size = 2;
                  else if (nColorsUsed <= 8)
                     size = 3;
                  else // up to 16 colors
                     size = 4;
                  }
               }

//
// now pack the sprite
//
             ret = packit(raw, in_w,/*in_h,*/ &out, size, packed,
                          w, h, action_x, action_y, bColIndexes);

             free(raw); // no need for this anymore

             if ( ret )
                {
                if (tiles == 1)	// Only once
                   if (type == TYPE_PCX || type == TYPE_PI1 || type == TYPE_BMP)
                      SaveRGB(palfile,rgb,pal_output,size,line);

                // if just one tile, use the original outfile name
                if (t_x * t_y == 1)
                   sprintf(outfile2,"%s%s", outfile, extension);
                else
                   sprintf(outfile2,"%s%3.3d%3.3d%s", outfile, t_yy, t_xx, extension);

                SaveSprite(outfile2,out,ret,line,pal_output);

                free(out);
                }
             else
                error(line,"Packed size = 0!");

             off_x += w;  // next column of tiles;
             tiles--;
             } // for t_xx
          off_y += h;	// next row of tiles
          } // for t_yy
      // split the input into several tiles *****************



  }while (batch_handle && !feof(batch_handle) );

  if (batch_handle)
    fclose(batch_handle);

#ifdef ATARI
  if (verbose)  getc(stdin);
#endif
  return 0;
}
