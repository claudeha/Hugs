/* --------------------------------------------------------------------------
 * This is the Hugs foreign function interface
 *
 * The Hugs 98 system is Copyright (c) Mark P Jones, Alastair Reid, the
 * Yale Haskell Group, and the OGI School of Science & Engineering at OHSU,
 * 1994-2002, All rights reserved.  It is distributed as free software under
 * the license in the file "License", which is included in the distribution.
 *
 * $RCSfile: ffi.c,v $
 * $Revision: 1.12 $
 * $Date: 2002/06/18 00:41:58 $
 * ------------------------------------------------------------------------*/

#include "prelude.h"
#include "storage.h"
#include "connect.h"
#include "errors.h"

/* --------------------------------------------------------------------------
 * Local function prototypes:
 * ------------------------------------------------------------------------*/

static Void local foreignType    Args((Int,Type));
static Void local foreignGet     Args((Int,Type,String,Int));
static Void local foreignPut     Args((Int,Type,String,Int));
static Void local ffiDeclare     Args((Int,Type,String,Int));
static Void local ffiDeclareList Args((Int,List,String));
static Void local foreignType    Args((Int,Type));
static Void local ffiGetList     Args((Int,List,String));
static Void local ffiPutList     Args((Int,List,String));
static Void local ffiCallFun     Args((Int,Text,List,List));
static Void local ffiDeclareFun  Args((Int,Text,Bool,Bool,List,Type));
static Void local ffiPrimProto   Args((Text,Int));
static Void local ffiPrimHeader  Args((Text,Int));
static Void local ffiReturn      Args((Type,String,Int));

extern String scriptFile;

static FILE* out = NIL;                /* file we're generating code into */

Void ffi(what)
Int what; {
    switch (what) {
        case RESET   : if (out) {
                           fclose(out);
                           out = NIL;
                       }
		       break;
    }
}

#define BUFSIZE 1000
static char buffer[BUFSIZE];
static Int used = 0;

static Void local insert(s)
String s; {
    Int l = strlen(s);
    if (used + l + 1 >= BUFSIZE) {
        ERRMSG(0) "Unable to build compilation command"
        EEND;
    }
    strcpy(buffer+used,s);
    used += l;
}

#ifdef _MSC_VER
#define INSERT_QUOTE()   insert("\"")
#else
#define INSERT_QUOTE()
#endif

static Void local compileAndLink(fn,libs)
String fn; 
List   libs; {
    List xs = NIL;
    char* i = 0;
    used = 0;

    /* All relative filenames are to be interpreted relative to the 
     * directory containing the .hs file.
     */
    // insert("cd '");
    // insert(dirname(scriptFile));
    // insert("'; ");
    i = strrchr(scriptFile,'/');
    if (i != 0 && *i == '/') {
        insert("cd '");
        *i = '\0';
        insert(scriptFile);
        *i = '/';
        ++i;
        insert("'; ");
    } else {
        i = scriptFile;
    }

    /* The basic command */
#ifdef _MSC_VER
    insert("cl /LD \"-I");
    insert(hugsdir());
    insert("/include\" \"");
    insert(mkFFIFilename(i));
    insert("\" -o \"");
    insert(mkFFIFilename2(i));
    insert("\" ");
#else
    insert("cc -shared -I'");
    insert(hugsdir());
    insert("/include' '");
    //    insert(notdir(mkFFIFilename(scriptFile)));
    insert(mkFFIFilename(i));
    insert("' -o '");
    //    insert(notdir(mkFFIFilename2(scriptFile)));
    insert(mkFFIFilename2(i));
    insert("' ");
#endif

    /* Add libraries */
    for(xs=libs; nonNull(xs); xs=tl(xs)) {
        Text t = textOf(hd(xs));
        if (!inventedText(t)) {
            INSERT_QUOTE();
            insert(textToStr(t));
            INSERT_QUOTE();
            insert(" ");
        }
    }

    // printf("Executing '%s'\n",buffer);
    if (system(buffer) != 0) {
        ERRMSG(0) "Error while running compilation command '%s'", buffer
        EEND;
    }
    used = 0;
}
#undef BUFSIZE

Void foreignHeader() {
    String fnm = mkFFIFilename(scriptFile);
    FILE* f = fopen(fnm,"w");
    if (f == NULL) {
        ERRMSG(0) "Unable to create file '%s'", fnm
        EEND;
    }
    out = f;
    fprintf(out,"/* Machine generated file, do not modify */\n");
    fprintf(out,"#include \"HsFFI.h\"\n");
    fprintf(out,"static HugsAPI4 *hugs = 0;");
}

Void foreignFooter(is,es)
List is;
List es; {
    List xs   = NIL;
    List libs = NIL;
    fprintf(out,"\n");

    /* Table of all primitives generated by foreign imports */
    fprintf(out,"static struct hugs_primitive hugs_primTable[] = {\n");
    for(xs=is; nonNull(xs); xs=tl(xs)) {
        Name n       = hd(xs);
        fprintf(out,"    {\"%s\", ",textToStr(name(n).text));
        fprintf(out,"%d, ",name(n).arity);
        fprintf(out,"hugsprim_%s_%d},\n",textToStr(name(n).extFun),name(n).foreignId);
    }
    for(xs=es; nonNull(xs); xs=tl(xs)) {
        Name n       = hd(xs);
        Text ext     = name(n).extFun;
        Bool dynamic = inventedText(ext);
        if (dynamic) {
            fprintf(out,"    {\"%s\", 3, ",textToStr(name(n).text));
            fprintf(out,"hugsprim_%s},\n",textToStr(name(n).extFun));
        }
    }
    fprintf(out,"};\n");
    fprintf(out,"\n");

    /* The control function: rebuilds stable ptr table on RESET */
    fprintf(out,
            "static void hugs_primControl(int);\n"
            "static void hugs_primControl(what)\n"
            "int what; {\n");

    if (nonNull(es)) {
      fprintf(out,
	      "    switch (what) {\n"
              "        case %d:\n", RESET
	      );
    }

    for(xs=es; nonNull(xs); xs=tl(xs)) {
        Name n       = hd(xs);
        Text ext     = name(n).extFun;
        Bool dynamic = inventedText(ext);
        if (!dynamic) {
            fprintf(out, "        hugs_stable_for_%s = ", textToStr(ext));
            fprintf(out, "hugs->lookupName(");
            fprintf(out, "\"%s\"", textToStr(module(name(n).mod).text));
            fprintf(out, ", \"%s\"", textToStr(name(n).text));
            fprintf(out, ");\n");
        }
    }
    if (nonNull(es)) {
      fprintf(out,"    }\n");
    }
    fprintf(out, "}\n");

    /* Boilerplate initialization function */
    fprintf(out,
           "static struct hugs_primInfo hugs_prims = { hugs_primControl, hugs_primTable, 0 };\n"
           "\n"
           "#ifdef __cplusplus\n"
           "extern \"C\" {\n"
           "#endif\n"
           "DLLEXPORT(void) initModule(HugsAPI4 *);\n"
           "DLLEXPORT(void) initModule(HugsAPI4 *hugsAPI) {\n"
           "    hugs = hugsAPI;\n"
           "    hugs->registerPrims(&hugs_prims);\n"
           "}\n"
           "#ifdef __cplusplus\n"
           "}\n"
           "#endif\n"
            "\n");

    fclose(out);
    out = NIL;

    for(xs=is; nonNull(xs); xs=tl(xs)) {
        Text t = name(hd(xs)).lib;
        if (!varIsMember(t,libs)) {
            libs = cons(mkVar(t),libs);
        }
    }
    for(xs=es; nonNull(xs); xs=tl(xs)) {
        Text t = name(hd(xs)).lib;
        if (!varIsMember(t,libs)) {
            libs = cons(mkVar(t),libs);
        }
    }
    compileAndLink(scriptFile,libs);

}

static Void local foreignType(l,t)
Int    l;
Type   t; {
    if      (t == typeChar)   fprintf(out,"HsChar");
    else if (t == typeInt)    fprintf(out,"HsInt");
    else if (t == typeInt8)   fprintf(out,"HsWord8");
    else if (t == typeInt16)  fprintf(out,"HsWord16");
    else if (t == typeInt32)  fprintf(out,"HsWord32");
    else if (t == typeInt64)  fprintf(out,"HsWord64");
    else if (t == typeWord8)  fprintf(out,"HsWord8");
    else if (t == typeWord16) fprintf(out,"HsWord16");
    else if (t == typeWord32) fprintf(out,"HsWord32");
    else if (t == typeWord64) fprintf(out,"HsWord64");
    else if (t == typeFloat)  fprintf(out,"HsFloat");
    else if (t == typeDouble) fprintf(out,"HsDouble");
    else if (t == typeBool)   fprintf(out,"HsBool");
    else if (t == typeAddr)   fprintf(out,"HsAddr");
    else if (getHead(t) == typePtr)    fprintf(out,"HsPtr");
    else if (getHead(t) == typeFunPtr) fprintf(out,"HsFunPtr");
    else if (getHead(t) == typeForeign)fprintf(out,"HsForeignPtr");
    else if (getHead(t) == typeStable) fprintf(out,"HsStablePtr");
    else {
        ERRMSG(l) "Illegal foreign type" ETHEN
        ERRTEXT " \"" ETHEN ERRTYPE(t);
        ERRTEXT "\""
        EEND;
   }
}

static Void local foreignGet(l,t,nm,num)
Int    l;
Type   t; 
String nm; 
Int    num; {
    if (t == typeUnit)        fprintf(out,"hugs->getUnit();\n");
    else if (t == typeChar)   fprintf(out,"%s%d = hugs->getChar();\n",       nm, num);
    else if (t == typeInt)    fprintf(out,"%s%d = hugs->getInt();\n",        nm, num);
    else if (t == typeInt8)   fprintf(out,"%s%d = hugs->getInt8();\n",       nm, num);
    else if (t == typeInt16)  fprintf(out,"%s%d = hugs->getInt16();\n",      nm, num);
    else if (t == typeInt32)  fprintf(out,"%s%d = hugs->getInt32();\n",      nm, num);
    else if (t == typeInt64)  fprintf(out,"%s%d = hugs->getInt64();\n",      nm, num);
    else if (t == typeWord8)  fprintf(out,"%s%d = hugs->getWord8();\n",      nm, num);
    else if (t == typeWord16) fprintf(out,"%s%d = hugs->getWord16();\n",     nm, num);
    else if (t == typeWord32) fprintf(out,"%s%d = hugs->getWord32();\n",     nm, num);
    else if (t == typeWord64) fprintf(out,"%s%d = hugs->getWord64();\n",     nm, num);
    else if (t == typeFloat)  fprintf(out,"%s%d = hugs->getFloat();\n",      nm, num);
    else if (t == typeDouble) fprintf(out,"%s%d = hugs->getDouble();\n",     nm, num);
    else if (t == typeBool)   fprintf(out,"%s%d = hugs->getBool();\n",       nm, num);
    else if (t == typeAddr)   fprintf(out,"%s%d = hugs->getAddr();\n",       nm, num);
    else if (getHead(t) == typePtr)    fprintf(out,"%s%d = hugs->getPtr();\n",        nm, num);
    else if (getHead(t) == typeFunPtr) fprintf(out,"%s%d = hugs->getFunPtr();\n",     nm, num);
    else if (getHead(t) == typeForeign)fprintf(out,"%s%d = hugs->getForeignPtr();\n", nm, num);
    else if (getHead(t) == typeStable) fprintf(out,"%s%d = hugs->getStablePtr4();\n",  nm, num);
    else {
        ERRMSG(l) "Illegal outbound (away from Haskell) type" ETHEN
        ERRTEXT " \"" ETHEN ERRTYPE(t);
        ERRTEXT "\""
        EEND;
   }
}

static Void local foreignPut(l,t,nm,num)
Int    l;
Type   t; 
String nm; 
Int    num; {
    if      (t == typeUnit)   fprintf(out,"\n");
    else if (t == typeChar)   fprintf(out,"hugs->putChar(%s%d);\n",       nm, num);
    else if (t == typeInt)    fprintf(out,"hugs->putInt(%s%d);\n",        nm, num);
    else if (t == typeInt8)   fprintf(out,"hugs->putInt8(%s%d);\n",       nm, num);
    else if (t == typeInt16)  fprintf(out,"hugs->putInt16(%s%d);\n",      nm, num);
    else if (t == typeInt32)  fprintf(out,"hugs->putInt32(%s%d);\n",      nm, num);
    else if (t == typeInt64)  fprintf(out,"hugs->putInt64(%s%d);\n",      nm, num);
    else if (t == typeWord8)  fprintf(out,"hugs->putWord8(%s%d);\n",      nm, num);
    else if (t == typeWord16) fprintf(out,"hugs->putWord16(%s%d);\n",     nm, num);
    else if (t == typeWord32) fprintf(out,"hugs->putWord32(%s%d);\n",     nm, num);
    else if (t == typeWord64) fprintf(out,"hugs->putWord64(%s%d);\n",     nm, num);
    else if (t == typeFloat)  fprintf(out,"hugs->putFloat(%s%d);\n",      nm, num);
    else if (t == typeDouble) fprintf(out,"hugs->putDouble(%s%d);\n",     nm, num);
    else if (t == typeBool)   fprintf(out,"hugs->putBool(%s%d);\n",       nm, num);
    else if (t == typeAddr)   fprintf(out,"hugs->putAddr(%s%d);\n",       nm, num);
    else if (getHead(t) == typePtr)    fprintf(out,"hugs->putPtr(%s%d);\n",        nm, num);
    else if (getHead(t) == typeFunPtr) fprintf(out,"hugs->putFunPtr(%s%d);\n",     nm, num);
    else if (getHead(t) == typeForeign)fprintf(out,"hugs->putForeignPtr(%s%d);\n", nm, num);
    else if (getHead(t) == typeStable) fprintf(out,"hugs->putStablePtr4(%s%d);\n", nm, num);
    else {
        ERRMSG(l) "Illegal inbound (coming into Haskell) type" ETHEN
        ERRTEXT " \"" ETHEN ERRTYPE(t);
        ERRTEXT "\""
        EEND;
   }
}

static Void local ffiDeclare(line,ty,prefix,i)       /* Declare variable  */
Int    line;
Type   ty;
String prefix; 
Int    i; {
    if (ty != typeUnit) {
        fprintf(out,"    ");
        foreignType(line,ty);
        fprintf(out," %s%d;\n",prefix,i);
    }
}

static Void local ffiReturn(ty,prefix,i)             /* Return variable  */
Type   ty;
String prefix; 
Int    i; {
    if (ty != typeUnit) {
        fprintf(out,"    return %s%d;\n",prefix,i);
    } else {
        fprintf(out,"    return;\n");
    }
}

static Void local ffiDeclareList(line,tys,prefix)   /* Declare variables */
Int    line;
List   tys; 
String prefix; {
    Int  i;
    for(i=1; nonNull(tys); tys=tl(tys),++i) {
        ffiDeclare(line,hd(tys),prefix,i);
    }
}

static Void local ffiGetList(line,tys,prefix)   /* Get values from Haskell */
Int    line;
List   tys; 
String prefix; {
    Int  i;
    for(i=1; nonNull(tys); tys=tl(tys),++i) {
        fprintf(out,"    ");
        foreignGet(line,hd(tys),prefix,i);
    }
}

static Void local ffiPutList(line,tys,prefix)    /* Put values to Haskell */
Int    line;
List   tys; 
String prefix; {
    Int  i;
    for(i=1; nonNull(tys); tys=tl(tys),++i) {
        fprintf(out,"    ");
        foreignPut(line,hd(tys),prefix,i);
    }
}

static Void local ffiDeclareFun(line,n,indirect,extraArg,argTys,resultTy)
Int  line;
Text n;
Bool indirect;
Bool extraArg; /* Add a StablePtr argument? */
List argTys;
List resultTy; {
    Int  i;
    if (resultTy == typeUnit) {
        fprintf(out,"void");
    } else {
        foreignType(line,resultTy);
    }
    if (indirect) {
        fprintf(out," (*%s)", textToStr(n));
    } else {
        fprintf(out," %s", textToStr(n));
    }
    fprintf(out,"(");
    if (extraArg) {
        fprintf(out,"HugsStablePtr fun1");
        if (nonNull(argTys)) {
            fprintf(out,", ");
        }
    }
    for(i=1; nonNull(argTys); argTys=tl(argTys),++i) {
        foreignType(line,hd(argTys));
        fprintf(out," arg%d",i);
        if (nonNull(tl(argTys))) {
            fprintf(out,", ");
        }
    }
    fprintf(out,")");
}

static Void local ffiCallFun(line,e,argTys,resultTy)
Int  line;
Text e;
List argTys;
Type resultTy; {
    Int  i;
    fprintf(out,"    ");
    if (resultTy != typeUnit) {
        fprintf(out,"res1 = ");
    }
    fprintf(out,"%s(", textToStr(e));
    for(i=1; nonNull(argTys); argTys=tl(argTys),++i) {
        fprintf(out,"arg%d",i);
        if (nonNull(tl(argTys))) {
            fprintf(out,", ");
        }
    }
    fprintf(out,");\n");
}


/* Generate a Hugs Prim prototype.
 * name should match the C function we're calling because we know
 * that name is a valid C identifier whereas the Haskell name may
 * not be.
 */
static Void local ffiPrimProto(name,id)
Text name; 
Int  id; {
    fprintf(out,"\nstatic void hugsprim_%s_%d(HugsStackPtr);\n",textToStr(name),id);
}

/* Generate a Hugs Prim Header.
 * name should match the C function we're calling because we know
 * that name is a valid C identifier whereas the Haskell name may
 * not be.
 */
static Void local ffiPrimHeader(name,id)
Text name; 
Int  id; {
    fprintf(out,"static void hugsprim_%s_%d(HugsStackPtr hugs_root)\n", textToStr(name),id);
}

/* Generate C code for calling C functions from Haskell.
 * The code has to be compiled with a C compiler and dynamically
 * loaded.
 *
 * For example:
 * 
 *     foreign import "static fn [lib] cid" name :: Int -> Float -> IO Char
 * ==>
 *     
 *     static void hugsprim_extnm(HugsStackPtr);
 *     static void hugsprim_extnm(HugsStackPtr hugs_root)
 *     {
 *         int   arg1 = hugs->getInt();                             
 *         float arg2 = hugs->getFloat();
 *         char  res1 = ext_nm(arg1,arg2);
 *         hugs->putChar(res1);
 *         hugs->returnIO(hugs_root,1);
 *     }
 * 
 */
Void implementForeignImport(line,id,fn,cid,argTys,isIO,resultTy)
Int  line;
Int  id;
Text fn;   /* Include file */
Text cid;  /* Function name */
List argTys;
Bool isIO;
Type resultTy; {
    ffiPrimProto(cid,id);
    ffiPrimHeader(cid,id);
    fprintf(out,"{\n");

    /* Prototype for function we're going to call */
    fprintf(out,"    extern ");
    ffiDeclareFun(line,cid,FALSE,FALSE,argTys,resultTy);
    fprintf(out,";\n");

    ffiDeclareList(line,argTys,"arg");
    ffiDeclare(line,resultTy,"res",1);
    ffiGetList(line,argTys,"arg");
    ffiCallFun(line,cid,argTys,resultTy);
    fprintf(out,"    ");
    foreignPut(line,resultTy,"res",1);
    if (isIO || nonNull(argTys)) {
      fprintf(out,"    hugs->return%s(hugs_root,%d);\n", 
              isIO?"IO":"Id",
              resultTy==typeUnit ? 0 : 1);
    }
    fprintf(out,"}\n");
}

Void implementForeignImportDynamic(line,id,e,argTys,isIO,resultTy)
Int  line;
Int  id;
Text e;
List argTys;
Bool isIO;
Type resultTy; {
    ffiPrimProto(e,id);
    ffiPrimHeader(e,id);
    fprintf(out,"{\n");

    /* Declare arguments and result */
    fprintf(out,"    ");
    ffiDeclareFun(line,e,TRUE,FALSE,argTys,resultTy);
    fprintf(out,";\n");

    ffiDeclareList(line,argTys,"arg");
    ffiDeclare(line,resultTy,"res",1);
    fprintf(out,"    %s = hugs->getAddr();\n", textToStr(e));
    ffiGetList(line,argTys,"arg");
    ffiCallFun(line,e,argTys,resultTy);
    fprintf(out,"    ");
    foreignPut(line,resultTy,"res",1);
    if (isIO || nonNull(argTys)) {
      fprintf(out,"    hugs->return%s(hugs_root,%d);\n", 
              isIO?"IO":"Id",
              resultTy==typeUnit ? 0 : 1);
    }
    fprintf(out,"}\n");
}

/*
 * For wrappers, we generate:
 *
 * For example:
 * 
 *     foreign import "wrapper" name ::            (Int -> Float -> Char) 
 *                                   -> IO (FunPtr (Int -> Float -> Char))
 * ==>
 *     
 *     static HsChar wrapper(HugsStablePtr arg1, HsInt arg2, HsFloat arg3);
 *     static HsChar wrapper(HugsStablePtr arg1, HsInt arg2, HsFloat arg3);
 *     {
 *         HsChar res1;
 *         hugs->derefStablePtr4(arg1);                             
 *         hugs->putInt(arg2);                             
 *         hugs->putFloat(arg3);
 *         if (hugs->runIO(2)) {
 *             exit(hugs->getInt());
 *         }
 *         res1 = hugs->getChar();
 *         return res1;
 *     }
 * 
 *     static void hugsprim_name(HugsStackPtr hugs_root);
 *     static void hugsprim_name(HugsStackPtr hugs_root)
 *     {
 *         HugsStablePtr arg1 = hugs->makeStablePtr4();    
 *         void* thunk = hugs->mkThunk(&wrapper,arg1);
 *         hugs->putAddr(thunk);
 *         hugs->returnIO(hugs_root,1);
 *     }
 */
Void implementForeignImportWrapper(line,id,e,argTys,isIO,resultTy)
Int  line;
Int  id;
Text e;
List argTys;
Bool isIO;
Type resultTy; {
    /* Prototype for function we're generating */
    fprintf(out,"\nstatic ");
    ffiDeclareFun(line,e,FALSE,TRUE,argTys,resultTy);
    fprintf(out,";\n");

    /* The function wrapper */
    fprintf(out,"static ");
    ffiDeclareFun(line,e,FALSE,TRUE,argTys,resultTy);
    fprintf(out,"\n{\n");
    ffiDeclare(line,resultTy,"res",1);

    /* Push function pointer and arguments */
    fprintf(out,"    hugs->derefStablePtr4(fun1);\n");
    ffiPutList(line,argTys,"arg");

    /* Make the call and check for uncaught exception */
    /* ToDo: I'm not sure that exiting from the Hugs session is the right 
     * response to the Haskell function calling System.exit.
     */
    fprintf(out,"    if (hugs->run%s(%d)) {\n", isIO?"IO":"Id", length(argTys));
    fprintf(out,
            "        exit(hugs->getInt());\n"
            "    }\n"
            );
    fprintf(out,"    ");
    foreignGet(line,resultTy,"res",1);
    ffiReturn(resultTy,"res",1);

    /* Return result */
    fprintf(out,"}\n");

    ffiPrimProto(e,id);
    ffiPrimHeader(e,id);
    fprintf(out,
            "{\n"
            "    HugsStablePtr arg1 = hugs->makeStablePtr4();\n"
            "    void* thunk = hugs->mkThunk((HsFunPtr)%s,arg1);\n",
            textToStr(e)
            );
    fprintf(out,
            "    hugs->putAddr(thunk);\n"
            "    hugs->returnIO(hugs_root,1);\n"
            "}\n");
}

/* 
 * Generate C code for calling C functions from Haskell.
 * The code has to be compiled with a C compiler and dynamically
 * loaded.
 *
 * For example:
 * 
 *     foreign export "extnm" name :: Int -> Float -> IO Char
 * ==>
 *     
 *     HugsStablePtr hugs_stable_for_extnm = -1;
 *     char extnm(int arg1, float arg2);
 *     char extnm(int arg1, float arg2)
 *     {
 *         char  res1;
 *         hugs->putStablePtr(hugs_stable_for_extnm);
 *         hugs->putInt(arg1);                             
 *         hugs->putFloat(arg2);                             
 *         if (hugs->runIO(2)) {
 *             exit(hugs->getInt());
 *         }
 *         res1 = hugs->getChar();
 *         return res1;
 *     }
 *
 */
Void implementForeignExport(line,id,e,argTys,isIO,resultTy)
Int  line;
Int  id;
Text e;
List argTys;
Bool isIO;
Type resultTy; {
    /* ToDo: calling convention */

    /* Prototype for function we're generating */
    fprintf(out,"\nextern ");
    ffiDeclareFun(line,e,FALSE,FALSE,argTys,resultTy);
    fprintf(out,";\n");

    fprintf(out,"static HugsStablePtr hugs_stable_for_%s = -1;\n", textToStr(e));

    /* The function wrapper */
    ffiDeclareFun(line,e,FALSE,FALSE,argTys,resultTy);
    fprintf(out,"\n{\n");
    ffiDeclare(line,resultTy,"res",1);

    /* Push function pointer and arguments */
    fprintf(out,"    hugs->putStablePtr(hugs_stable_for_%s);\n", textToStr(e));
    ffiPutList(line,argTys,"arg");

    /* Make the call and check for uncaught exception */
    if (isIO) {
        /* ToDo: I'm not sure that exiting from the Hugs session is the right 
         * response to the Haskell function calling System.exit.
         */
        fprintf(out,"    if (hugs->runIO(%d)) {\n", length(argTys));
        fprintf(out,
                "        exit(hugs->getInt());\n"
                "    }\n"
                );
    } else {
        fprintf(out,"    hugs->ap(%d);\n", length(argTys));
    } 
    fprintf(out,"    ");
    foreignGet(line,resultTy,"res",1);
    ffiReturn(resultTy,"res",1);
    fprintf(out,"}\n");
}

/* 
 * Generate primitive for address of a C symbol.
 *
 * For example:
 * 
 *     foreign import "static & cid" name :: Addr
 * ==>
 *     
 *     static void hugsprim_name(HugsStackPtr);
 *     static void hugsprim_name(HugsStackPtr hugs_root)
 *     {
 *         extern int cid; // probably the wrong type but it doesn't matter 
 *         hugs->putAddr(&cid);
 *         hugs_returnId(1);
 *     }
 */
Void implementForeignImportLabel(line, id, fn, cid, n, ty)
Int  line;
Int  id;
Text fn;   /* Include file */
Text cid;  /* Function name */
Text n;    /* Haskell name */
Type ty; {
    ffiPrimProto(cid,id);
    ffiPrimHeader(cid,id);
    fprintf(out,"{\n");
    fprintf(out,"    extern int %s;\n",      textToStr(cid));
    fprintf(out,"    hugs->putAddr(&%s);\n", textToStr(cid));
    fprintf(out,"}\n");
}

/* ToDo: 
 * chain all foreign exports together and free at end of run?
 * copy GreenCard.h into Test.c? 
 */

/*-------------------------------------------------------------------------*/
