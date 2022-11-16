/*
  Teem: Tools to process and visualize scientific data and images             .
  Copyright (C) 2013, 2012, 2011, 2010, 2009  University of Chicago
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "unrrdu.h"
#include "privateUnrrdu.h"

#include <ctype.h>

const int
unrrduPresent = 42;

const char *
unrrduBiffKey = "unrrdu";

/* number of columns that hest will used */
unsigned int
unrrduDefNumColumns = 78;

/*
******** unrrduCmdList[]
**
** NULL-terminated array of unrrduCmd pointers, as ordered by UNRRDU_MAP macro
*/
unrrduCmd *
unrrduCmdList[] = {
  UNRRDU_MAP(UNRRDU_LIST)
  NULL
};

/*
******** unrrduCmdMain
**
** A "main" function for unu-like programs, which is very similar to
** teem/src/bin/unu.c:main(), and
** teem/src/bin/tend.c:main(), and
** teem/src/limn/test/lpu.c:main().
** With more time (and a major Teem release), this function may change,
** and those programs may use this.
**
** A sneaky but basic issue is the const-correctness of how the hestParm
** is used; we'd like to take a const hestParm* to communicate parameters
** the caller has set, but the show-stopper is that unrrduCmd->main()
** takes a non-const hestParm, and it has to be that way, because some
** unu commands alter the given hparm (which probably shouldn't happen).
** Until that's fixed, we have a non-const hestParm* coming in here.
*/
int
unrrduCmdMain(int argc, const char **argv,
              const char *cmd, const char *title,
              const unrrduCmd *const *cmdList,
              hestParm *_hparm, FILE *fusage) {
  int i, ret;
  const char *me;
  char *argv0 = NULL;
  hestParm *hparm;
  airArray *mop;

  me = argv[0];

  /* parse environment variables first, in case they break nrrdDefault*
     or nrrdState* variables in a way that nrrdSanity() should see */
  nrrdDefaultGetenv();
  nrrdStateGetenv();

  /* unu does some unu-specific environment-variable handling here */

  nrrdSanityOrDie(me);

  mop = airMopNew();
  if (_hparm) {
    hparm = _hparm;
  } else {
    hparm = hestParmNew();
    airMopAdd(mop, hparm, (airMopper)hestParmFree, airMopAlways);
    hparm->elideSingleEnumType = AIR_TRUE;
    hparm->elideSingleOtherType = AIR_TRUE;
    hparm->elideSingleOtherDefault = AIR_FALSE;
    hparm->elideSingleNonExistFloatDefault = AIR_TRUE;
    hparm->elideMultipleNonExistFloatDefault = AIR_TRUE;
    hparm->elideSingleEmptyStringDefault = AIR_TRUE;
    hparm->elideMultipleEmptyStringDefault = AIR_TRUE;
    hparm->cleverPluralizeOtherY = AIR_TRUE;
    /* learning columns from current window; if ioctl is available
    if (1) {
      struct winsize ws;
      ioctl(1, TIOCGWINSZ, &ws);
      hparm->columns = ws.ws_col - 1;
    }
    */
    hparm->columns = 78;
  }

  /* if there are no arguments, then we give general usage information */
  if (1 >= argc) {
    /* this is like unrrduUsageUnu() */
    unsigned int ii, maxlen = 0;
    char *buff, *fmt, tdash[] = "--- %s ---";
    for (ii=0; cmdList[ii]; ii++) {
      if (cmdList[ii]->hidden) {
        continue;
      }
      maxlen = AIR_MAX(maxlen, AIR_UINT(strlen(cmdList[ii]->name)));
    }
    if (!maxlen) {
      fprintf(fusage, "%s: problem: maxlen = %u\n", me, maxlen);
      airMopError(mop); return 1;
    }
    buff = AIR_CALLOC(strlen(tdash) + strlen(title) + 1, char);
    airMopAdd(mop, buff, airFree, airMopAlways);
    sprintf(buff, tdash, title);
    fmt = AIR_CALLOC(hparm->columns + strlen(buff) + 1, char); /* generous */
    airMopAdd(mop, buff, airFree, airMopAlways);
    sprintf(fmt, "%%%us\n",
            AIR_UINT((hparm->columns-strlen(buff))/2 + strlen(buff) - 1));
    fprintf(fusage, fmt, buff);

    for (ii=0; cmdList[ii]; ii++) {
      unsigned int cc, len;
      if (cmdList[ii]->hidden) {
        continue;
      }
      len = AIR_UINT(strlen(cmdList[ii]->name));
      strcpy(buff, "");
      for (cc=len; cc<maxlen; cc++)
        strcat(buff, " ");
      strcat(buff, cmd);
      strcat(buff, " ");
      strcat(buff, cmdList[ii]->name);
      strcat(buff, " ... ");
      len = strlen(buff);
      fprintf(fusage, "%s", buff);
      _hestPrintStr(fusage, len, len, hparm->columns,
                    cmdList[ii]->info, AIR_FALSE);
    }
    airMopError(mop);
    return 1;
  }
  /* else, we see if its --version */
  if (!strcmp("--version", argv[1])) {
    char vbuff[AIR_STRLEN_LARGE];
    airTeemVersionSprint(vbuff);
    printf("%s\n", vbuff);
    exit(0);
  }
  /* else, we should see if they're asking for a command we know about */
  for (i=0; cmdList[i]; i++) {
    if (!strcmp(argv[1], cmdList[i]->name)) {
      break;
    }
    /* if user typed "prog --help" we treat it as "prog about",
       but only if there is an "about" command */
    if (!strcmp("--help", argv[1])
        && !strcmp("about", cmdList[i]->name)) {
      break;
    }
  }
  if (cmdList[i]) {
    /* yes, we have that command */
    /* initialize variables used by the various commands */
    argv0 = AIR_CALLOC(strlen(cmd) + strlen(argv[1]) + 2, char);

    airMopMem(mop, &argv0, airMopAlways);
    sprintf(argv0, "%s %s", cmd, argv[1]);

    /* run the individual command, saving its exit status */
    ret = cmdList[i]->main(argc-2, argv+2, argv0, hparm);
  } else {
    fprintf(stderr, "%s: unrecognized command: \"%s\"; type \"%s\" for "
            "complete list\n", me, argv[1], me);
    ret = 1;
  }

  airMopDone(mop, ret);
  return ret;
}

/*
******** unrrduUsageUnu
**
** prints out a little banner, and a listing of all available commands
** with their one-line descriptions
*/
void
unrrduUsageUnu(const char *me, hestParm *hparm) {
  char buff[AIR_STRLEN_LARGE], fmt[AIR_STRLEN_LARGE];
  unsigned int cmdi, chi, len, maxlen;

  maxlen = 0;
  for (cmdi=0; unrrduCmdList[cmdi]; cmdi++) {
    maxlen = AIR_MAX(maxlen, AIR_UINT(strlen(unrrduCmdList[cmdi]->name)));
  }

  sprintf(buff, "--- unu: Utah Nrrd Utilities command-line interface ---");
  len = AIR_UINT(strlen(buff));
  sprintf(fmt, "%%%us\n", (hparm->columns > len
                           ? hparm->columns-len
                           : 0)/2 + len - 1);
  fprintf(stdout, fmt, buff);
  for (cmdi=0; unrrduCmdList[cmdi]; cmdi++) {
    int nofft;
    if (unrrduCmdList[cmdi]->hidden) {
      /* nothing to see here! */
      continue;
    }
    nofft = !strcmp(unrrduCmdList[cmdi]->name, "fft") && !nrrdFFTWEnabled;
    len = AIR_UINT(strlen(unrrduCmdList[cmdi]->name));
    len += !!nofft;
    strcpy(buff, "");
    for (chi=len; chi<maxlen; chi++)
      strcat(buff, " ");
    if (nofft) {
      strcat(buff, "(");
    }
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, unrrduCmdList[cmdi]->name);
    strcat(buff, " ... ");
    len = AIR_UINT(strlen(buff));
    fprintf(stdout, "%s", buff);
    if (nofft) {
      char *infop;
      /* luckily, still fits within 80 columns */
      fprintf(stdout, "Not Enabled: ");
      infop = AIR_CALLOC(strlen(unrrduCmdList[cmdi]->info) + 2, char);
      sprintf(infop, "%s)", unrrduCmdList[cmdi]->info);
      _hestPrintStr(stdout, len, len, hparm->columns,
                    infop, AIR_FALSE);
      free(infop);
    } else {
      _hestPrintStr(stdout, len, len, hparm->columns,
                    unrrduCmdList[cmdi]->info, AIR_FALSE);
    }
  }
  return;
}

/*
******** unrrduUsage
**
** A generic version of the usage command, which can be used by other
** programs that are leveraging the unrrduCmd infrastructure.
**
** does not use biff
*/
int
unrrduUsage(const char *me, hestParm *hparm,
            const char *title, unrrduCmd **cmdList) {
  char buff[AIR_STRLEN_LARGE], fmt[AIR_STRLEN_LARGE];
  unsigned int cmdi, chi, len, maxlen;

  if (!(title && cmdList)) {
    /* got NULL pointer */
    return 1;
  }
  maxlen = 0;
  for (cmdi=0; cmdList[cmdi]; cmdi++) {
    maxlen = AIR_MAX(maxlen, AIR_UINT(strlen(cmdList[cmdi]->name)));
  }

  sprintf(buff, "--- %s ---", title);
  len = AIR_UINT(strlen(buff));
  sprintf(fmt, "%%%us\n", (hparm->columns > len
                           ? hparm->columns-len
                           : 0)/2 + len - 1);
  fprintf(stdout, fmt, buff);
  for (cmdi=0; cmdList[cmdi]; cmdi++) {
    len = AIR_UINT(strlen(cmdList[cmdi]->name));
    strcpy(buff, "");
    for (chi=len; chi<maxlen; chi++)
      strcat(buff, " ");
    strcat(buff, me);
    strcat(buff, " ");
    strcat(buff, cmdList[cmdi]->name);
    strcat(buff, " ... ");
    len = AIR_UINT(strlen(buff));
    fprintf(stdout, "%s", buff);
    _hestPrintStr(stdout, len, len, hparm->columns,
                  cmdList[cmdi]->info, AIR_FALSE);
  }
  return 0;
}

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
/* --------------------------------------------------------- */

/*
******** unrrduHestPosCB
**
** For parsing position along an axis. Can be a simple (long) integer,
** or M to signify last position along axis (#samples-1), or
** M+<int> or M-<int> to signify some position relative to the end.
**
** It can also be m+<int> to signify some position relative to some
** "minimum", assuming that a minimum position is being specified
** at the same time as this one.  Obviously, there has to be some
** error handling to make sure that no one is trying to define a
** minimum position with respect to itself.  And, the ability to
** specify a position as "m+<int>" shouldn't be advertised in situations
** (unu slice) where you only have one position, rather than an interval
** between two positions (unu crop and unu pad).
**
** This information is represented with two integers, pos[0] and pos[1]:
** pos[0] ==  0: pos[1] gives the absolute position
** pos[0] ==  1: pos[1] gives the position relative to the last index
** pos[0] == -1: pos[1] gives the position relative to a "minimum" position
*/
int
unrrduParsePos(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParsePos";
  long int *pos;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  pos = (long int*)ptr;
  if (!strcmp("M", str)) {
    pos[0] = 1;
    pos[1] = 0;
    return 0;
  }
  if ('M' == str[0]) {
    if (!( '-' == str[1] || '+' == str[1] )) {
      sprintf(err, "%s: \'M\' can be followed only by \'+\' or \'-\'", me);
      return 1;
    }
    pos[0] = 1;
    if (1 != sscanf(str+1, "%ld", &(pos[1]))) {
      sprintf(err, "%s: can't parse \"%s\" as M+<int> or M-<int>", me, str);
      return 1;
    }
    return 0;
  }
  if ('m' == str[0]) {
    if ('+' != str[1]) {
      sprintf(err, "%s: \'m\' can only be followed by \'+\'", me);
      return 1;
    }
    pos[0] = -1;
    if (1 != sscanf(str+1, "%ld", &(pos[1]))) {
      sprintf(err, "%s: can't parse \"%s\" as m+<int>", me, str);
      return 1;
    }
    if (pos[1] < 0 ) {
      sprintf(err, "%s: int in m+<int> must be non-negative (not %ld)",
              me, pos[1]);
      return 1;
    }
    return 0;
  }
  /* else its just a plain unadorned integer */
  pos[0] = 0;
  if (1 != sscanf(str, "%ld", &(pos[1]))) {
    sprintf(err, "%s: can't parse \"%s\" as int", me, str);
    return 1;
  }
  return 0;
}

hestCB unrrduHestPosCB = {
  2*sizeof(long int),
  "position",
  unrrduParsePos,
  NULL
};

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
/* --------------------------------------------------------- */

/*
******** unrrduHestMaybeTypeCB
**
** although nrrdType is an airEnum that hest already knows how
** to parse, we want the ability to have "unknown" be a valid
** parsable value, contrary to how airEnums usually work with hest.
** For instance, we might want to use "unknown" to represent
** "same type as the input, whatever that is".
**
** 18 July 03: with new nrrdTypeDefault, this function becomes
** less of a hack, and more necessary, because the notion of an
** unknown but valid type (as a default type is) falls squarely
** outside the nrrdType airEnum framework.  Added a separate test
** for "default", even though currently nrrdTypeUnknown is the same
** value as nrrdTypeDefault.
*/
int
unrrduParseMaybeType(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseMaybeType";
  int *typeP;

  /* fprintf(stderr, "!%s: str = \"%s\"\n", me, str); */
  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  typeP = (int*)ptr;
  if (!strcmp("unknown", str)) {
    *typeP = nrrdTypeUnknown;
  } else if (!strcmp("default", str)) {
    *typeP = nrrdTypeDefault;
  } else {
    *typeP = airEnumVal(nrrdType, str);
    if (nrrdTypeUnknown == *typeP) {
      sprintf(err, "%s: can't parse \"%s\" as type", me, str);
      return 1;
    }
  }
  /* fprintf(stderr, "!%s: *typeP = %d\n", me, *typeP); */
  return 0;
}

hestCB unrrduHestMaybeTypeCB = {
  sizeof(int),
  "type",
  unrrduParseMaybeType,
  NULL
};

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
/* --------------------------------------------------------- */

/*
******** unrrduHestBitsCB
**
** for parsing an int that can be 8, 16, or 32
*/
int
unrrduParseBits(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseBits";
  unsigned int *bitsP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  bitsP = (unsigned int*)ptr;
  if (1 != sscanf(str, "%u", bitsP)) {
    sprintf(err, "%s: can't parse \"%s\" as int", me, str);
    return 1;
  }
  if (!( 8 == *bitsP || 16 == *bitsP || 32 == *bitsP )) {
    sprintf(err, "%s: bits (%d) not 8, 16, or 32", me, *bitsP);
    return 1;
  }
  return 0;
}

hestCB unrrduHestBitsCB = {
  sizeof(int),
  "quantization bits",
  unrrduParseBits,
  NULL
};

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
/* --------------------------------------------------------- */

/*
******** unrrduParseScale
**
** parse the strings used with "unu resample -s" to indicate
** the new number of samples.
** =         : unrrduScaleNothing
** a         : unrrduScaleAspectRatio
** x<float>  : unrrduScaleMultiply
** x=<float> : unrrduScaleMultiply
** /<float>  : unrrduScaleDivide
** /=<float> : unrrduScaleDivide
** +=<uint>  : unrrduScaleAdd
** -=<uint>  : unrrduScaleSubstract
** <uint>    : unrrduScaleExact
*/
int
unrrduParseScale(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseScale";
  double *scale;
  unsigned int num;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  scale = AIR_CAST(double *, ptr);
  if (!strcmp("=", str)) {
    scale[0] = AIR_CAST(double, unrrduScaleNothing);
    scale[1] = 0.0;
  } else if (!strcmp("a", str)) {
    scale[0] = AIR_CAST(double, unrrduScaleAspectRatio);
    scale[1] = 0.0;
  } else if (strlen(str) > 2
      && ('x' == str[0] || '/' == str[0])
      && '=' == str[1]) {
    if (1 != sscanf(str+2, "%lf", scale+1)) {
      sprintf(err, "%s: can't parse \"%s\" as x=<float> or /=<float>",
              me, str);
      return 1;
    }
    scale[0] = AIR_CAST(double, ('x' == str[0]
                                 ? unrrduScaleMultiply
                                 : unrrduScaleDivide));
  } else if (strlen(str) > 1
             && ('x' == str[0] || '/' == str[0])) {
    if (1 != sscanf(str+1, "%lf", scale+1)) {
      sprintf(err, "%s: can't parse \"%s\" as x<float> or /<float>",
              me, str);
      return 1;
    }
    scale[0] = AIR_CAST(double, ('x' == str[0]
                                 ? unrrduScaleMultiply
                                 : unrrduScaleDivide));
  } else if (strlen(str) > 2
             && ('+' == str[0] || '-' == str[0])
             && '=' == str[1]) {
    if (1 != sscanf(str+2, "%u", &num)) {
      sprintf(err, "%s: can't parse \"%s\" as +=<uint> or -=<uint>",
              me, str);
      return 1;
    }
    scale[0] = AIR_CAST(double, ('+' == str[0]
                                 ? unrrduScaleAdd
                                 : unrrduScaleSubtract));
    scale[1] = AIR_CAST(double, num);
  } else {
    if (1 != sscanf(str, "%u", &num)) {
      sprintf(err, "%s: can't parse \"%s\" as uint", me, str);
      return 1;
    }
    scale[0] = AIR_CAST(double, unrrduScaleExact);
    scale[1] = AIR_CAST(double, num);
  }
  return 0;
}

hestCB unrrduHestScaleCB = {
  2*sizeof(double),
  "sampling specification",
  unrrduParseScale,
  NULL
};

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
/* --------------------------------------------------------- */

/*
******** unrrduHestFileCB
**
** for parsing a filename, which means opening it in "rb" mode and
** getting a FILE *.  "-" is interpreted as stdin, which is not
** fclose()ed at the end, unlike all other files.
*/
void *
unrrduMaybeFclose(void *_file) {
  FILE *file;

  file = (FILE *)_file;
  if (stdin != file) {
    file = airFclose(file);
  }
  return NULL;
}

int
unrrduParseFile(void *ptr, char *str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseFile";
  FILE **fileP;

  if (!(ptr && str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  fileP = (FILE **)ptr;
  if (!( *fileP = airFopen(str, stdin, "rb") )) {
    sprintf(err, "%s: fopen(\"%s\",\"rb\") failed: %s",
            me, str, strerror(errno));
    return 1;
  }
  return 0;
}

hestCB unrrduHestFileCB = {
  sizeof(FILE *),
  "filename",
  unrrduParseFile,
  unrrduMaybeFclose,
};

/* --------------------------------------------------------- */
/* --------------------------------------------------------- */
/* --------------------------------------------------------- */

/*
******** unrrduHestEncodingCB
**
** for parsing output encoding, including compression flags
** enc[0]: which encoding, from nrrdEncodingType* enum
** enc[1]: for compressions: zlib "level" and bzip2 "blocksize"
** enc[2]: for zlib: strategy, from nrrdZlibStrategy* enum
*/
int
unrrduParseEncoding(void *ptr, char *_str, char err[AIR_STRLEN_HUGE]) {
  char me[]="unrrduParseEncoding", *str, *opt;
  int *enc;
  airArray *mop;

  if (!(ptr && _str)) {
    sprintf(err, "%s: got NULL pointer", me);
    return 1;
  }
  enc = (int *)ptr;
  /* these are the defaults, they may not get over-written */
  enc[1] = -1;
  enc[2] = nrrdZlibStrategyDefault;

  enc[0] = airEnumVal(nrrdEncodingType, _str);
  if (nrrdEncodingTypeUnknown != enc[0]) {
    /* we're done; encoding was simple: "raw" or "gz" */
    return 0;
  }
  mop = airMopNew();
  str = airStrdup(_str);
  airMopMem(mop, &str, airMopAlways);
  opt = strchr(str, ':');
  if (!opt) {
    /* couldn't parse string as nrrdEncodingType, but there wasn't a colon */
    sprintf(err, "%s: didn't recognize \"%s\" as an encoding", me, str);
    airMopError(mop); return 1;
  } else {
    *opt = '\0';
    opt++;
    enc[0] = airEnumVal(nrrdEncodingType, str);
    if (nrrdEncodingTypeUnknown == enc[0]) {
      sprintf(err, "%s: didn't recognize \"%s\" as an encoding", me, str);
      airMopError(mop); return 1;
    }
    if (!nrrdEncodingArray[enc[0]]->isCompression) {
      sprintf(err, "%s: only compression encodings have parameters", me);
      airMopError(mop); return 1;
    }
    while (*opt) {
      int opti = AIR_INT(*opt);
      if (isdigit(opti)) {
        enc[1] = *opt - '0';
      } else if ('d' == tolower(opti)) {
        enc[2] = nrrdZlibStrategyDefault;
      } else if ('h' == tolower(opti)) {
        enc[2] = nrrdZlibStrategyHuffman;
      } else if ('f' == tolower(opti)) {
        enc[2] = nrrdZlibStrategyFiltered;
      } else {
        sprintf(err, "%s: parameter char \"%c\" not a digit or 'd','h','f'",
                me, *opt);
        airMopError(mop); return 1;
      }
      opt++;
    }
  }
  airMopOkay(mop);
  return 0;
}

hestCB unrrduHestEncodingCB = {
  3*sizeof(int),
  "encoding",
  unrrduParseEncoding,
  NULL
};

