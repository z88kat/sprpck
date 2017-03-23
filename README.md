The Atari Lynx Sprite Packer

## Description

The easiest way to create the sprite pixel data is to use the sprpck tool. This tool can create the sprite data structure.  It works exclusively for the Lynx sprite data.

## Requirements

gcc<br />
cc65

## Building

```
make clean
make
```

This will generate the `sprpck` binary

## Usage

The `sprpck` binary needs to be in your build path.  You can just put directly into the cc65/bin directory.

```
sprpck [-c][-v][-s#][-t#][-u][-p#]
[-axxxyyy][-Swwwhhh][-oxxxyyy][-iwwwhhh]
[-rxxxyyy] [-z] [-Pfilename] in [out]
or
sprpck batchfile
-c       : compress color index
-v       : don't be quiet
-s       : sprite-depth 4,3,2,1 bit(s) per pixel (4 default)
-t       : type 0 = 4bit raw,  1 = 8bit raw, 2 = SPS, 3 = PCX (3 is default)
           type 4 = 1bit raw type 5 = PI1 (Atari ST), 6 = MS Windows BMP 
-u       : unpacked     (packed is default)
-p       : palette output-format : 0 - C, 1 - ASM, 2 - LYXASS(default)
-Pfilename adjust the colour palettes of input picture and given PAL-file
-axxxyyy : action point (e.g. -a200020)
-Swwwhhh : sprite width and height (input-size is default)
-oxxxyyy : offset in data (e.g. -o010200 )
-iwwwhhh : input size (not needed for PCX)
-rxxxyyy : split picture into yyy * xxx tiles
-z       : (only possible with -c) auto-set sprite-depth 
in       : input data
out      : output filename, optional, default is in.spr

Note: With -p0 the sprite is saved as cc65-Object file !

In batchmode, lines must have the same format as in command
line-mode, only if a input-file is defined in one line it can be
omitted in the following lines.

1bit raw =  8 pels per byte ( => -s1 is default )
4bit raw => 2 pels per byte
8bit raw => 1 pel  per byte
SPS      => ASCII-hex-number per pel (blank = 0)
PCX      => either 8 bits / 1 plane or 1 bit /4 planes
PI1      => 1 bit / 4 planes , Atari ST Low Rez-format
BMP      => either 8 bits or 4 bits not RLE encoded
```


## Lynx MakeFile

You need to update your Makefile to make use of the Sprite Packer.

```
SPRPCK=sprpck

# Rule for making a *.o file out of a *.bmp file
.bmp.o:
	$(SPRPCK) -v -t6 -p2 $<
	$(ECHO) .global _$(*B) > $*.s
	$(ECHO) .segment '"$(RODATA_SEGMENT)"' >> $*.s
	$(ECHO) _$(*B): .incbin '"$*.spr"' >> $*.s
	$(AS) -t lynx -o $@ $(AFLAGS) $*.s
	$(RM) $*.s
	$(RM) $*.pal
	$(RM) $*.spr
```

The make tool will find and use the rule above for the robot.bmp file and produce a couple of output files:

foo.spr (contains the sprite pixel data)<br />
foo.pal (contains color palette information)<br />
foo.s   (assembler code)<br />
foo.o   (the final object file with the compiled pixel data and reference variables)<br />


## BMP Support

BMP support is a bit patchy.  The sprpck will have problems with compressed BMP files.<br />
I have found that exported BMP's generated with GIMP work fine.

## Tested On

MacOS Sierra.  But should compile on any 64bit Linux OS.

