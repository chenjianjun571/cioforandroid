//
// Created by chenjianjun on 15/9/24.
//

#ifndef EXAMPLE_CLASSREFERENCEHOLDER_H
#define EXAMPLE_CLASSREFERENCEHOLDER_H

#include <jni.h>

namespace cio_jni {

    // LoadGlobalClassReferenceHolder must be called in JNI_OnLoad.
    void LoadGlobalClassReferenceHolder();
    // FreeGlobalClassReferenceHolder must be called in JNI_UnLoad.
    void FreeGlobalClassReferenceHolder();

    jclass FindClass(JNIEnv* jni, const char* name);

    // Convenience macro defining JNI-accessible methods in the org.webrtc package.
    // Eliminates unnecessary boilerplate and line-wraps, reducing visual clutter.
#define JOW(rettype, name) extern "C" rettype JNIEXPORT JNICALL Java_org_cio_##name
};


#endif //EXAMPLE_CLASSREFERENCEHOLDER_H
