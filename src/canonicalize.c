/* readlink wrapper to return the link name in malloc'd storage.
   Unlike xreadlink and xreadlink_with_size, don't ever call exit.

   Copyright (C) 2001, 2003-2007, 2009-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering <jim@meyering.net>  */

//#include <config.h>

//#include "areadlink.h"

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef SSIZE_MAX
# define SSIZE_MAX ((ssize_t) (SIZE_MAX / 2))
#endif

/* SYMLINK_MAX is used only for an initial memory-allocation sanity
   check, so it's OK to guess too small on hosts where there is no
   arbitrary limit to symbolic link length.  */
#ifndef SYMLINK_MAX
# define SYMLINK_MAX 1024
#endif

#define MAXSIZE (SIZE_MAX < SSIZE_MAX ? SIZE_MAX : SSIZE_MAX)

/* Call readlink to get the symbolic link value of FILE.
   SIZE is a hint as to how long the link is expected to be;
   typically it is taken from st_size.  It need not be correct.
   Return a pointer to that NUL-terminated string in malloc'd storage.
   If readlink fails, malloc fails, or if the link value is longer
   than SSIZE_MAX, return NULL (caller may use errno to diagnose).  */

char *
areadlink_with_size (char const *file, size_t size)
{
  /* Some buggy file systems report garbage in st_size.  Defend
     against them by ignoring outlandish st_size values in the initial
     memory allocation.  */
  size_t symlink_max = SYMLINK_MAX;
  size_t INITIAL_LIMIT_BOUND = 8 * 1024;
  size_t initial_limit = (symlink_max < INITIAL_LIMIT_BOUND
                          ? symlink_max + 1
                          : INITIAL_LIMIT_BOUND);

  /* The initial buffer size for the link value.  */
  size_t buf_size = size < initial_limit ? size + 1 : initial_limit;

  while (1)
    {
      ssize_t r;
      size_t link_length;
      char *buffer = malloc (buf_size);

      if (buffer == NULL)
        return NULL;
      r = readlink (file, buffer, buf_size);
      link_length = r;

      /* On AIX 5L v5.3 and HP-UX 11i v2 04/09, readlink returns -1
         with errno == ERANGE if the buffer is too small.  */
      if (r < 0 && errno != ERANGE)
        {
          int saved_errno = errno;
          free (buffer);
          errno = saved_errno;
          return NULL;
        }

      if (link_length < buf_size)
        {
          buffer[link_length] = 0;
          return buffer;
        }

      free (buffer);
      if (buf_size <= MAXSIZE / 2)
        buf_size *= 2;
      else if (buf_size < MAXSIZE)
        buf_size = MAXSIZE;
      else
        {
          errno = ENOMEM;
          return NULL;
        }
    }
}


/* Return the canonical absolute name of a given file.
   Copyright (C) 1996-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

//#include <config.h>

#include "canonicalize.h"

#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

//#include "areadlink.h"
//#include "file-set.h"
//#include "hash-triple.h"
//#include "pathmax.h"
//#include "xalloc.h"
//#include "getcwd.h"
//#include "dosname.h"

#define MULTIPLE_BITS_SET(i) (((i) & ((i) - 1)) != 0)

/* In this file, we cannot handle file names longer than PATH_MAX.
   On systems with no file name length limit, use a fallback.  */
#ifndef PATH_MAX
# define PATH_MAX 8192
#endif

#ifndef DOUBLE_SLASH_IS_DISTINCT_ROOT
# define DOUBLE_SLASH_IS_DISTINCT_ROOT 0
#endif

#if ISSLASH ('\\')
# define SLASHES "/\\"
#else
# define SLASHES "/"
#endif

#if !((HAVE_CANONICALIZE_FILE_NAME && FUNC_REALPATH_WORKS)      \
      || GNULIB_CANONICALIZE_LGPL)
/* Return the canonical absolute name of file NAME.  A canonical name
   does not contain any ".", ".." components nor any repeated file name
   separators ('/') or symlinks.  All components must exist.
   The result is malloc'd.  */

char *
canonicalize_file_name (const char *name)
{
  return canonicalize_filename_mode (name, CAN_EXISTING);
}
#endif /* !HAVE_CANONICALIZE_FILE_NAME */

/* Return true if we've already seen the triple, <FILENAME, dev, ino>.
   If *HT is not initialized, initialize it.  */
//static bool
//seen_triple (Hash_table **ht, char const *filename, struct stat const *st)
//{
//  if (*ht == NULL)
//    {
//      size_t initial_capacity = 7;
//      *ht = hash_initialize (initial_capacity,
//                            NULL,
//                            triple_hash,
//                            triple_compare_ino_str,
//                            triple_free);
//      if (*ht == NULL)
//        xalloc_die ();
//    }
//
//  if (seen_file (*ht, filename, st))
//    return true;
//
//  record_file (*ht, filename, st);
//  return false;
//}

/* Return the canonical absolute name of file NAME, while treating
   missing elements according to CAN_MODE.  A canonical name
   does not contain any ".", ".." components nor any repeated file name
   separators ('/') or, depending on other CAN_MODE flags, symlinks.
   Whether components must exist or not depends on canonicalize mode.
   The result is malloc'd.  */

char *
canonicalize_filename_mode (const char *name, canonicalize_mode_t can_mode)
{
  char *rname, *dest, *extra_buf = NULL;
  char const *start;
  char const *end;
  char const *rname_limit;
  size_t extra_len = 0;
  //Hash_table *ht = NULL;
  int saved_errno;
  int can_flags = can_mode & ~CAN_MODE_MASK;
  bool logical = can_flags & CAN_NOLINKS;
  size_t prefix_len;

  can_mode &= CAN_MODE_MASK;

  if (MULTIPLE_BITS_SET (can_mode))
    {
      errno = EINVAL;
      return NULL;
    }

  if (name == NULL)
    {
      errno = EINVAL;
      return NULL;
    }

  if (name[0] == '\0')
    {
      errno = ENOENT;
      return NULL;
    }

  /* This is always zero for Posix hosts, but can be 2 for MS-Windows
     and MS-DOS X:/foo/bar file names.  */
  prefix_len = FILE_SYSTEM_PREFIX_LEN (name);

  if (!IS_ABSOLUTE_FILE_NAME (name))
    {
      rname = getcwd (NULL, 0);
      if (!rname)
        return NULL;
      dest = strchr (rname, '\0');
      if (dest - rname < PATH_MAX)
        {
          char *p = realloc (rname, PATH_MAX);
          dest = p + (dest - rname);
          rname = p;
          rname_limit = rname + PATH_MAX;
        }
      else
        {
          rname_limit = dest;
        }
      start = name;
      prefix_len = FILE_SYSTEM_PREFIX_LEN (rname);
    }
  else
    {
      rname = malloc (PATH_MAX);
      rname_limit = rname + PATH_MAX;
      dest = rname;
      if (prefix_len)
        {
          memcpy (rname, name, prefix_len);
          dest += prefix_len;
        }
      *dest++ = '/';
      if (DOUBLE_SLASH_IS_DISTINCT_ROOT)
        {
          if (ISSLASH (name[1]) && !ISSLASH (name[2]) && !prefix_len)
            *dest++ = '/';
          *dest = '\0';
        }
      start = name + prefix_len;
    }

  for ( ; *start; start = end)
    {
      /* Skip sequence of multiple file name separators.  */
      while (ISSLASH (*start))
        ++start;

      /* Find end of component.  */
      for (end = start; *end && !ISSLASH (*end); ++end)
        /* Nothing.  */;

      if (end - start == 0)
        break;
      else if (end - start == 1 && start[0] == '.')
        /* nothing */;
      else if (end - start == 2 && start[0] == '.' && start[1] == '.')
        {
          /* Back up to previous component, ignore if at root already.  */
          if (dest > rname + prefix_len + 1)
            for (--dest; dest > rname && !ISSLASH (dest[-1]); --dest)
              continue;
          if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rname + 1
              && !prefix_len && ISSLASH (*dest) && !ISSLASH (dest[1]))
            dest++;
        }
      else
        {
          struct stat st;

          if (!ISSLASH (dest[-1]))
            *dest++ = '/';

          if (dest + (end - start) >= rname_limit)
            {
              ptrdiff_t dest_offset = dest - rname;
              size_t new_size = rname_limit - rname;

              if (end - start + 1 > PATH_MAX)
                new_size += end - start + 1;
              else
                new_size += PATH_MAX;
              rname = realloc (rname, new_size);
              rname_limit = rname + new_size;

              dest = rname + dest_offset;
            }

          dest = memcpy (dest, start, end - start);
          dest += end - start;
          *dest = '\0';

          if (logical && (can_mode == CAN_MISSING))
            {
              /* Avoid the stat in this case as it's inconsequential.
                 i.e. we're neither resolving symlinks or testing
                 component existence.  */
              st.st_mode = 0;
            }
          else if ((logical ? stat (rname, &st) : lstat (rname, &st)) != 0)
            {
              saved_errno = errno;
              if (can_mode == CAN_EXISTING)
                goto error;
              if (can_mode == CAN_ALL_BUT_LAST)
                {
                  if (end[strspn (end, SLASHES)] || saved_errno != ENOENT)
                    goto error;
                  continue;
                }
              st.st_mode = 0;
            }

          if (S_ISLNK (st.st_mode))
            {
              char *buf;
              size_t n, len;

              /* Detect loops.  We cannot use the cycle-check module here,
                 since it's actually possible to encounter the same symlink
                 more than once in a given traversal.  However, encountering
                 the same symlink,NAME pair twice does indicate a loop.  */
              //if (seen_triple (&ht, name, &st))
              //  {
              //    if (can_mode == CAN_MISSING)
              //      continue;
              //    saved_errno = ELOOP;
              //    goto error;
              //  }

              buf = areadlink_with_size (rname, st.st_size);
              if (!buf)
                {
                  if (can_mode == CAN_MISSING && errno != ENOMEM)
                    continue;
                  saved_errno = errno;
                  goto error;
                }

              n = strlen (buf);
              len = strlen (end);

              if (!extra_len)
                {
                  extra_len =
                    ((n + len + 1) > PATH_MAX) ? (n + len + 1) : PATH_MAX;
                  extra_buf = malloc (extra_len);
                }
              else if ((n + len + 1) > extra_len)
                {
                  extra_len = n + len + 1;
                  extra_buf = realloc (extra_buf, extra_len);
                }

              /* Careful here, end may be a pointer into extra_buf... */
              memmove (&extra_buf[n], end, len + 1);
              name = end = memcpy (extra_buf, buf, n);

              if (IS_ABSOLUTE_FILE_NAME (buf))
                {
                  size_t pfxlen = FILE_SYSTEM_PREFIX_LEN (buf);

                  if (pfxlen)
                    memcpy (rname, buf, pfxlen);
                  dest = rname + pfxlen;
                  *dest++ = '/'; /* It's an absolute symlink */
                  if (DOUBLE_SLASH_IS_DISTINCT_ROOT)
                    {
                      if (ISSLASH (buf[1]) && !ISSLASH (buf[2]) && !pfxlen)
                        *dest++ = '/';
                      *dest = '\0';
                    }
                  /* Install the new prefix to be in effect hereafter.  */
                  prefix_len = pfxlen;
                }
              else
                {
                  /* Back up to previous component, ignore if at root
                     already: */
                  if (dest > rname + prefix_len + 1)
                    for (--dest; dest > rname && !ISSLASH (dest[-1]); --dest)
                      continue;
                  if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rname + 1
                      && ISSLASH (*dest) && !ISSLASH (dest[1]) && !prefix_len)
                    dest++;
                }

              free (buf);
            }
          else
            {
              if (!S_ISDIR (st.st_mode) && *end && (can_mode != CAN_MISSING))
                {
                  saved_errno = ENOTDIR;
                  goto error;
                }
            }
        }
    }
  if (dest > rname + prefix_len + 1 && ISSLASH (dest[-1]))
    --dest;
  if (DOUBLE_SLASH_IS_DISTINCT_ROOT && dest == rname + 1 && !prefix_len
      && ISSLASH (*dest) && !ISSLASH (dest[1]))
    dest++;
  *dest = '\0';
  if (rname_limit != dest + 1)
    rname = realloc (rname, dest - rname + 1);

  free (extra_buf);
  //if (ht)
  //  hash_free (ht);
  return rname;

error:
  free (extra_buf);
  free (rname);
  //if (ht)
  //  hash_free (ht);
  errno = saved_errno;
  return NULL;
}
