//
// Created by chenjianjun on 15/9/24.
//

#ifndef EXAMPLE_JNIHELPERS_H
#define EXAMPLE_JNIHELPERS_H
#include <jni.h>
#include <string>
#include <android/log.h>

#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "chenjianjun", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "chenjianjun", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "chenjianjun", __VA_ARGS__)

#define CHECK_EXCEPTION(jni)    \
  CHECK(!jni->ExceptionCheck()) \
      << (jni->ExceptionDescribe(), jni->ExceptionClear(), "")

#define CHECK_RELEASE(ptr) \
  CHECK_EQ(0, (ptr)->Release()) << "Unexpected refcount."

namespace cio_jni
{
    jint InitGlobalJniVariables(JavaVM *jvm);
    JNIEnv* GetEnv();
    JavaVM *GetJVM();

    JNIEnv* AttachCurrentThreadIfNeeded();
    jlong jlongFromPointer(void* ptr);

    jmethodID GetMethodID(JNIEnv* jni, jclass c, const std::string& name, const char* signature);
    jmethodID GetStaticMethodID(JNIEnv* jni, jclass c, const char* name, const char* signature);
    jfieldID GetFieldID(JNIEnv* jni, jclass c, const char* name, const char* signature);
    jclass GetObjectClass(JNIEnv* jni, jobject object);
    jobject GetObjectField(JNIEnv* jni, jobject object, jfieldID id);
    jstring GetStringField(JNIEnv* jni, jobject object, jfieldID id);
    jlong GetLongField(JNIEnv* jni, jobject object, jfieldID id);
    jint GetIntField(JNIEnv* jni, jobject object, jfieldID id);
    bool GetBooleanField(JNIEnv* jni, jobject object, jfieldID id);

    bool IsNull(JNIEnv* jni, jobject obj);
//    jstring JavaStringFromStdString(JNIEnv* jni, const std::string& native);
//    std::string JavaToStdString(JNIEnv* jni, const jstring& j_string);
    jobject JavaEnumFromIndex(JNIEnv* jni, jclass state_class, const std::string& state_class_name, int index);
    jobject NewGlobalRef(JNIEnv* jni, jobject o);
    void DeleteGlobalRef(JNIEnv* jni, jobject o);

    class WeakRef {
    public:
        WeakRef(JNIEnv* jni, jweak ref);
        ~WeakRef();
        jobject obj() { return obj_; }

    private:
        JNIEnv* const jni_;
        jobject const obj_;
    };

    class ScopedLocalRefFrame {
    public:
        explicit ScopedLocalRefFrame(JNIEnv* jni);
        ~ScopedLocalRefFrame();

    private:
        JNIEnv* jni_;
    };

    template<class T>// T is jclass, jobject, jintArray, etc.
    class ScopedGlobalRef {
    public:
        ScopedGlobalRef(JNIEnv* jni, T obj)
                :obj_(static_cast<T>(jni->NewGlobalRef(obj))) {}
        ~ScopedGlobalRef() {
            DeleteGlobalRef(AttachCurrentThreadIfNeeded(), obj_);
        }
        T operator*() const {
            return obj_;
        }
    private:
        T obj_;
    };
}


#endif //EXAMPLE_JNIHELPERS_H
