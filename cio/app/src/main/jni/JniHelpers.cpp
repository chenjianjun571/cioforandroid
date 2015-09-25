//
// Created by chenjianjun on 15/9/24.
//

#include "JniHelpers.h"
#include <asm/unistd.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace cio_jni
{
    static JavaVM* g_jvm = nullptr;
    static pthread_once_t g_jni_ptr_once = PTHREAD_ONCE_INIT;

    static pthread_key_t g_jni_ptr;

//    using icu::UnicodeString;

    JavaVM *GetJVM()
    {
        return g_jvm;
    }

    JNIEnv* GetEnv()
    {
        void* env = nullptr;
        g_jvm->GetEnv(&env, JNI_VERSION_1_6);
        return reinterpret_cast<JNIEnv*>(env);
    }

    static void ThreadDestructor(void* prev_jni_ptr)
    {
        if (!GetEnv())
            return;

        g_jvm->DetachCurrentThread();
    }

    static void CreateJNIPtrKey()
    {
        pthread_key_create(&g_jni_ptr, &ThreadDestructor);
    }

    jint InitGlobalJniVariables(JavaVM *jvm)
    {
        g_jvm = jvm;

        pthread_once(&g_jni_ptr_once, &CreateJNIPtrKey);

        JNIEnv* jni = nullptr;
        if (jvm->GetEnv(reinterpret_cast<void**>(&jni), JNI_VERSION_1_6) != JNI_OK)
            return -1;

        return JNI_VERSION_1_6;
    }

    static std::string GetThreadId()
    {
        char buf[21];  // Big enough to hold a kuint64max plus terminating NULL.
        snprintf(buf, sizeof(buf), "%lu", syscall(__NR_gettid));
        return std::string(buf);
    }

    static std::string GetThreadName()
    {
        char name[17];
        if (prctl(PR_GET_NAME, name) != 0) {
            LOGE("prctl(PR_GET_NAME) failed");
        }
        name[16] = '\0';
        return std::string(name);
    }

    JNIEnv* AttachCurrentThreadIfNeeded()
    {
        JNIEnv* jni = GetEnv();
        if (jni) return jni;

        pthread_getspecific(g_jni_ptr);

        std::string name(GetThreadName() + " - " + GetThreadId());
        JavaVMAttachArgs args;
        args.version = JNI_VERSION_1_6;
        args.name = &name[0];
        args.group = NULL;
        // Deal with difference in signatures between Oracle's jni.h and Android's.
#ifdef _JAVASOFT_JNI_H_  // Oracle's jni.h violates the JNI spec!
        void* env = NULL;
#else
        JNIEnv* env = NULL;
#endif
        g_jvm->AttachCurrentThread(&env, &args);
        jni = reinterpret_cast<JNIEnv*>(env);
        pthread_setspecific(g_jni_ptr, jni);

        return jni;
    }

    jlong jlongFromPointer(void* ptr)
    {
        static_assert(sizeof(intptr_t) <= sizeof(jlong), "Time to rethink the use of jlongs");
        // Going through intptr_t to be obvious about the definedness of the
        // conversion from pointer to integral type.  intptr_t to jlong is a standard
        // widening by the static_assert above.
        jlong ret = reinterpret_cast<intptr_t>(ptr);
        return ret;
    }

    jmethodID GetMethodID(JNIEnv* jni, jclass c, const std::string& name, const char* signature)
    {
        return jni->GetMethodID(c, name.c_str(), signature);
    }

    jmethodID GetStaticMethodID(JNIEnv* jni, jclass c, const char* name, const char* signature)
    {
        return jni->GetStaticMethodID(c, name, signature);
    }





    jfieldID GetFieldID(JNIEnv* jni, jclass c, const char* name, const char* signature)
    {
        return jni->GetFieldID(c, name, signature);
    }

    jclass GetObjectClass(JNIEnv* jni, jobject object)
    {
        return jni->GetObjectClass(object);
    }

    jobject GetObjectField(JNIEnv* jni, jobject object, jfieldID id)
    {
        return jni->GetObjectField(object, id);
    }

    jstring GetStringField(JNIEnv* jni, jobject object, jfieldID id)
    {
        return static_cast<jstring>(GetObjectField(jni, object, id));
    }

    jlong GetLongField(JNIEnv* jni, jobject object, jfieldID id)
    {
        return jni->GetLongField(object, id);
    }

    jint GetIntField(JNIEnv* jni, jobject object, jfieldID id)
    {
        return jni->GetIntField(object, id);
    }

    bool GetBooleanField(JNIEnv* jni, jobject object, jfieldID id)
    {
        return jni->GetBooleanField(object, id);
    }

// Java references to "null" can only be distinguished as such in C++ by
// creating a local reference, so this helper wraps that logic.
    bool IsNull(JNIEnv* jni, jobject obj)
    {
        ScopedLocalRefFrame local_ref_frame(jni);
        return jni->NewLocalRef(obj) == nullptr;
    }

//// Given a UTF-8 encoded |native| string return a new (UTF-16) jstring.
//    jstring JavaStringFromStdString(JNIEnv* jni, const std::string& native)
//    {
//        UnicodeString ustr(UnicodeString::fromUTF8(native));
//        return jni->NewString(ustr.getBuffer(), ustr.length());
//    }
//
//    // Given a (UTF-16) jstring return a new UTF-8 native string.
//    std::string JavaToStdString(JNIEnv* jni, const jstring& j_string)
//    {
//        const jchar* jchars = jni->GetStringChars(j_string, NULL);
//        UnicodeString ustr(jchars, jni->GetStringLength(j_string));
//        jni->ReleaseStringChars(j_string, jchars);
//        std::string ret;
//        return ustr.toUTF8String(ret);
//    }

    // Return the (singleton) Java Enum object corresponding to |index|;
    jobject JavaEnumFromIndex(JNIEnv* jni, jclass state_class,
                              const std::string& state_class_name, int index)
    {
        jmethodID state_values_id = GetStaticMethodID(
                jni, state_class, "values", ("()[L" + state_class_name  + ";").c_str());
        jobjectArray state_values = static_cast<jobjectArray>(
                jni->CallStaticObjectMethod(state_class, state_values_id));
        return jni->GetObjectArrayElement(state_values, index);
    }

    jobject NewGlobalRef(JNIEnv* jni, jobject o) {
        return jni->NewGlobalRef(o);
    }

    void DeleteGlobalRef(JNIEnv* jni, jobject o) {
        jni->DeleteGlobalRef(o);
    }

    WeakRef::WeakRef(JNIEnv* jni, jweak ref)
            : jni_(jni), obj_(jni_->NewLocalRef(ref)) {
    }
    WeakRef::~WeakRef() {
        if (obj_) {
            jni_->DeleteLocalRef(obj_);
        }
    }

// Scope Java local references to the lifetime of this object.  Use in all C++
// callbacks (i.e. entry points that don't originate in a Java callstack
// through a "native" method call).
    ScopedLocalRefFrame::ScopedLocalRefFrame(JNIEnv* jni) : jni_(jni) {
        jni_->PushLocalFrame(0);
    }
    ScopedLocalRefFrame::~ScopedLocalRefFrame() {
        jni_->PopLocalFrame(nullptr);
    }
}
