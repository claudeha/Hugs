/* --------------------------------------------------------------------------
 * Implementation of the Haskell IO monad.
 *
 * The primitives below implement the standard IO monad for Haskell 1.3
 * using a continuation passing bimonad (two streams of processing for
 * `exceptional' and `normal' I/O respectively).  The primitives are
 * believed to give a reasonably good implementation of the semantics
 * specified by the Haskell 1.3 report.  There are also some additional
 * primitives, particularly for dealing with IOError and Handle values
 * that are not included in the prelude, but have been suggested for
 * inclusion in standard libraries.  I don't know what semantics and
 * specifications will be specified for these operations in the final
 * version of the 1.3 I/O specification, so be prepared for changes.
 *
 * The Hugs 98 system is Copyright (c) Mark P Jones, Alastair Reid, the
 * Yale Haskell Group, and the Oregon Graduate Institute of Science and
 * Technology, 1994-1999, All rights reserved.  It is distributed as
 * free software under the license in the file "License", which is
 * included in the distribution.
 *
 * $RCSfile: iomonad.c,v $
 * $Revision: 1.12 $
 * $Date: 2001/06/08 23:33:13 $
 * ------------------------------------------------------------------------*/
 
Name nameIORun;			        /* run IO code                     */
Name nameIOBind;		        /* bind IO code                    */
Name namePutStr;		        /* Prelude.putStr                  */

static Name namePass;			/* auxiliary:: \f b c a -> f a b c */
#if IO_HANDLES
static Name nameHreader;	        /* auxiliary function		   */
static FILE *writingFile = 0;		/* points to file open for writing */
#endif

#if IO_HANDLES
static String toIOErrorDescr Args((int,Bool));
static Name toIOError Args((int));
#endif

/* --------------------------------------------------------------------------
 * IO monad control:
 * ------------------------------------------------------------------------*/

static Void iomonadControl Args((Int));
static Void iomonadControl(what)
Int what; {
    switch (what) {
	case INSTALL : 
		       setCurrModule(modulePrelude);
#define pFun(n,s,t)    addPrim(0,n=newName(findText(s),NIL),t,modulePrelude,NIL)
		       pFun(namePass,	 "_pass",    "passIO");
#if    IO_HANDLES
		       pFun(nameHreader, "_hreader", "hreader");
#endif
#undef pFun
#define predef(nm,str) nm=newName(findText(str),NIL); name(nm).defn=PREDEFINED
		       predef(nameIORun,    "hugsIORun");
		       predef(nameIOBind,   "primbindIO");
		       predef(namePutStr,   "hugsPutStr");
#undef predef
		       break;

	case RESET   : 
#if IO_HANDLES
		       if (writingFile) {
			   fclose(writingFile);
			   writingFile = 0;
		       }
#endif
		       break;
    }
}

PROTO_PRIM(primLunit);
PROTO_PRIM(primRunit);
PROTO_PRIM(primLbind);
PROTO_PRIM(primRbind);
PROTO_PRIM(primPass);

PROTO_PRIM(primGC);
PROTO_PRIM(primGetEnv);
PROTO_PRIM(primSystem);
PROTO_PRIM(primGetRandomSeed);

PROTO_PRIM(primArgc);
PROTO_PRIM(primArgv);

PROTO_PRIM(primUserError);
PROTO_PRIM(primIsUserErr);

PROTO_PRIM(primIsPermDenied);
/* PROTO_PRIM(primIsNameErr); */

PROTO_PRIM(primGetCh);
PROTO_PRIM(primGetChar);
PROTO_PRIM(primPutChar);
PROTO_PRIM(primPutStr);

#if IO_HANDLES
static Void local fwritePrim  Args((StackPtr,Bool,Bool,String));
static Void local fopenPrim   Args((StackPtr,Bool,String));

PROTO_PRIM(primHGetChar);
PROTO_PRIM(primHPutChar);
PROTO_PRIM(primHPutStr);
PROTO_PRIM(primHreader);
PROTO_PRIM(primHContents);
PROTO_PRIM(primContents);
PROTO_PRIM(primOpenFile);
PROTO_PRIM(primOpenBinaryFile);
PROTO_PRIM(primStdin);
PROTO_PRIM(primStdout);
PROTO_PRIM(primStderr);
PROTO_PRIM(primHIsEOF);
PROTO_PRIM(primHugsHIsEOF);
PROTO_PRIM(primHFlush);
PROTO_PRIM(primHClose);
PROTO_PRIM(primHGetPosn);
PROTO_PRIM(primHSetPosn);
PROTO_PRIM(primHSetBuffering);
PROTO_PRIM(primHGetBuffering);
PROTO_PRIM(primHSeek);
PROTO_PRIM(primHLookAhead);
PROTO_PRIM(primHIsOpen);
PROTO_PRIM(primHIsClosed);
PROTO_PRIM(primHIsReadable);
PROTO_PRIM(primHIsWritable);
PROTO_PRIM(primHIsSeekable);
PROTO_PRIM(primHFileSize);
PROTO_PRIM(primHWaitForInput);
PROTO_PRIM(primEqHandle);
PROTO_PRIM(primReadFile);
PROTO_PRIM(primWriteFile);
PROTO_PRIM(primAppendFile);

PROTO_PRIM(primReadBinaryFile);
PROTO_PRIM(primWriteBinaryFile);
PROTO_PRIM(primAppendBinaryFile);

PROTO_PRIM(primIsIllegal);
PROTO_PRIM(primIsWriteErr);
PROTO_PRIM(primIsEOFError);
PROTO_PRIM(primIsAlreadyExist);
PROTO_PRIM(primIsAlreadyInUse);
PROTO_PRIM(primIsDoesNotExist);
PROTO_PRIM(primIsFull);
PROTO_PRIM(primIsUnsupported);

PROTO_PRIM(primGetErrorString);
PROTO_PRIM(primGetHandle);
PROTO_PRIM(primGetFileName);
#endif

#if IO_REFS
PROTO_PRIM(primNewRef);
PROTO_PRIM(primDerefRef);
PROTO_PRIM(primAssignRef);
PROTO_PRIM(primEqRef);
#endif

PROTO_PRIM(primMakeSP);
PROTO_PRIM(primDerefSP);
PROTO_PRIM(primFreeSP);

PROTO_PRIM(primMakeFO);
PROTO_PRIM(primWriteFO);
PROTO_PRIM(primEqFO);

#if GC_WEAKPTRS
PROTO_PRIM(primMakeWeakPtr);
PROTO_PRIM(primDerefWeakPtr);
PROTO_PRIM(primWeakPtrEq);
PROTO_PRIM(primMkWeak);
PROTO_PRIM(primDeRefWeak);
PROTO_PRIM(primReplaceFinalizer);
PROTO_PRIM(primFinalize);
PROTO_PRIM(primRunFinalizer);
PROTO_PRIM(primFinalizerWaiting);
#endif

#if STABLE_NAMES
PROTO_PRIM(primMakeSN);
PROTO_PRIM(primDerefSN);
PROTO_PRIM(primHashSN);
PROTO_PRIM(primEqSN);
#endif

#if DIRECTORY_OPS
PROTO_PRIM(primCreateDirectory);
PROTO_PRIM(primRemoveDirectory);
PROTO_PRIM(primRemoveFile);
PROTO_PRIM(primRenameDirectory);
PROTO_PRIM(primRenameFile);
PROTO_PRIM(primGetDirectory);
PROTO_PRIM(primSetDirectory);
PROTO_PRIM(primFileExist);
PROTO_PRIM(primDirExist);
PROTO_PRIM(primGetPermissions);
PROTO_PRIM(primSetPermissions);
PROTO_PRIM(primGetDirContents);
#endif

#if CPUTIME_OPS 
PROTO_PRIM(primClockTicks);
PROTO_PRIM(primGetCPUUsage);
#endif

#ifdef HSCRIPT
PROTO_PRIM(primGetCurrentScript);
#endif

static struct primitive iomonadPrimTable[] = {
  {"lunitIO",		3, primLunit},
  {"runitIO",		3, primRunit},
  {"lbindIO",		4, primLbind},
  {"rbindIO",		4, primRbind},
  {"passIO",		4, primPass},

  {"primGC",	        2, primGC},
  {"getEnv",	        3, primGetEnv},
  {"primSystem",	3, primSystem},
  {"getRandomSeed",	2, primGetRandomSeed},

  {"primArgc",	        2, primArgc},
  {"primArgv",	        3, primArgv},

  {"getCh",		2, primGetCh},
  {"getChar",		2, primGetChar},
  {"putChar",		3, primPutChar},
  {"putStr",		3, primPutStr},

  /*  {"userError",		1, primUserError}, */
  {"isUserError",	1, primIsUserErr},
  /* non-standard tests */
  /*  {"hugsIsNameErr",	1, primIsNameErr}, */
  /* end non-standard tests */

#if IO_HANDLES
  {"hGetChar",		3, primHGetChar},
  {"hPutChar",		4, primHPutChar},
  {"hPutStr",		4, primHPutStr},
  {"hreader",		1, primHreader},
  {"hGetContents",	3, primHContents},
  {"getContents",	2, primContents},
  {"openFile",          4, primOpenFile},
  {"openBinaryFile",    4, primOpenBinaryFile},
  {"stdin",		0, primStdin},
  {"stdout",		0, primStdout},
  {"stderr",		0, primStderr},
  {"hIsEOF",		3, primHIsEOF},
  {"hugsHIsEOF",	3, primHugsHIsEOF},
  {"hFlush",		3, primHFlush},
  {"hClose",		3, primHClose},
  {"hGetPosnPrim",	3, primHGetPosn},
  {"hSetPosnPrim",	4, primHSetPosn},
  {"hSetBuff",          5, primHSetBuffering},
  {"hGetBuff",          3, primHGetBuffering},
  {"hSeekPrim",         5, primHSeek},
  {"hLookAhead",        3, primHLookAhead},
  {"hIsOpen",		3, primHIsOpen},
  {"hIsClosed",		3, primHIsClosed},
  {"hIsReadable",	3, primHIsReadable},
  {"hIsWritable",	3, primHIsWritable},
  {"hIsSeekable",       3, primHIsSeekable},
  {"hFileSize",         3, primHFileSize},
  {"hWaitForInput",     4, primHWaitForInput},
  {"primEqHandle",	2, primEqHandle},
  {"readFile",		3, primReadFile},
  {"writeFile",		4, primWriteFile},
  {"appendFile",	4, primAppendFile},
  {"readBinaryFile",	3, primReadBinaryFile},
  {"writeBinaryFile",	4, primWriteBinaryFile},
  {"appendBinaryFile",	4, primAppendBinaryFile},
  {"isAlreadyExistsError", 1, primIsAlreadyExist},
  {"isDoesNotExistError",  1, primIsDoesNotExist},
  {"isAlreadyInUseError",  1, primIsAlreadyInUse},
  {"isFullError",	   1, primIsFull},
  {"isEOFError",	   1, primIsEOFError},
  {"isIllegalOperation",   1, primIsIllegal},
  {"isPermissionError",	   1, primIsPermDenied},
  /* non-standard tests */
  {"hugsIsWriteErr",       1, primIsWriteErr},
  /* end non-standard tests */
  {"ioeGetErrorString",	1, primGetErrorString},
  {"ioeGetHandle",	1, primGetHandle},
  {"ioeGetFileName",	1, primGetFileName},
#endif

#if IO_REFS
  {"newRef",            3, primNewRef},
  {"getRef",		3, primDerefRef},
  {"setRef",		4, primAssignRef},
  {"eqRef",		2, primEqRef},
#endif

  {"makeStablePtr",	3, primMakeSP},
  {"deRefStablePtr",	3, primDerefSP},
  {"freeStablePtr",	3, primFreeSP},

  {"makeForeignObj",	4, primMakeFO},
  {"writeForeignObj",	4, primWriteFO},
  {"eqForeignObj",	2, primEqFO},

#if GC_WEAKPTRS
  {"makeWeakPtr",       3, primMakeWeakPtr},
  {"derefWeakPtr",      3, primDerefWeakPtr},
  {"weakPtrEq",		2, primWeakPtrEq},
  {"mkWeak",		5, primMkWeak},
  {"deRefWeak",		3, primDeRefWeak},
  {"replaceFinalizer",	4, primReplaceFinalizer},
  {"finalize",		3, primFinalize},
  {"runFinalizer",	2, primRunFinalizer},
  {"finalizerWaiting",	2, primFinalizerWaiting},
#endif

#if STABLE_NAMES
  {"makeStableName",	3, primMakeSN},
  {"deRefStableName",	1, primDerefSN},
  {"hashStableName",	1, primHashSN},
  {"eqStableName",	2, primEqSN},
#endif

#if DIRECTORY_OPS
  /* Directory primitives */
  {"createDirectory",      3, primCreateDirectory},
  {"removeDirectory",      3, primRemoveDirectory},
  {"removeFile",           3, primRemoveFile},
  {"renameDirectory",      4, primRenameDirectory},
  {"renameFile",           4, primRenameFile},
  {"getCurrentDirectory",  2, primGetDirectory},
  {"setCurrentDirectory",  3, primSetDirectory},
  {"doesFileExist",        3, primFileExist},
  {"doesDirectoryExist",   3, primDirExist},
  {"getPerms",             3, primGetPermissions},
  {"setPerms",             7, primSetPermissions},
  {"getDirContents",       3, primGetDirContents},
#endif
  
#if CPUTIME_OPS
  /* CPUTime primitives */
  {"clockTicks",           0, primClockTicks},
  {"getCPUUsage",          2, primGetCPUUsage},
#endif
  
#ifdef HSCRIPT
  {"getCurrentScript",  2, primGetCurrentScript},
#endif

  {0,			0, 0}
};

static struct primInfo iomonadPrims = { iomonadControl, iomonadPrimTable, 0 };

/* --------------------------------------------------------------------------
 * Macros
 *
 * Note: the IOReturn and IOFail macros do not use the standard "do while"
 * trick to create a single statement because some C compilers (eg sun)
 * report warning messages "end-of-loop code not reached".
 * This may lead to syntax errors if used where a statement is required - such
 * errors can be fixed by adding braces round the call.  Blech!
 * ------------------------------------------------------------------------*/

#define IOArg(n)    primArg((n)+2)
#define IOReturn(r) { updapRoot(primArg(1),r); return; }
#define IOFail(r)   { updapRoot(primArg(2),r); return; }

/* --------------------------------------------------------------------------
 * The monad combinators:
 * ------------------------------------------------------------------------*/

primFun(primLunit) {			/* bimonad left unit		   */
    updapRoot(primArg(2),primArg(3));	/* lunit 3 2 1 = 2 3		   */
}

primFun(primRunit) {			/* bimonad right unit		   */
    updapRoot(primArg(1),primArg(3));	/* lunit 3 2 1 = 1 3		   */
}

primFun(primLbind) {			/* bimonad left bind		   */
    push(ap(namePass,primArg(3)));	/* lbind 4 3 2 1 = 4 (pass 3 2 1) 1*/
    toparg(primArg(2));
    toparg(primArg(1));
    updapRoot(ap(primArg(4),top()),primArg(1));
}

primFun(primRbind) {			/* bimonad right bind		   */
    push(ap(namePass,primArg(3)));	/* rbind 4 3 2 1 = 4 2 (pass 3 2 1)*/
    toparg(primArg(2));
    toparg(primArg(1));
    updapRoot(ap(primArg(4),primArg(2)),top());
}

primFun(primPass) {			/* Auxiliary function		   */
    push(ap(primArg(4),primArg(1)));	/* pass 4 3 2 1 = 4 1 3 2	   */
    toparg(primArg(3));
    updapRoot(top(),primArg(2));
}

/* --------------------------------------------------------------------------
 * Handle operations:
 * ------------------------------------------------------------------------*/

#if IO_HANDLES

Cell openHandle(root,sCell,hmode,binary,loc) /* open handle to file named s in  */
StackPtr root;
Cell   sCell;                                /* the specified hmode             */
Int    hmode; 
Bool   binary;
String loc; {
    Int i;
    String s = evalName(sCell);

    /* openHandle() returns a *pair*, the first component contains NIL if
       opening the handle failed (and the second component contains the IOError
       describing the error). If the handled was created successfully,
       the second component contains the Handle.
    */

    if (!s) {				/* check for valid name		   */
      return(pair(NIL,
		  mkIOError(nameIllegal,
			    loc,
			    "illegal file name",
			    nameNothing)));
    }

    for (i=0; i<NUM_HANDLES && nonNull(handles[i].hcell); ++i)
	;                                       /* Search for unused handle*/
    if (i>=NUM_HANDLES) {                       /* If at first we don't    */
	garbageCollect();                       /* succeed, garbage collect*/
	for (i=0; i<NUM_HANDLES && nonNull(handles[i].hcell); ++i)
	    ;                                   /* and try again ...       */
    }

    if (i>=NUM_HANDLES) {                       /* ... before we give up   */
      return(pair(NIL,
		  mkIOError(nameIllegal,
			    loc,
			    "too many handles open",
			    sCell)));
    }
    else {                                      /* prepare to open file    */
	String stmode;
	if (binary) {
	    stmode = (hmode&HAPPEND)    ? "ab+" :
		     (hmode&HWRITE)     ? "wb+" :
		     (hmode&HREADWRITE) ? "wb+" :
		     (hmode&HREAD)      ? "rb" : (String)0;
	} else {
	    stmode = (hmode&HAPPEND)     ? "a+"  :
		     (hmode&HWRITE)      ? "w+"  :
		     (hmode&HREADWRITE)  ? "w+"  :
		     (hmode&HREAD)       ? "r"  : (String)0;
	}
	if (stmode && (handles[i].hfp=fopen(s,stmode))) {
	    handles[i].hmode = hmode;
	    handles[i].hbufMode = HUNKNOWN_BUFFERING;
	    handles[i].hbufSize = (-1);
	    if (hmode&HREADWRITE) {
	      handles[i].hHaveRead = FALSE;
	    }
	    return (pair(nameNothing,handles[i].hcell = ap(HANDCELL,i)));
	}
	return (pair(NIL,
		     mkIOError(toIOError(errno),
			       loc,
			       toIOErrorDescr(errno,TRUE),
			       sCell)));
    }
}

/* --------------------------------------------------------------------------
 * Building strings:
 * ------------------------------------------------------------------------*/

Void local pushString(s)       /* push pointer to string onto stack */
String s; {
    Int  l      = strlen(s);
    push(nameNil);
    while (--l >= 0) {
	topfun(consChar(s[l]));
    }
}

/* Helper function for constructing IOErrors (see Prelude defn of
 * IOError for explanation of what the the individual arguments
 * do.
 */
  
Cell
mkIOError(kind, loc, desc, mbF)
Name   kind;
String loc;
String desc;
Cell   mbF;
{
   Cell locStr;
   pushString(loc);
   locStr = pop();

   pushString(desc);

   return (ap(ap(ap(ap(nameIOError,
   			  kind),
		       locStr),
		    pop()),
       	     ((mbF == nameNothing) ? mbF : ap(nameJust,mbF))));
}


#endif
/* --------------------------------------------------------------------------
 * IO Errors (more defined for file ops)
 * ------------------------------------------------------------------------*/


/*
 * Map a libc error code to an IOError
 */
static Name toIOError(errc)
int errc;
{
#ifdef HAVE_ERRNO_H
  switch(errc) {

  case EEXIST:
    return nameAlreadyExists;
  case ENOENT:
  case ENOTDIR:
    return nameDoesNotExist;
  case EPERM:
  case EACCES:
    return namePermDenied;
  case ENOSPC:
  case EFBIG:
    return nameIsFull;
  default:
    return nameIllegal;
  }
#else
  return nameIllegal;
#endif
}

/*
 * Map a libc error code to an IOError descriptive string
 */
static String toIOErrorDescr(errc,isFile)
int   errc;
Bool  isFile;
{
#ifdef HAVE_ERRNO_H
  switch(errc) {

  case EEXIST:
    return (isFile ? "file already exists" : "directory already exists");
  case ENOENT:
  case ENOTDIR:
    return (isFile ? "file does not exist" : "directory does not exist");
  case EPERM:
  case EACCES:
    return ""; /* No need to replicate the info conveyed by the IOErrorKind */
  case ENOSPC:
  case EFBIG:
    return "device is full";
  default:
    return "";
  }
#else
  return "";
#endif
}

primFun(primIsUserErr) {		/* :: IOError -> Bool        	   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead==nameUserErr);
}

primFun(primIsDoesNotExist) {		/* :: IOError -> Bool		   */
    eval(primArg(1));   /* unwinds the IOError dcon, pushing the args onto the stack */
    eval(primArg(5));   /* select the error 'kind' and evaluate it */
    checkCon();
    BoolResult(whnfHead==nameDoesNotExist);
}

primFun(primIsAlreadyExist) {		/* :: IOError -> Bool		   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead== nameAlreadyExists);
}

primFun(primIsAlreadyInUse) {		/* :: IOError -> Bool		   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead== nameAlreadyInUse);
}

primFun(primIsFull) {		/* :: IOError -> Bool		   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead== nameIsFull);
}

primFun(primIsPermDenied) {		/* :: IOError -> Bool		   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead==namePermDenied);
}

primFun(primIsUnsupported) {		/* :: IOError -> Bool		   */
    updateRoot(nameFalse);
}


primFun(primIsIllegal) {		/* :: IOError -> Bool	   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead==nameIllegal);
}

primFun(primIsWriteErr) {		/* :: IOError -> Bool	   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead==nameWriteErr);
}

primFun(primIsEOFError) {		/* :: IOError -> Bool	   */
    eval(primArg(1));
    eval(primArg(5));
    checkCon();
    BoolResult(whnfHead==nameEOFErr);
}

/* --------------------------------------------------------------------------
 * Misc.
 * ------------------------------------------------------------------------*/

primFun(primGC) {			/* force a GC right now            */
    garbageCollect();
    IOReturn(nameUnit);
}

#if BIGNUMS && defined HAVE_TIME_H
#include <time.h>

primFun(primGetRandomSeed) {		/* generate a random seed          */
    IOReturn(bigInt(clock()));
}

#else

primFun(primGetRandomSeed) {		/* generate a random seed          */
    ERRMSG(0) "getRandomSeed is not implemented on this architecture"
    EEND;
}

#endif
				     
primFun(primGetEnv) {                 /* primGetEnv :: String -> IO String */
    String s = evalName(IOArg(1));    /* Eval name	                   */
    String r;
    if (!s) {			      /* check for valid name		   */
	IOFail(mkIOError(nameIllegal,
		         "System.getEnv",
			 "illegal environment variable name",
			 IOArg(1)));
    } else if ((r = getenv(s))!=0) {    
	pushString(r);
	IOReturn(pop());
    } else {
	IOFail(mkIOError(nameDoesNotExist,
		         "System.getEnv",
			 "environment variable not found",
			 IOArg(1)));
    }
}

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

					 
primFun(primSystem) {                   /* primSystem :: String -> IO Int  */
    String s = evalName(IOArg(1));	/* Eval name	                   */
    Int r;
    if (s) {				/* check for valid string          */
#if HAVE_MACSYSTEM
        r = macsystem(s);
#else
	r = system(s);
#endif
	IOReturn(mkInt(WEXITSTATUS(r)));
    } else {
	IOFail(mkIOError(nameIllegal,
		         "System.system",
			 "illegal system command string",
			 IOArg(1)));
    }
}
    
static String defaultArgv[] = {
  "Hugs"  /* program name */
};

static String* hugsArgv = defaultArgv;
static Int     hugsArgc = sizeof defaultArgv / sizeof defaultArgv[0];

Void setHugsArgs(argc,argv)
Int    argc;
String argv[]; {
    hugsArgc = argc;
    hugsArgv = argv;
}

primFun(primArgc) {                     /* primArgc :: IO Int              */
    IOReturn(mkInt(hugsArgc));
}
    
primFun(primArgv) {                     /* primArgv :: Int -> IO String    */
    Int i;
    IntArg(i,3);
    if (0 <= i && i < hugsArgc) {
	pushString(hugsArgv[i]);
	IOReturn(pop());
    } else {
	IOFail(mkIOError(nameIllegal,
		         "System.getArgs",
			 "illegal argument",
			 nameNothing));
    }
}
    
/* --------------------------------------------------------------------------
 * Console IO
 * ------------------------------------------------------------------------*/

primFun(primGetCh) {			/* Get character from stdin wo/echo*/
    IOReturn(mkChar(readTerminalChar()));
}

#if __MWERKS__ && macintosh
primFun(primGetChar) {			 /* Metrowerks console has no NO_ECHO mode. */
    IOReturn(mkChar(readTerminalChar()));
}
#else
primFun(primGetChar) {			/* Get character from stdin w/ echo*/
    Char c = readTerminalChar();
    putchar(c);
    fflush(stdout);
    IOReturn(mkChar(c));
}
#endif

primFun(primPutChar) {			/* print character on stdout	   */
    eval(pop());
    putchar(charOf(whnfHead));
    fflush(stdout);
    IOReturn(nameUnit);
}

primFun(primPutStr) {			/* print string on stdout	   */
    blackHoleRoot();            	/* supposedly = hPutStr stdout,	   */
    eval(pop());			/* included here for speed	   */
    while (whnfHead==nameCons) {
	eval(top());
	checkChar();
	putchar(charOf(whnfHead));
#if FLUSHEVERY
	fflush(stdout);
#endif
	drop();
	eval(pop());
    }
#if !FLUSHEVERY
    fflush(stdout);
#endif
    IOReturn(nameUnit);
}

/* --------------------------------------------------------------------------
 * File IO
 * ------------------------------------------------------------------------*/

#if IO_HANDLES

#define HandleArg(nm,offset)  \
    eval(primArg(offset));    \
    nm = intValOf(whnfHead)

#define IOBoolResult(e)  \
    IOReturn((e)?nameTrue:nameFalse)

primFun(primHGetChar) {			/* Read character from handle	   */
    Int h;
    HandleArg(h,3);
    
    /* Flush output buffer for R/W handles */
    if (handles[h].hmode&HREADWRITE && !handles[h].hHaveRead) {
	fflush(handles[h].hfp);
	handles[h].hHaveRead = TRUE;
    }

    if (handles[h].hmode&(HREAD|HREADWRITE)) {
	Char c = (h==HSTDIN ? readTerminalChar() : getc(handles[h].hfp));
	if (c!=EOF) {
	    IOReturn(mkChar(c));
	} else if ( feof(handles[h].hfp) ) {
	  IOFail(mkIOError(nameEOFErr,
			   "IO.hGetChar",
			   "end of file",
			   nameNothing));
	}
    }
    IOFail(mkIOError(toIOError(errno),
		     "IO.hGetChar",
		     toIOErrorDescr(errno,TRUE),
		     nameNothing));
}

primFun(primHPutChar) {			/* print character on handle	   */
    Char c = 0;
    Int  h;
    HandleArg(h,4);
    CharArg(c,3);

    /* Flush input buffer for R/W handles */
    if (handles[h].hmode&HREADWRITE && handles[h].hHaveRead) {
	fflush(handles[h].hfp);
	handles[h].hHaveRead = FALSE;
    }
    if (handles[h].hmode&(HWRITE|HAPPEND|HREADWRITE)) {
	if ( putc(c,handles[h].hfp) == EOF ) {
	  IOFail(mkIOError(toIOError(errno),
			   "IO.hPutChar",
			   toIOErrorDescr(errno,TRUE),
			   nameNothing));
	}
	IOReturn(nameUnit);
    }
    IOFail(mkIOError(nameIllegal,
		     "IO.hPutChar",
		     "handle is not writable",
		     nameNothing));
}

primFun(primHPutStr) {			/* print string on handle	   */
    Int h;
    HandleArg(h,4);
    push(primArg(3));
    primArg(3) = NIL;

    /* Make sure the input buffer is flushed for R/W handles */
    if (handles[h].hmode&HREADWRITE && handles[h].hHaveRead) {
	fflush(handles[h].hfp);
	handles[h].hHaveRead = FALSE;
    }
    if (handles[h].hmode&(HWRITE|HAPPEND|HREADWRITE)) {
	blackHoleRoot();
	eval(pop());
	while (whnfHead==nameCons) {
	    eval(pop());
	    putc(charOf(whnfHead),handles[h].hfp);
#if FLUSHEVERY
	    if ( h < 2 ) {  /* Only flush the standard handles */
	      fflush(handles[h].hfp);
	    }
#endif
	    eval(pop());
	}
#if !FLUSHEVERY
	if (h < 2) { /* Only flush the standard handles */
	  fflush(handles[h].hfp);
	}
#endif
	IOReturn(nameUnit);
    }
    IOFail(mkIOError(nameIllegal,
		     "IO.hPutStr",
		     "handle is not writable",
		     nameNothing));
}

primFun(primHreader) {			/* read String from a handle 	   */
    Int h;                              /* Handle -> String                */
    HandleArg(h,1);
    if (handles[h].hmode&HSEMICLOSED) {	/* read requires semi-closed handle*/
	Int c = (h==HSTDIN ? readTerminalChar() : getc(handles[h].hfp));
	if (c!=EOF && c>=0 && c<NUM_CHARS) {
	    updapRoot(consChar(c),ap(nameHreader,primArg(1)));
	    return;
	}
	clearerr(handles[h].hfp);
    }
    updateRoot(nameNil);
}

primFun(primHContents) {		/* hGetContents :: Handle -> IO Str*/
    Int h;
    HandleArg(h,3);
    if ((handles[h].hmode&(HREAD|HREADWRITE))==0) { /* must have readable handle	   */
        IOFail(mkIOError(nameIllegal,
		         "IO.hGetContents",
		         "handle is not readable",
		         nameNothing));
    } else {				/* semi-close handle		   */
	handles[h].hmode = HSEMICLOSED;
	if (handles[h].hmode&HREADWRITE && !handles[h].hHaveRead) {
	  fflush(handles[h].hfp);
	  handles[h].hHaveRead = TRUE;
	}
	IOReturn(ap(nameHreader,IOArg(1)));
    }
}

primFun(primContents) {			/* Get contents of stdin	   */
    if ((handles[HSTDIN].hmode&HREAD)==0) {
        IOFail(mkIOError(nameIllegal,
		         "Prelude.getContents",
		         "handle is not readable",
		         nameNothing));
    } else {
	handles[HSTDIN].hmode = HSEMICLOSED;
	IOReturn(ap(nameHreader,handles[HSTDIN].hcell));
    }
}

static Void local fopenPrim(root,binary,loc)/* Auxiliary function for          */
StackPtr root;                              /* opening a file                  */
Bool     binary;
String   loc; {
    Int    m = HCLOSED;

    eval(IOArg(1));			/* Eval IOMode			   */
    if (isName(whnfHead) && isCfun(whnfHead))
	switch (cfunOf(whnfHead)) {	/* we have to use numeric consts   */
	    case 1 : m = HREAD;		/* here to avoid the need to put   */
		     break;		/* IOMode in startup environment   */
	    case 2 : m = HWRITE;
		     break;
	    case 3 : m = HAPPEND;
		     break;
	    case 4 : m = HREADWRITE;
		     break;
	}

    if (m!=HCLOSED) {			/* Only accept legal modes	   */
	Cell hnd = openHandle(root,IOArg(2),m,binary,loc);
	if (!isNull(fst(hnd))) {
	   IOReturn(snd(hnd));
        } else {
	  IOFail(snd(hnd));
	}
    }
    
   IOFail(mkIOError(nameIllegal,
		    loc,
		    "unknown handle mode",
		    IOArg(2)));
}

primFun(primOpenFile) {			/* open handle to a text file	   */
    fopenPrim(root,FALSE,"IO.openFile");
}

primFun(primOpenBinaryFile) {		/* open handle to a binary file	   */
    fopenPrim(root,TRUE,"IOExtensions.openBinaryFile");
}

primFun(primStdin) {			/* Standard input handle	   */
    push(handles[HSTDIN].hcell);
}

primFun(primStdout) {			/* Standard output handle	   */
    push(handles[HSTDOUT].hcell);
}

primFun(primStderr) {			/* Standard error handle	   */
    push(handles[HSTDERR].hcell);
}

/* NOTE: this doesn't implement the Haskell 1.3 semantics */
primFun(primHugsHIsEOF) {		/* Test for end of file on handle  */
    Int h;
    HandleArg(h,3);
    if (handles[h].hmode!=HCLOSED) {
        IOBoolResult(feof(handles[h].hfp));
    } else {
        IOFail(mkIOError(nameIllegal,
		         "IO.hugsIsEOF",
		         "handle is closed",
			 nameNothing));
    }
}

primFun(primHIsEOF) {	/* Test for end of file on handle  */
                        /* :: Handle -> IO Bool */
    Int h;
    FILE* fp;
    HandleArg(h,3);
    if (handles[h].hmode&(HREAD|HREADWRITE)) {
      Bool isEOF;
      fp = handles[h].hfp;
      isEOF = feof(fp);
      if (isEOF) { /* If the EOF flag is already signalled,
		      peeking at the next char isn't likely
		      to produce a different outcome! */
	IOBoolResult(isEOF);
      } else {
	Int c = fgetc(fp);
	isEOF = feof(fp);

	/* Put the char back and clear any flags. */
	ungetc(c,fp);
	clearerr(fp);
	
	IOBoolResult(isEOF);
	}
    } else {
        IOFail(mkIOError(nameIllegal,
		         "IO.hIsEOF",
		         "handle is closed",
			 nameNothing));
    }
}


primFun(primHFlush) {			/* Flush handle			   */
    Int h;
    HandleArg(h,3);
    if (handles[h].hmode&(HWRITE|HAPPEND|HREADWRITE)) { /* Only allow flushing writable handles */
	fflush(handles[h].hfp);
	if (handles[h].hmode&HREADWRITE) {
	  handles[h].hHaveRead = FALSE;
	}
	IOReturn(nameUnit);
    }
    else
        IOFail(mkIOError(nameIllegal,
		         "IO.hFlush",
		         "handle is not writable",
			 nameNothing));
}

primFun(primHClose) {			/* Close handle                   */
    Int h;
    HandleArg(h,3);
    if (handles[h].hmode!=HCLOSED) {
	if (h>HSTDERR && handles[h].hfp)
	    fclose(handles[h].hfp);
	handles[h].hfp   = 0;
	handles[h].hmode = HCLOSED;
	IOReturn(nameUnit);
    }
    IOFail(mkIOError(nameIllegal,
		     "IO.hClose",
		     "handle is closed",
		     nameNothing));
}

primFun(primHGetPosn) {			/* Get file position               */
    Int h;
    HandleArg(h,3);
    if (handles[h].hmode!=HCLOSED) {
#if HAVE_FTELL
	long pos = ftell(handles[h].hfp);
	IOReturn(mkInt((Int)pos));
#else
	/* deliberate fall through to IOFail */
#endif
    }
    IOFail(mkIOError(nameIllegal,
		     "IO.hGetPosn",
		     "handle is closed",
		     nameNothing));
}

primFun(primHSetPosn) {			/* Set file position               */
#if HAVE_FSEEK
    long   pos = 0;
#endif
    Int    h;
    HandleArg(h,4);
    IntArg(pos,3);
    if (handles[h].hmode!=HCLOSED) {
#if HAVE_FSEEK
        fflush(handles[h].hfp);
	if (fseek(handles[h].hfp,pos,SEEK_SET) == 0) {
	    IOReturn(nameUnit);
	}
#else
	/* deliberate fall through to IOFail */
#endif
    }
    IOFail(mkIOError(nameIllegal,
		     "IO.hSetPosn",
		     "handle is closed",
		     nameNothing));
}

primFun(primHSeek) {	/* Seek to new file posn */
                        /* :: Handle -> Int -> Int -> IO () */
  Int h;
  Int sMode;
  Int off;
  
  HandleArg(h,5);
  IntArg(sMode, 4);
  IntArg(off, 3);
  
  if (sMode == 0) 
    sMode = SEEK_SET;
  else if (sMode == 1)
    sMode = SEEK_CUR;
  else
    sMode = SEEK_END;

  if (handles[h].hmode&(HWRITE|HREAD|HAPPEND|HREADWRITE)) {
    if (fseek(handles[h].hfp,off,sMode) != 0) {
      IOFail(mkIOError(toIOError(errno),
		       "IO.hSeek",
		       toIOErrorDescr(errno,TRUE),
		       nameNothing));
    }

    if (handles[h].hmode&HREADWRITE) {
      handles[h].hHaveRead = FALSE;
    }
      
    IOReturn(nameUnit);
  }
  IOFail(mkIOError(nameIllegal,
		   "IO.hSeek",
		   "handle is not seekable",
		   nameNothing));
}


primFun(primHLookAhead) { /* Peek at the next char */
                          /* :: Handle -> IO Char  */
  Int h;
  Int c;
  
  HandleArg(h,3);

  if (handles[h].hmode&(HREAD|HREADWRITE)) {
    if (!feof(handles[h].hfp)) {
      if ((c = fgetc(handles[h].hfp)) != EOF) {
	ungetc(c, handles[h].hfp);
	IOReturn(mkChar(c));
      } else {
	IOFail(mkIOError(toIOError(errno),
			 "IO.hLookAhead",
			 toIOErrorDescr(errno,TRUE),
			 nameNothing));
      }
    } else {
      IOFail(mkIOError(nameEOFErr,
		       "IO.hLookAhead",
		       "end of file",
		       nameNothing));
    }
  } else if (handles[h].hmode&HWRITE) {
    IOFail(mkIOError(nameIllegal,
		     "IO.hLookAhead",
		     "handle is not readable",
		     nameNothing));
  } else {
    IOFail(mkIOError(nameIllegal,
		     "IO.hLookAhead",
		     "handle is closed",
		     nameNothing));
  
  }
}



primFun(primHSetBuffering) {	/* Change a Handle's buffering */
                                /* :: Handle -> Int -> Int -> IO () */
    Int h;
    Int ty;
    Int sz;
    int rc;
    HandleArg(h,5);
    IntArg(ty,4);
    IntArg(sz,3);

    if (handles[h].hmode!=HCLOSED) {
        switch(ty) {
        case 0:
	  ty = _IONBF;
	  handles[h].hbufMode = HANDLE_NOTBUFFERED;
	  handles[h].hbufSize = 0;
	  break;
        case 1:
	  ty = _IOLBF;
	  sz = BUFSIZ;
	  handles[h].hbufMode = HANDLE_LINEBUFFERED;
	  handles[h].hbufSize = 0;
	  break;
        case 2:
	  ty = _IOFBF;
	  handles[h].hbufMode = HANDLE_BLOCKBUFFERED;
	  if (sz == 0) {
	    sz=BUFSIZ;
	  }
	  handles[h].hbufSize = sz;
	  break;
        default:
	  IOFail(mkIOError(nameIllegal,
		           "IO.hSetBuffering",
		           "illegal buffer mode",
		           nameNothing));
        }

	/* Change the buffering mode; setvbuf() flushes the old buffer. */
	/* Let setvbuf() allocate the buffer for us. */
	rc = setvbuf(handles[h].hfp, NULL, ty, sz);
	if (rc != 0) {
	  IOFail(mkIOError(toIOError(errno),
		           "IO.hSetBuffering",
		           "unable to change buffering",
		           nameNothing));
	}
	IOReturn(nameUnit);
    } else
        IOFail(mkIOError(nameIllegal,
		         "IO.hSetBuffering",
		         "handle is closed",
		         nameNothing));
}

primFun(primHGetBuffering) {	/* Return buffering info of a handle. */
                                /*  Handle :: IO (Int,Int)            */
  Int h;
  HandleArg(h,3);

  if (handles[h].hmode != HCLOSED) {
    if (handles[h].hbufMode == HUNKNOWN_BUFFERING) {
      /* figure out buffer mode and size. */
#if HAVE_ISATTY
      if (isatty (fileno(handles[h].hfp)) ) {
	/* TTY connected handles are linebuffered. */
	handles[h].hbufMode = HANDLE_LINEBUFFERED;
	handles[h].hbufSize = 0;
      } else {
#endif
	/* ..if not, block buffered. */
	handles[h].hbufMode = HANDLE_BLOCKBUFFERED;
	handles[h].hbufSize = BUFSIZ;
#if HAVE_ISATTY
      }
#endif
    }
    IOReturn(ap(ap(mkTuple(2),mkInt((Int)handles[h].hbufMode)),
		mkInt((Int)handles[h].hbufSize)));
  } else {
        IOFail(mkIOError(nameIllegal,
		         "IO.hGetBuffering",
		         "handle is closed",
		         nameNothing));
  }
}

primFun(primHIsOpen) {			/* Test is handle open             */
    Int h;
    HandleArg(h,3);
    IOBoolResult(handles[h].hmode!=HCLOSED 
		 && handles[h].hmode!=HSEMICLOSED);
}

primFun(primHIsClosed) {		/* Test is handle closed           */
    Int h;
    HandleArg(h,3);
    IOBoolResult(handles[h].hmode==HCLOSED);
}

primFun(primHIsReadable) {		/* Test is handle readable         */
    Int h;
    HandleArg(h,3);
    IOBoolResult(handles[h].hmode&HREAD || handles[h].hmode&HREADWRITE);
}

primFun(primHIsWritable) {		/* Test is handle writable         */
    Int h;
    HandleArg(h,3);
    IOBoolResult(handles[h].hmode&(HWRITE|HREADWRITE|HAPPEND));
}

primFun(primHIsSeekable) {		/* Test if handle is writable   */
  Int h;
  Bool okHandle;
#if HAVE_FSTAT
  struct stat sb;
#endif
  
  HandleArg(h,3);
    

  okHandle = (handles[h].hmode&(HREAD|HWRITE|HREADWRITE|HAPPEND));
#if HAVE_FSTAT
  if (okHandle && (fstat(fileno(handles[h].hfp), &sb) == 0)) {
    okHandle = S_ISREG(sb.st_mode);
  }
#endif
  IOBoolResult(okHandle);
}

primFun(primHFileSize) {  /* If handle points to a regular file,
			     return the size of the file   */
                          /* :: Handle -> IO Integer       */
  Int h;
  Bool okHandle;
#if HAVE_FSTAT
  struct stat sb;
#endif

  HandleArg(h,3);
  
  okHandle = (handles[h].hmode&(HREAD|HWRITE|HREADWRITE|HAPPEND));
#if HAVE_FSTAT
  if (okHandle && (fstat(fileno(handles[h].hfp), &sb) == 0) &&
      S_ISREG(sb.st_mode)) {
    IOReturn(bigWord(sb.st_size));
  }
#endif
  IOFail(mkIOError(nameIllegal,
		   "IO.hFileSize",
		   (okHandle ? "not a regular file" : "handle is (semi-)closed."),
		   nameNothing));
}

primFun(primEqHandle) {			/* Test for handle equality        */
    Int h1, h2;
    HandleArg(h1,1);
    HandleArg(h2,2);
    BoolResult(h1==h2);
}

primFun(primReadFile) {			/* read file as lazy string	   */
  Cell hnd = openHandle(root,IOArg(1),HREAD,FALSE,"Prelude.readFile");
  if (isNull(fst(hnd))) {
    IOFail(snd(hnd));
  }
  handles[intValOf(snd(hnd))].hmode = HSEMICLOSED;
  IOReturn(ap(nameHreader,snd(hnd)));
}

primFun(primReadBinaryFile) {		/* read file as lazy string	   */
  Cell hnd = openHandle(root,IOArg(1),HREAD,TRUE,"IOExtensions.readBinaryFile");
  if (isNull(fst(hnd))) {
    IOFail(snd(hnd));
  }
  handles[intValOf(snd(hnd))].hmode = HSEMICLOSED;
  IOReturn(ap(nameHreader,snd(hnd)));
}

primFun(primWriteFile) {		/* write string to specified file  */
    fwritePrim(root,FALSE,FALSE,"Prelude.writeFile");
}

primFun(primAppendFile) {		/* append string to specified file */
    fwritePrim(root,TRUE,FALSE,"Prelude.appendFile");
}

primFun(primWriteBinaryFile) {		/* write string to specified file  */
    fwritePrim(root,FALSE,TRUE,"IOExtensions.writeBinaryFile");
}

primFun(primAppendBinaryFile) {		/* append string to specified file */
    fwritePrim(root,TRUE,TRUE,"IOExtensions.appendBinaryFile");
}

#if HAVE_GETFINFO
extern int access(char *fileName, int dummy);
#endif

static Void local fwritePrim(root,append,binary,loc)
                                 /* Auxiliary function for  */
StackPtr root;			 /* writing/appending to    */
Bool     append; 		 /* an output file	    */
Bool     binary;
String   loc; {
    String mode = binary ? (append ? "ab" : "wb")
			 : (append ? "a"  : "w");
    String s    = evalName(IOArg(2));		/* Eval and check filename */
    if (!s) {
        IOFail(mkIOError(nameIllegal,
			 loc,
		         "illegal file name",
			 IOArg(2)));
    }
    else if (append && access(s,0)!=0) {	/* Check that file exists  */
        IOFail(mkIOError(nameDoesNotExist,
			 loc,
		         "file name does not exist",
			 IOArg(2)));
    }
    else if ((writingFile=fopen(s,mode))==0) {	/* Open file for writing   */
        IOFail(mkIOError(toIOError(errno),
			 loc,
			 toIOErrorDescr(errno,TRUE),
			 IOArg(2)));
    }
    else {					/* Output characters	   */
	blackHoleRoot();
	drop();
	eval(pop());
	while (whnfHead==nameCons) {
	    eval(top());
	    checkChar();
	    fputc(charOf(whnfHead),writingFile);
	    drop();
	    eval(pop());
	}
	fclose(writingFile);
	writingFile = 0;
	IOReturn(nameUnit);
    }
}

primFun(primGetErrorString) {		/* :: IOError -> String	   */
  updateRoot(ap(nameGetErrorString,primArg(1)));
}

primFun(primGetHandle) {		/* :: IOError -> Maybe Handle	   */
    eval(primArg(1));
    /* insert tests here */
    updateRoot(nameNothing);
}

primFun(primGetFileName) {		/* :: IOError -> Maybe FilePath	   */
  updateRoot(ap(nameGetFilename,primArg(1)));
}

primFun(primHWaitForInput) { /* Check whether a character can be read
				from a handle within x msecs */
                             /* :: Handle -> Int -> IO Bool */
  Int h;
  Int msecs;
  
  HandleArg(h,4);
  IntArg(msecs,3);
  
#if defined(HAVE_SELECT) && !(defined(_WIN32) && !defined(__CYGWIN__))
  if (handles[h].hmode&(HREAD|HREADWRITE)) {
    /* Implementation is a rip-off of GHC's inputReady.c */
    int maxfd, fd;
    int ready;
    fd_set rfd;
    struct timeval tv;
    
    FD_ZERO(&rfd);
    fd = fileno(handles[h].hfp);
    FD_SET(fd, &rfd);
    
    maxfd = fd + 1;
    tv.tv_sec  = msecs / 1000;
    tv.tv_usec = msecs % 1000;
    
    while ( (ready = select(maxfd, &rfd, NULL, NULL, &tv)) < 0 ) {
      if (errno != EINTR) {
	IOFail(mkIOError(nameIllegal,
			 "IO.hWaitForInput",
			 "input waiting terminated by signal",
			 nameNothing));
      }
    }
    IOBoolResult(ready > 0);
  } else {
    IOFail(mkIOError(nameIllegal,
		     "IO.hWaitForInput",
		     "handle is not readable",
		     nameNothing));
  }
#else
  /* For now, punt on implementing async IO under Win32 */
  /* For other platforms that don't support select90 on file
     file descs, please insert code that'll work. */
  IOFail(mkIOError(nameIllegal,
		   "IO.hWaitForInput",
		   "unsupported operation",
		   nameNothing));
#endif
}

#endif /* IO_HANDLES */

/* --------------------------------------------------------------------------
 * Mutable variables
 * ------------------------------------------------------------------------*/

#if IO_REFS

#if CHECK_TAGS
#define checkRef() if (MUTVAR != whatIs(whnfHead)) internal("Ref expected")
#else
#define checkRef() /* do nothing */
#endif

primFun(primNewRef) {			/* a -> IO (Ref a)		   */
    IOReturn(ap(MUTVAR,IOArg(1)));
}

primFun(primDerefRef) {			/* Ref a -> IO a		   */
    eval(pop());
    checkRef();
    IOReturn(snd(whnfHead));
}

primFun(primAssignRef) {		/* Ref a -> a -> IO ()		   */
    eval(IOArg(2));
    checkRef();
    snd(whnfHead) = IOArg(1);
    IOReturn(nameUnit);
}

primFun(primEqRef) {			/* Ref a -> Ref a -> Bool	   */
    eval(primArg(2));
    checkRef();
    push(whnfHead);
    eval(primArg(1));
    checkRef();
    updateRoot(pop()==whnfHead ? nameTrue : nameFalse);
}
#endif

/* --------------------------------------------------------------------------
 * Stable Pointers
 * ------------------------------------------------------------------------*/

primFun(primMakeSP) {			/* a -> IO (StablePtr a)	   */
    Int sp = mkStablePtr(IOArg(1));
    if (sp > 0) {
	IOReturn(mkInt(sp));
    } else {
        IOFail(mkIOError(nameIllegal,
			 "Foreign.makeStablePtr",
		         "illegal operation",
			 nameNothing));
    }
}

primFun(primDerefSP) {			/* StablePtr a -> IO a   	   */
    eval(IOArg(1));
    IOReturn(derefStablePtr(whnfInt));
}

primFun(primFreeSP) {			/* StablePtr a -> IO ()   	   */
    eval(IOArg(1));
    freeStablePtr(whnfInt);
    IOReturn(nameUnit);
}
    
/* --------------------------------------------------------------------------
 * Foreign Objects
 * ------------------------------------------------------------------------*/

#if CHECK_TAGS
#define checkForeign() if (MPCELL != whatIs(whnfHead)) internal("ForeignObj expected")
#else
#define checkForeign() /* do nothing */
#endif

primFun(primMakeFO) {			/* a -> IO (Ref a)		   */
    Pointer addr = 0;
    Void (*free)(Pointer) = 0;
    eval(IOArg(2));
    addr = ptrOf(whnfHead);
    eval(IOArg(1));
    free = (Void (*)(Pointer))ptrOf(whnfHead);
    IOReturn(mkMallocPtr(addr,free));
}

primFun(primWriteFO) {		/* ForeignObj -> Addr -> IO ()		   */
    Cell mp = NIL;
    eval(IOArg(2));
    checkForeign();
    mp = whnfHead;
    eval(IOArg(1));
    derefMP(mp) = ptrOf(whnfHead);
    IOReturn(nameUnit);
}

primFun(primEqFO) {			/* ForeignObj -> ForeignObj -> Bool*/
    eval(primArg(2));
    checkForeign();
    push(whnfHead);
    eval(primArg(1));
    checkForeign();
    updateRoot(pop()==whnfHead ? nameTrue : nameFalse);
}

#if STABLE_NAMES
/* --------------------------------------------------------------------------
 * Stable Names
 * ------------------------------------------------------------------------*/

primFun(primMakeSN) {		/* a -> IO (StableName a)		   */
    IOReturn(ap(STABLENAME,IOArg(1)));
}

primFun(primDerefSN) {		/* StableName a -> a			   */
    eval(primArg(1));
    updateRoot(snd(whnfHead));
}

primFun(primHashSN) {		/* StableName a -> Int			   */
    eval(primArg(1));
    updateRoot(mkInt(whnfHead));
}
primFun(primEqSN) {		/* StableName a -> StableName a -> Bool	   */
    eval(primArg(2));
    push(whnfHead);
    eval(primArg(1));
    updateRoot(pop()==whnfHead ? nameTrue : nameFalse);
}
#endif

#if GC_WEAKPTRS
/* --------------------------------------------------------------------------
 * Weak Pointers
 * ------------------------------------------------------------------------*/

#if CHECK_TAGS
#define checkWeak() if(WEAKCELL!=whatIs(whnfHead)) internal("weakPtr expected");
#else
#define checkWeak() /* do nothing */
#endif

primFun(primMakeWeakPtr) {		/* a -> IO (Weak a)		   */
    assert(isGenPair(IOArg(1)));	/* (Sadly, this may not be true)   */
    IOReturn(mkWeakPtr(IOArg(1)));	/* OLD ... retire soon		   */
}

primFun(primDerefWeakPtr) {		/* Weak a -> IO (Maybe a)	   */
    eval(IOArg(1));			/* OLD ... retire soon		   */
    checkWeak();
    if (isNull(derefWeakPtr(whnfHead))) {
	IOReturn(nameNothing);
    } else {
	IOReturn(ap(nameJust,derefWeakPtr(whnfHead)));
    }
}

primFun(primWeakPtrEq) {		/* Weak a -> Weak a -> Bool	   */
    eval(primArg(2));
    push(whnfHead);
    eval(primArg(1));
    updateRoot(pop()==whnfHead ? nameTrue : nameFalse);
}

primFun(primMkWeak) {			/* k -> v -> Maybe (IO ())	   */
    Cell w = NIL;			/*		    -> IO (Weak v) */
    eval(IOArg(1));
    if (whnfHead==nameJust) {		/* Look for finalizer		   */
	w = pop();
    }
    w		     = ap(NIL,ap(NIL,ap(NIL,w)));
    fst(snd(w))      = IOArg(3);
    fst(snd(snd(w))) = IOArg(2);
    liveWeakPtrs     = cons(w,liveWeakPtrs);
    fst(w)           = WEAKFIN;
    IOReturn(w);
}

primFun(primDeRefWeak) {		/* Weak v -> IO (Maybe v)	   */
    eval(IOArg(1));
    if (whatIs(whnfHead)!=WEAKFIN) {
	internal("primDeRefWeak");
    }
    if (nonNull(snd(whnfHead))) {
	IOReturn(ap(nameJust,fst(snd(snd(whnfHead)))));
    } else {
	IOReturn(nameNothing);
    }
}

primFun(primReplaceFinalizer) {		/* Weak v -> Maybe (IO ())	   */
					/*	-> IO (Maybe (IO ()))	   */
    eval(IOArg(1));			/* Grab new finalizer ...	   */
    if (whnfHead!=nameJust) {
	push(NIL);
    }
    eval(IOArg(2));			/* Get weak pointer ...		   */
    if (whatIs(whnfHead)!=WEAKFIN) {
	internal("primReplaceFinalizer");
    } else if (nonNull(snd(whnfHead))) {/* ... and replace finalizer	   */
	Cell oldfin = snd(snd(snd(whnfHead)));
	snd(snd(snd(whnfHead))) = pop();
	if (nonNull(oldfin)) {
	    IOReturn(ap(nameJust,oldfin));
	}
    }
    IOReturn(nameNothing);
}

primFun(primFinalize) {			/* Weak v -> IO ()		   */
    eval(IOArg(1));			/* Bring weak pointer to an early  */
    if (whatIs(whnfHead)!=WEAKFIN) {	/* end ...			   */
	internal("primFinalize");
    } else if (nonNull(snd(whnfHead))) {
	Cell wp = whnfHead;
	Cell vf = snd(snd(wp));
	if (isPair(vf)) {
	    if (nonNull(snd(vf))) {
		fst(vf)    = snd(vf);
		snd(vf)    = finalizers;
		finalizers = vf;
	    }
	    fst(snd(wp)) = NIL;
	    snd(snd(wp)) = NIL;
	    snd(wp)      = NIL;
	}
	liveWeakPtrs = removeCell(wp,liveWeakPtrs);
    }
    IOReturn(nameUnit);
}

primFun(primRunFinalizer) {		/* IO ()			   */
    if (isNull(finalizers)) {
	IOReturn(nameUnit);
    } else {
	updapRoot(ap(hd(finalizers),primArg(2)),primArg(1));
	finalizers = tl(finalizers);
	return;
    }
}

primFun(primFinalizerWaiting) {		/* IO Boolean			   */
  IOBoolResult(!isNull(finalizers));
}
#endif /* GC_WEAKPTRS */

#if DIRECTORY_OPS
/* --------------------------------------------------------------------------
 * Directory primitives
 * ------------------------------------------------------------------------*/

primFun(primCreateDirectory) { /* create a directory	   */
  int rc;
  String s = evalName(IOArg(1));
  
  if (!s) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.createDirectory",
		     "illegal directory name",
		     IOArg(1)));
  }
  
#if defined(_MSC_VER) || defined(__MINGW__)
   rc = mkdir(s);
#else
   rc = mkdir(s,0777);
#endif
   if (rc != 0) {
      IOFail(mkIOError(toIOError(errno),
		       "Directory.createDirectory",
		       toIOErrorDescr(errno,FALSE),
		       IOArg(1)));
   }
  IOReturn(nameUnit);
}

primFun(primRemoveDirectory) { /* remove a directory	   */
  int rc;
  String s = evalName(IOArg(1));
  
  if (!s) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.removeDirectory",
		     "illegal directory name",
		     IOArg(1)));
  }
  
   rc = rmdir(s);

   if (rc != 0) {
     IOFail(mkIOError(toIOError(errno),
		      "Directory.removeDirectory",
		      toIOErrorDescr(errno,FALSE),
		      IOArg(1)));
   }
  IOReturn(nameUnit);
}

primFun(primRemoveFile) { /* remove a file	   */
  int rc;
  String s = evalName(IOArg(1));
  
  if (!s) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.removeFile",
		     "illegal file name",
		     IOArg(1)));
  }
  
   rc = unlink(s);

   if (rc != 0) {
     IOFail(mkIOError(toIOError(errno),
		      "Directory.removeFile",
		      toIOErrorDescr(errno,TRUE),
		      IOArg(1)));
   }
  IOReturn(nameUnit);
}

/* Pair of macros for creating temporary strings */
#ifdef HAVE_ALLOCA
# define ALLOC_STRING(x) (String)alloca(sizeof(char)*(x + 1))
# define FREE_STRING(x)
#elif HAVE__ALLOCA
# define ALLOC_STRING(x) (String)_alloca(sizeof(char)*(x + 1))
# define FREE_STRING(x)
#else
# define ALLOC_STRING(x) (String)malloc(sizeof(char)*(x + 1))
# define FREE_STRING(x)  free(x)
#endif

primFun(primRenameDirectory) { /* rename a directory	   */
  int rc;
  String tmpStr;
  String to;
  String from;

  tmpStr = evalName(IOArg(1));
  to = ALLOC_STRING(strlen(tmpStr));
  strcpy(to, tmpStr);

  from = evalName(IOArg(2));

  if (!from) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.renameDirectory",
		     "illegal directory name",
		     IOArg(2)));
  }

  if (!to) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.renameDirectory",
		     "illegal directory name",
		     IOArg(1)));
  }
  
  rc = rename(from,to);

  FREE_STRING(to);

  if (rc != 0) {
     IOFail(mkIOError(toIOError(errno),
		      "Directory.renameDirectory",
		      toIOErrorDescr(errno,FALSE),
		      IOArg(1)));
  }
  IOReturn(nameUnit);
}

primFun(primRenameFile) { /* rename a file	   */
  int rc;
  String tmpStr;
  String to;
  String from;

  tmpStr = evalName(IOArg(1));
  to = ALLOC_STRING(strlen(tmpStr));
  strcpy(to, tmpStr);

  from = evalName(IOArg(2));
  
  if (!from) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.renameFile",
		     "illegal directory name",
		     IOArg(1)));
  }
  
  if (!to) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.renameFile",
		     "illegal directory name",
		     IOArg(2)));
  }
  
  rc = rename(from,to);
  
  FREE_STRING(to);

  if (rc != 0) {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.renameFile",
		     toIOErrorDescr(errno,TRUE),
		     IOArg(1)));
  }
  IOReturn(nameUnit);
}


primFun(primGetDirectory) { /* IO String - get current directory. */
  int rc;
  char buffer[FILENAME_MAX+1];
  if ((char*)(getcwd(buffer,FILENAME_MAX)) != (char*)NULL) {    
    pushString(buffer);
    IOReturn(pop());
  } else {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.getCurrentDirectory",
		     toIOErrorDescr(errno,FALSE),
		     IOArg(1)));
  }
}

primFun(primSetDirectory) { /* String -> IO () - set current directory. */
  int rc;
  String s = evalName(IOArg(1));

  if (!s) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.setCurrentDirectory",
		     "illegal directory name",
		     IOArg(1)));
  }
  
   rc = chdir(s);

   if (rc != 0) {
     IOFail(mkIOError(toIOError(errno),
		      "Directory.setCurrentDirectory",
		      toIOErrorDescr(errno,FALSE),
		      IOArg(1)));
   }
   IOReturn(nameUnit);
}

primFun(primFileExist) { /* FilePath -> IO Bool - check to see if file exists. */
  int rc;
  String s = evalName(IOArg(1));
  struct stat st;

  if (!s) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.doesFileExist",
		     "illegal file name",
		     IOArg(1)));
  }
  
  rc = stat(s, &st);
  
  if (rc < 0) {
    IOReturn(nameFalse);
  } else {
    IOBoolResult(((st.st_mode & S_IFDIR) != S_IFDIR));
  }
}

primFun(primDirExist) { /* FilePath -> IO Bool - check to see if directory exists. */
  int rc;
  String s = evalName(IOArg(1));
  struct stat st;

  if (!s) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.doesDirectoryExist",
		     "illegal directory name",
		     IOArg(1)));
  }
  
  rc = stat(s, &st);
  
  if (rc < 0) {
    IOReturn(nameFalse);
  } else {
    IOBoolResult(((st.st_mode & S_IFDIR) == S_IFDIR));
  }
}

#define IS_FLAG_SET(x,flg) ((x.st_mode & flg) == flg)
#define ToBool(v) ( (v) ? nameTrue : nameFalse)

primFun(primGetPermissions) { /* FilePath -> IO (Bool,Bool,Bool,Bool) */
  int rc;
  String s = evalName(IOArg(1));
  struct stat st;

  if (!s) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.getPermissions",
		     "illegal file name",
		     IOArg(1)));
  }
  
  rc = stat(s, &st);
  
  if (rc != 0) {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.getPermissions",
		     toIOErrorDescr(errno,TRUE),
		     IOArg(1)));
  } else {
    IOReturn(ap(ap(ap(ap( mkTuple(4),
			  ToBool(IS_FLAG_SET(st, S_IREAD))),
		      ToBool(IS_FLAG_SET(st, S_IWRITE))),
		   ToBool(IS_FLAG_SET(st, S_IEXEC) && !IS_FLAG_SET(st,S_IFDIR))),
		ToBool(IS_FLAG_SET(st,S_IEXEC) && IS_FLAG_SET(st,S_IFDIR))));
  }
}

#define EVAL_BOOL(x,y) \
   eval(y);\
   if (whnfHead==nameTrue) { \
      x = TRUE; \
   } else if (whnfHead==nameFalse) { \
      x = FALSE; \
   } else { \
      IOFail(mkIOError(nameIllegal, \
	     "Directory.setPermissions", \
	     "illegal flag", \
	     nameNothing)); \
   }

#ifdef _MSC_VER
#define READ_FLAG   S_IREAD
#define WRITE_FLAG  S_IWRITE
#define EXEC_FLAG   S_IEXEC
#else
#define READ_FLAG   S_IRUSR
#define WRITE_FLAG  S_IWUSR
#define EXEC_FLAG   S_IXUSR
#endif

#define SET_CHMOD_FLAG(x,y)  (x ? y : 0)

primFun(primSetPermissions) { /* FilePath -> Bool -> Bool -> Bool -> Bool -> IO () */
  int rc;
  String str;

  Bool   r;
  Bool   w;
  Bool   e;
  Bool   s;
  
  EVAL_BOOL(s, IOArg(1));
  EVAL_BOOL(e, IOArg(2));
  EVAL_BOOL(w, IOArg(3));
  EVAL_BOOL(r, IOArg(4));
  
  str = evalName(IOArg(5));
  
  if (!str) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.setPermissions",
		     "illegal file name",
		     IOArg(5)));
  }

  rc = chmod(str,
	     SET_CHMOD_FLAG(r, READ_FLAG)  |
	     SET_CHMOD_FLAG(w, WRITE_FLAG) |
	     SET_CHMOD_FLAG(e||s, EXEC_FLAG));
	     
  if (rc != 0) {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.setPermissions",
		     toIOErrorDescr(errno,TRUE),
		     IOArg(5)));
  } else {
    IOReturn(nameUnit);
  }
  
}

/* Pedantically remove these local defs. */
#undef READ_FLAG
#undef WRITE_FLAG
#undef EXEC_FLAG
#undef SET_CHMOD_FLAG

primFun(primGetDirContents) { /* FilePath -> IO [FilePath] */
  int rc;
#ifdef _MSC_VER
  /* The MS CRT doesn't provide opendir()/readdir(), but uses
     the 'std' MS find first/next/close group of functions for
     iterating over the contents of a directory. */
  long dirHandle;
  struct _finddata_t fData;
  char buffer[FILENAME_MAX+20];
  struct stat st;
  Cell ls;
  String fName = evalName(IOArg(1));
  
  if (!fName) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.getDirectoryContents",
		     "illegal directory name",
		     IOArg(1)));
  }
  
  if (strlen(fName) > FILENAME_MAX) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.getDirectoryContents",
		     "file name too long",
		     IOArg(1)));
  }
  
  /* First, check whether the directory exists... */
  if ( (stat(fName, &st) < 0) || ((st.st_mode & S_IFDIR) != S_IFDIR) ) {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.getDirectoryContents",
		     toIOErrorDescr(errno,FALSE),
		     IOArg(1)));
  }
  
  strcpy(buffer, fName);
  strcat(buffer, "\\*.*");

  dirHandle = _findfirst(buffer, &fData);
  rc = dirHandle;
  
  ls = nameNil;
  
  while (rc >= 0) {
    pushString(fData.name);
    ls = ap2(nameCons, pop(), ls);
    rc = _findnext(dirHandle, &fData);
  }
  if (errno != ENOENT) {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.getDirectoryContents",
		     toIOErrorDescr(errno,FALSE),
		     IOArg(1)));
  }

  /* Close and release resources */
  rc = _findclose(dirHandle);
  if (rc == -1 && errno != ENOENT) {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.getDirectoryContents",
		     toIOErrorDescr(errno,FALSE),
		     IOArg(1)));
  }
  IOReturn(ls);
#elif defined(HAVE_DIRENT_H)
  /* opendir() / readdir() implementation. */
  DIR* dir;
  struct dirent* pDir;
  Cell ls;
  String fName = evalName(IOArg(1));
  
  if (!fName) {
    IOFail(mkIOError(nameIllegal,
		     "Directory.getDirectoryContents",
		     "illegal file name",
		     IOArg(1)));
  }
  
  dir = opendir(fName);
  
  if (dir == NULL) {
    IOFail(mkIOError(toIOError(errno),
		     "Directory.getDirectoryContents",
		     toIOErrorDescr(errno,FALSE),
		     IOArg(1)));
  }
  
  ls = nameNil;
  
  /* To ensure that the test below doesn't
     succeed just because the impl of readdir()
     'forgot' to reset 'errno', do it ourselves. */
  errno = 0;

  while (pDir = readdir(dir)) {
    pushString(pDir->d_name);
    ls = ap2(nameCons, pop(), ls);
  }

  if (errno != 0) {
    int rc = errno;
    closedir(dir);
    IOFail(mkIOError(toIOError(rc),
		     "Directory.getDirectoryContents",
		     toIOErrorDescr(rc,FALSE),
		     IOArg(1)));
  }

  closedir(dir);
  IOReturn(ls);

#else
  /* Sorry, don't know how to access a directory on your platform */
    IOFail(mkIOError(nameIllegal,
		     "Directory.getDirectoryContents",
		     "operation not supported",
		     IOArg(1)));
#endif
}

#endif /* DIRECTORY_OPS */


#if CPUTIME_OPS
/* CPUTime primitive operators */

#ifdef CLK_TCK
CAFInt(primClockTicks, CLK_TCK)
#elif defined(CLOCKS_PER_SEC)
CAFInt(primClockTicks, CLOCKS_PER_SEC)
#else
CAFInt(primClockTicks, sysconf(_SC_CLK_TCK))
#endif

/* 
 * The code for grabbing the process times has been
 * lifted from GHC. Don't feel too bad about that, since
 * I wrote and maintained it. 
 */
primFun(primGetCPUUsage) { /* IO (Int,Int,Int,Int) */
  int userSec, userNSec;
  int sysSec,  sysNSec;
#ifndef _WIN32
#if defined(HAVE_GETRUSAGE) /* && ! irix_TARGET_OS && ! solaris2_TARGET_OS */
    struct rusage t;

    getrusage(RUSAGE_SELF, &t);
    userSec  = t.ru_utime.tv_sec;
    userNSec = 1000 * t.ru_utime.tv_usec;
    sysSec   = t.ru_stime.tv_sec;
    sysNSec  = 1000 * t.ru_stime.tv_usec;

#else
# if defined(HAVE_TIMES)
    struct tms t;
#  if defined(CLK_TCK)
#   define ticks CLK_TCK
#  else
    long ticks;
    ticks = sysconf(_SC_CLK_TCK);
#  endif

    times(&t);
    userSec  = t.tms_utime / ticks;
    userNSec = (t.tms_utime - userSec * ticks) * (1000000000 / ticks);
    sysSec   = t.tms_stime / ticks;
    sysNSec  = (t.tms_stime - sysSec * ticks) * (1000000000 / ticks);

# else
    IOFail(mkIOError(nameIllegal,
		     "CPUTime.getCPUTime",
		     "illegal operation",
		     nameNothing));
# endif
#endif
#else
/* Win32 version */

#ifdef _MSC_VER
#define NS_PER_SEC 10000000
#else
#define NS_PER_SEC 10000000LL
#endif
#define FT2usecs(ll,ft)    \
    (ll)=(ft).dwHighDateTime; \
    (ll) <<= 32;              \
    (ll) |= (ft).dwLowDateTime;

    FILETIME creationTime, exitTime, kernelTime, userTime;
#ifdef _MSC_VER
    unsigned __int64 uT, kT;
#else
    unsigned long long uT, kT;
#endif
 
    /* Notice that the 'process time' includes the time used
       by all the threads of a process, all of which may not
       be kept busy running the Hugs interpreter...
    */
    if (!GetProcessTimes (GetCurrentProcess(), &creationTime,
		          &exitTime, &kernelTime, &userTime)) {
	/* Probably on a Win95 box..*/
        userSec  = 0;
        userNSec = 0;
        sysSec   = 0;
        sysNSec  = 0;
    } else {

      FT2usecs(uT, userTime);
      FT2usecs(kT, kernelTime);

      userSec  = (unsigned int)(uT / NS_PER_SEC);
      userNSec = (unsigned int)((uT - userSec * NS_PER_SEC) * 100);
      sysSec   = (unsigned int)(kT / NS_PER_SEC);
      sysNSec  = (unsigned int)((kT - sysSec * NS_PER_SEC) * 100);
    }
#endif
    IOReturn(ap(ap(ap(ap( mkTuple(4), mkInt(userSec)),
		      mkInt(userNSec)),
		   mkInt(sysSec)),
		mkInt(sysNSec)));
}

#endif /* CPUTIME_OPS */

#if HSCRIPT
#if EMBEDDED
extern void* getCurrentScript(void);

primFun(primGetCurrentScript) {  /* IO Int */
    IOReturn( mkInt( (int)getCurrentScript() ) );
}

#else
 
primFun(primGetCurrentScript) {  /* IO Int */
    IOReturn( mkInt( 0 ) );
}

#endif /* EMBEDDED */
#endif /* HSCRIPT */

/*-------------------------------------------------------------------------*/
