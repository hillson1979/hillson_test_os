/**
 * classlib.h — HillsonOS classlib 接口
 *
 * 简化版 classlib，仅提供编译所需的最小接口。
 * 大多数函数返回默认值或空操作。
 */
#ifndef JAMVM_CLASSLIB_H
#define JAMVM_CLASSLIB_H

#include "../../jamvm_src/src/jam.h"

/* ================================================================
 * Thread
 * ================================================================ */

static inline int classlibInitJavaThread(void *thread, Object *jlthread,
                                          Object *name, Object *group,
                                          char is_daemon, int priority) {
    (void)thread; (void)jlthread; (void)name; (void)group;
    (void)is_daemon; (void)priority;
    return 0;
}

static inline Object *classlibThreadPreInit(Class *thread_class,
                                             Class *thrdGrp_class) {
    (void)thread_class; (void)thrdGrp_class;
    return NULL;
}

#define classlibThreadPostInit() TRUE
#define classlibCreateJavaThread(t,j) 0

static inline void *classlibJThread2Thread(Object *jThread) {
    return NULL;
}

static inline Object *classlibMarkThreadTerminated(Object *jThread) {
    return NULL;
}

#define classlibGetThreadState(t)    0
#define classlibSetThreadState(t,s)  ((void)0)
#define classlibThreadName2Buff(t,b,l) ((void)0)

static inline int classlibInitialiseSignals(void) { return 1; }
static inline void classlibSignalThread(void *self) { (void)self; }

/* ================================================================
 * Class
 * ================================================================ */

#define classlibInitialiseClass()  TRUE

static inline void classlibCacheClassLoaderFields(Class *loader_class) {
    (void)loader_class;
}

static inline void *classlibLoaderTable(Object *class_loader) {
    (void)class_loader; return NULL;
}

static inline void *classlibCreateLoaderTable(Object *class_loader) {
    (void)class_loader; return NULL;
}

static inline char *classlibDefaultBootClassPath(void) {
    return (char*)"/java/classes";
}

static inline char *classlibDefaultEndorsedDirs(void) { return NULL; }
static inline char *classlibDefaultExtDirs(void) { return NULL; }

#define classlibBootClassPathOpt(args) classlibDefaultBootClassPath()

static inline void classlibNewLibraryUnloader(Object *loader, void *entry) {
    (void)loader; (void)entry;
}

static inline Object *classlibSkipReflectionLoader(Object *loader) {
    return loader;
}

static inline char *classlibExternalClassName(Class *class) {
    return class ? CLASS_CB(class)->name : NULL;
}

#ifdef JSR292
#define classlibInjectedFieldsCount(classname) 0
#define classlibFillInInjectedFields(classname, field) {}
#endif

/* ================================================================
 * Reflection
 * ================================================================ */

#define classlibInitReflection() TRUE

static inline Object *classlibCreateConstructorObject(MethodBlock *mb) {
    (void)mb; return NULL;
}
static inline Object *classlibCreateMethodObject(MethodBlock *mb) {
    (void)mb; return NULL;
}
static inline Object *classlibCreateFieldObject(FieldBlock *fb) {
    (void)fb; return NULL;
}
static inline MethodBlock *classlibMbFromReflectObject(Object *ob) {
    (void)ob; return NULL;
}
static inline FieldBlock *classlibFbFromReflectObject(Object *ob) {
    (void)ob; return NULL;
}

/* ================================================================
 * DLL
 * ================================================================ */

#define classlibInitialiseDll() TRUE
#define classlibDefaultBootDllPath() NULL

static inline void *classlibLookupLoadedDlls(char *name, Object *loader) {
    (void)name; (void)loader; return NULL;
}

/* ================================================================
 * JNI
 * ================================================================ */

#define classlibInitialiseJNI() TRUE

static inline Object *classlibNewDirectByteBuffer(void *addr, long long cap) {
    (void)addr; (void)cap; return NULL;
}
static inline void *classlibGetDirectBufferAddress(Object *buff) {
    (void)buff; return NULL;
}
static inline long long classlibGetDirectBufferCapacity(Object *buff) {
    (void)buff; return 0;
}
static inline Object *classlibCheckIfOnLoad(Frame *last) {
    (void)last; return NULL;
}

/* ================================================================
 * Properties
 * ================================================================ */

#define classlibAddDefaultProperties(properties) ((void)0)

static inline char *classlibDefaultJavaHome(void) {
    return (char*)"/java";
}

/* ================================================================
 * Access
 * ================================================================ */

#define classlibInitialiseAccess() TRUE
#define classlibAccessCheck(c1, c2) TRUE

/* ================================================================
 * Natives
 * ================================================================ */

#define classlibInitialiseNatives() TRUE

/* ================================================================
 * Exception
 * ================================================================ */

#define classlibInitialiseException(tc) TRUE

/* ================================================================
 * Frame
 * ================================================================ */

#define classlibInitialiseFrame() TRUE

static inline Frame *classlibGetCallerFrame(Frame *last, int depth) {
    (void)depth;
    return last;
}

static inline int classlibIsSkippedReflectFrame(Frame *frame) {
    (void)frame; return 0;
}

/* ================================================================
 * Shutdown
 * ================================================================ */

#define classlibVMShutdown() ((void)0)

/* ================================================================
 * Alloc/GC
 * ================================================================ */

#define classlibMarkSpecial(ob, mark)    ((void)0)
#define classlibPostCompact()            ((void)0)

static inline void classlibHandleUnmarkedSpecial(Object *ob) {
    (void)ob;
}

/* ================================================================
 * Method Handles (JSR 292)
 * ================================================================ */

#ifdef JSR292
static inline int isPolymorphicRef(Class *c, int cp_idx) {
    (void)c; (void)cp_idx; return 0;
}
#endif

#endif /* JAMVM_CLASSLIB_H */
