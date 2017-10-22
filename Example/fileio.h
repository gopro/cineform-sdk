/*

readavi - Copyright 2004-2013 by Michael Kohn <mike@mikekohn.net>
For more info please visit http://www.mikekohn.net/

This code falls under the BSD license.

*/

int write_long(FILE *out, int n);
int write_word(FILE *out, int n);

int read_long(FILE *in);
int read_word(FILE *in);

int read_chars(FILE *in, char *s, int count);
int read_chars_bin(FILE *in, char *s, int count);
int write_chars(FILE *out, char *s);
int write_chars_bin(FILE *out, char *s, int count);

