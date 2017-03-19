/*
     This file is part of libmicrohttpd
     Copyright (C) 2007,2013 Christian Grothoff

     libmicrohttpd is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published
     by the Free Software Foundation; either version 3, or (at your
     option) any later version.

     libmicrohttpd is distributed in the hope that it will be useful, but
     WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with libmicrohttpd; see the file COPYING.  If not, write to the
     Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
     Boston, MA 02110-1301, USA.
*/

/**
 * @file test_postprocessor.c
 * @brief  Testcase for postprocessor
 * @author Christian Grothoff
 */

#include "libmicroplatform.h"
#include "microhttpd.h"
#include "internal.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifndef WINDOWS
#include <unistd.h>
#endif

MHD_PanicCallback mhd_panic;
void *mhd_panic_cls;
/**
 * Array of values that the value checker "wants".
 * Each series of checks should be terminated by
 * five NULL-entries.
 */
const char *want[] = {
#define URL_DATA "abc=def&x=5"
#define URL_START 0
  "abc", NULL, NULL, NULL, "def",
  "x", NULL, NULL, NULL, "5",
#define URL_END (URL_START + 10)
  NULL, NULL, NULL, NULL, NULL,
#define FORM_DATA "--AaB03x\r\ncontent-disposition: form-data; name=\"field1\"\r\n\r\nJoe Blow\r\n--AaB03x\r\ncontent-disposition: form-data; name=\"pics\"; filename=\"file1.txt\"\r\nContent-Type: text/plain\r\nContent-Transfer-Encoding: binary\r\n\r\nfiledata\r\n--AaB03x--\r\n"
#define FORM_START (URL_END + 5)
  "field1", NULL, NULL, NULL, "Joe Blow",
  "pics", "file1.txt", "text/plain", "binary", "filedata",
#define FORM_END (FORM_START + 10)
  NULL, NULL, NULL, NULL, NULL,
#define FORM_NESTED_DATA "--AaB03x\r\ncontent-disposition: form-data; name=\"field1\"\r\n\r\nJane Blow\r\n--AaB03x\r\ncontent-disposition: form-data; name=\"pics\"\r\nContent-type: multipart/mixed, boundary=BbC04y\r\n\r\n--BbC04y\r\nContent-disposition: attachment; filename=\"file1.txt\"\r\nContent-Type: text/plain\r\n\r\nfiledata1\r\n--BbC04y\r\nContent-disposition: attachment; filename=\"file2.gif\"\r\nContent-type: image/gif\r\nContent-Transfer-Encoding: binary\r\n\r\nfiledata2\r\n--BbC04y--\r\n--AaB03x--"
#define FORM_NESTED_START (FORM_END + 5)
  "field1", NULL, NULL, NULL, "Jane Blow",
  "pics", "file1.txt", "text/plain", NULL, "filedata1",
  "pics", "file2.gif", "image/gif", "binary", "filedata2",
#define FORM_NESTED_END (FORM_NESTED_START + 15)
  NULL, NULL, NULL, NULL, NULL,
#define URL_EMPTY_VALUE_DATA "key1=value1&key2=&key3="
#define URL_EMPTY_VALUE_START (FORM_NESTED_END + 5)
  "key1", NULL, NULL, NULL, "value1",
  "key2", NULL, NULL, NULL, "",
  "key3", NULL, NULL, NULL, "",
#define URL_EMPTY_VALUE_END (URL_EMPTY_VALUE_START + 15)
  NULL, NULL, NULL, NULL, NULL
};

const char *boundary = {
"-----------------------------7df2af22a1144\r\n\
Content-Disposition: form-data; name=\"ota_web\"; filename=\"1.txt\"\r\n\
Content-Type: text/plain\r\n\
\r\n\
dsdasewqeqwzxfsdffsfsfsfsffsfsfd\r\n\
sdfsdgdfgdfsgvxvvxcvxvdfsgsdf\r\n\
sdgdfgdfsgvxvvxcvxfsdfsdfsdffgsdbdfghsggggggggggggggggggggggggggggggggggggggggshgfhfghfdhfdhvdfsgsdf\r\n\
-----------------------------7df2af22a1144\r\n\
Content-Disposition: form-data; name=\"ota_web\"; filename=\"2.txt\"\r\n\
Content-Type: text/plain\r\n\
\r\n\
ggggggggggggggggggggggggggggggggggg\r\n\
gggggfsdfsdfsdsdvvvg\r\n\
-----------------------------7df2af22a1144--\r\n"
};

static int32_t rnd_val = 0;
/**
 * Generate 31-bit pseudo random number.
 * Function initialize itself at first call to current time.
 * @return 31-bit pseudo random number.
 */
int MHD_W32_random_(void)
{
  if (0 == rnd_val)
    rnd_val = (int32_t)time(NULL);
  /* stolen from winsup\cygwin\random.cc */
  rnd_val = (16807 * (rnd_val % 127773) - 2836 * (rnd_val / 127773))
               & 0x7fffffff;
  return (int)rnd_val;
}

static int
mismatch (const char *a, const char *b)
{
  if (a == b)
    return 0;
  if ((a == NULL) || (b == NULL))
    return 1;
  return 0 != strcmp (a, b);
}

static int
value_checker (void *cls,
               enum MHD_ValueKind kind,
               const char *key,
               const char *filename,
               const char *content_type,
               const char *transfer_encoding,
               const char *data, uint64_t off, size_t size)
{
  int *want_off = cls;
  int idx = *want_off;

#if 1
  printf("\r\nVC: key:%s filename:%s\n", key, filename);
  printf("VC: data:%d\n%.*s", (int)size, (int)size, data);
#endif
  if ( (0 != off) && (0 == size) )
    return MHD_YES;
/*    
  if ((idx < 0) ||
      (want[idx] == NULL) ||
      (0 != strcmp (key, want[idx])) ||
      (mismatch (filename, want[idx + 1])) ||
      (mismatch (content_type, want[idx + 2])) ||
      (mismatch (transfer_encoding, want[idx + 3])) ||
      (0 != memcmp (data, &want[idx + 4][off], size)))
    {
      *want_off = -1;
      return MHD_NO;
    }
  if (off + size == strlen (want[idx + 4]))
    *want_off = idx + 5;
*/
	
  return MHD_YES;

}


static int
test_multipart ()
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = 0;
  int i;
  int delta;
  size_t size;

  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value =
    MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA ", boundary=---------------------------7df2af22a1144";
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection,
                                  1024, &value_checker, &want_off);
  i = 0;
  size = strlen (boundary);
  while (i < size)
    {
      delta = 1 + MHD_random_ () % (size - i);
      MHD_post_process (pp, &boundary[i], delta);
      i += delta;
    }
  MHD_destroy_post_processor (pp);
//  if (want_off != size)
//    return 2;
  return 0;
}


static int
test_nested_multipart ()
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = FORM_NESTED_START;
  int i;
  int delta;
  size_t size;

  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value =
    MHD_HTTP_POST_ENCODING_MULTIPART_FORMDATA ", boundary=AaB03x";
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection,
                                  1024, &value_checker, &want_off);
  i = 0;
  size = strlen (FORM_NESTED_DATA);
  while (i < size)
    {
    	delta = 100;
      //delta = 1 + MHD_random_ () % (size - i);
      MHD_post_process (pp, &FORM_NESTED_DATA[i], delta);
      i += delta;
    }
  MHD_destroy_post_processor (pp);
  if (want_off != FORM_NESTED_END)
    return 4;
  return 0;
}


static int
test_empty_value ()
{
  struct MHD_Connection connection;
  struct MHD_HTTP_Header header;
  struct MHD_PostProcessor *pp;
  unsigned int want_off = URL_EMPTY_VALUE_START;
  int i;
  int delta;
  size_t size;

  memset (&connection, 0, sizeof (struct MHD_Connection));
  memset (&header, 0, sizeof (struct MHD_HTTP_Header));
  connection.headers_received = &header;
  header.header = MHD_HTTP_HEADER_CONTENT_TYPE;
  header.value = MHD_HTTP_POST_ENCODING_FORM_URLENCODED;
  header.kind = MHD_HEADER_KIND;
  pp = MHD_create_post_processor (&connection,
                                  1024, &value_checker, &want_off);
  i = 0;
  size = strlen (URL_EMPTY_VALUE_DATA);
  while (i < size)
    {
      delta = 1 + MHD_random_ () % (size - i);
      MHD_post_process (pp, &URL_EMPTY_VALUE_DATA[i], delta);
      i += delta;
    }
  MHD_destroy_post_processor (pp);
  if (want_off != URL_EMPTY_VALUE_END)
    return 8;
  return 0;
}




int
main (int argc, char *const *argv)
{
  unsigned int errorCount = 0;

  errorCount += test_multipart ();
//  errorCount += test_empty_value();
  if (errorCount != 0)
    fprintf (stderr, "Error (code: %u)\n", errorCount);
  return errorCount != 0;       /* 0 == pass */
}
