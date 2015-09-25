//
// Created by chenjianjun on 15/9/18.
//

#include "org_cio_CIO.h"
#include "NetCenter.h"

static JavaVM* g_jvm = nullptr;
static NetCenter* g_NetCenter = nullptr;

// ##############################################jni实现#############################################
#ifdef __cplusplus
extern "C" {
#endif

jint JNIEXPORT JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
    jint ret = InitGlobalJniVariables(jvm);
    if (ret < 0)
        return -1;

    LoadGlobalClassReferenceHolder();

    return ret;
}

void JNIEXPORT JNICALL JNI_OnUnLoad(JavaVM *jvm, void *reserved) {
    FreeGlobalClassReferenceHolder();
}

//static jobject JavaEnumFromIndex(JNIEnv* jni, const std::string& state_class_fragment, int index)
//{
//    const std::string state_class = "org/cio/" + state_class_fragment;
//    return JavaEnumFromIndex(jni, FindClass(jni, state_class.c_str()), state_class, index);
//}

JNIEXPORT jboolean JNICALL Java_org_cio_CIO_InitWithDelegate
        (JNIEnv *jni, jobject, jobject j_observer)
{
    g_NetCenter = new(std::nothrow) NetCenter(jni, j_observer);
    if (nullptr == g_NetCenter)
    {
        return false;
    }

    if (FUNC_SUCCESS != g_NetCenter->NetInit())
    {
        return false;
    }

    return true;
}

/*
 * Class:     org_cio_CIO
 * Method:    Terminat
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_cio_CIO_Terminat(JNIEnv *, jobject)
{
    if (nullptr == g_NetCenter)
    {
        return;
    }

    g_NetCenter->NetExit();

    delete g_NetCenter;
    g_NetCenter = nullptr;
}

/*
 * Class:     org_cio_CIO
 * Method:    OpenIOChannel
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_cio_CIO_OpenIOChannel
        (JNIEnv *, jobject)
{
    if (nullptr == g_NetCenter)
    {
        return -1;
    }

    return g_NetCenter->CreateFD();
}

/*
 * Class:     org_cio_CIO
 * Method:    ConnectServiceWhitFD
 * Signature: (ILjava/lang/String;I)Z
 */
JNIEXPORT jboolean JNICALL Java_org_cio_CIO_ConnectServiceWhitFD
        (JNIEnv *env, jobject, jint fd, jstring hostname, jint hostport)
{
    if (nullptr == g_NetCenter)
    {
        return false;
    }

    const char *hostname_ = env->GetStringUTFChars(hostname, nullptr);
    if (g_NetCenter->ConnectServiceWhitFD(fd, hostname_, hostport))
    {
        env->ReleaseStringUTFChars(hostname, hostname_);
        return true;
    }

    env->ReleaseStringUTFChars(hostname, hostname_);

    return false;
}

/*
 * Class:     org_cio_CIO
 * Method:    SendDataWithFD
 * Signature: (I[CI)I
 */
JNIEXPORT jint JNICALL Java_org_cio_CIO_SendDataWithFD
        (JNIEnv *env, jobject, jint fd, jbyteArray buffer, jint len)
{
    jbyte *array = env->GetByteArrayElements(buffer, nullptr);
    if(array == nullptr){
        return 0;
    }

    jint ret = g_NetCenter->SendDataWithFD(fd, array, len);
    env->ReleaseByteArrayElements(buffer, array, 0);

    return ret;
}

/*
 * Class:     org_cio_CIO
 * Method:    CloseIOChannelWithFD
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_cio_CIO_CloseIOChannelWithFD
(JNIEnv *, jobject, jint fd)
{
    if (nullptr == g_NetCenter)
    {
        return;
    }

    g_NetCenter->CloseFD(fd);
}

#ifdef __cplusplus
}
#endif

