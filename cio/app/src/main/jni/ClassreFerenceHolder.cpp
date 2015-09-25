//
// Created by chenjianjun on 15/9/24.
//
#include "ClassreFerenceHolder.h"
#include "JniHelpers.h"
#include <map>
#include <string>

namespace cio_jni
{
    class ClassReferenceHolder {
    public:
        explicit ClassReferenceHolder(JNIEnv* jni);
        ~ClassReferenceHolder();

        void FreeReferences(JNIEnv* jni);
        jclass GetClass(const std::string& name);

    private:
        void LoadClass(JNIEnv* jni, const std::string& name);

        std::map<std::string, jclass> classes_;
    };

    static ClassReferenceHolder* g_class_reference_holder = nullptr;

    void LoadGlobalClassReferenceHolder()
    {
        g_class_reference_holder = new ClassReferenceHolder(GetEnv());
    }

    void FreeGlobalClassReferenceHolder()
    {
        g_class_reference_holder->FreeReferences(AttachCurrentThreadIfNeeded());
        delete g_class_reference_holder;
        g_class_reference_holder = nullptr;
    }

    ClassReferenceHolder::ClassReferenceHolder(JNIEnv* jni)
    {
        LoadClass(jni, "org/cio/CIODelegate");
    }

    ClassReferenceHolder::~ClassReferenceHolder()
    {
        classes_.empty();
    }

    void ClassReferenceHolder::FreeReferences(JNIEnv* jni)
    {
        for (std::map<std::string, jclass>::const_iterator it = classes_.begin();
             it != classes_.end(); ++it) {
            jni->DeleteGlobalRef(it->second);
        }
        classes_.clear();
    }

    jclass ClassReferenceHolder::GetClass(const std::string& name)
    {
        std::map<std::string, jclass>::iterator it = classes_.find(name);
        if (it == classes_.end())
        {
            return nullptr;
        }
        return it->second;
    }

    void ClassReferenceHolder::LoadClass(JNIEnv* jni, const std::string& name)
    {
        jclass localRef = jni->FindClass(name.c_str());
        jclass globalRef = reinterpret_cast<jclass>(jni->NewGlobalRef(localRef));
        bool inserted = classes_.insert(std::make_pair(name, globalRef)).second;
    }

    jclass FindClass(JNIEnv* jni, const char* name)
    {
        return g_class_reference_holder->GetClass(name);
    }

}
