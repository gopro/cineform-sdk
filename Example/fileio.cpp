/*

readavi - Copyright 2004-2013 by Michael Kohn <mike@mikekohn.net>
For more info please visit http://www.mikekohn.net/

This code falls under the BSD license.

*/

#include <stdio.h>
#include <stdlib.h>

int write_long(FILE *out, int n)
{
  putc((n&255),out);
  putc(((n>>8)&255),out);
  putc(((n>>16)&255),out);
  putc(((n>>24)&255),out);

  return 0;
}

int write_word(FILE *out, int n)
{
  putc((n&255),out);
  putc(((n>>8)&255),out);

  return 0;
}

int read_long(FILE *in)
{
int c;

  c=getc(in);
  c=c+(getc(in)<<8);
  c=c+(getc(in)<<16);
  c=c+(getc(in)<<24);

  return c;
}

int read_word(FILE *in)
{
int c;

  c=getc(in);
  c=c+(getc(in)<<8);

  return c;
}

int read_chars(FILE *in, char *s, int count)
{
int t;

  for (t=0; t<count; t++)
  {
    s[t]=getc(in);
  }

  s[t]=0;

  return 0;
}

int read_chars_bin(FILE *in, char *s, int count)
{
int t;

  for (t=0; t<count; t++)
  {
    s[t]=getc(in);
  }

  return 0;
}

int write_chars(FILE *out, char *s)
{
int t;

  t=0;
  while(s[t]!=0 && t<255)
  {
    putc(s[t++],out);
  }

  return 0;
}

int write_chars_bin(FILE *out, char *s, int count)
{
int t;

  for (t=0; t<count; t++)
  {
    putc(s[t],out);
  }

  return 0;
}

