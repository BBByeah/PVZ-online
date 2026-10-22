#pragma once
#include <string>
#include <cstdint>
#include "../ModText.h"
#ifdef __ANDROID__
#include <SDL.h>
#include <jni.h>
namespace LocalHostControl {
inline bool Copy(const std::string& text){return SDL_SetClipboardText(text.c_str())==0;}
inline int RawPort() {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());if(!env)return 0;
    auto activity=static_cast<jobject>(SDL_AndroidGetActivity());if(!activity)return 0;
    auto type=env->GetObjectClass(activity);auto method=env->GetMethodID(type,"getLanHostPort","()I");
    int port=method?env->CallIntMethod(activity,method):0;
    if(env->ExceptionCheck()){env->ExceptionClear();port=0;}
    env->DeleteLocalRef(type);env->DeleteLocalRef(activity);return port;
}
inline uint16_t Port(){int port=RawPort();return port>0&&port<=65535?uint16_t(port):0;}
inline bool Starting(){return RawPort()<0;}
inline bool Alive(){return RawPort()!=0;}
inline std::string Error() {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());if(!env)return ModText::Tr("本机托管已停止，请重新开房。","Hosting stopped. Create a new room.");
    auto activity=static_cast<jobject>(SDL_AndroidGetActivity());if(!activity)return ModText::Tr("本机托管已停止，请重新开房。","Hosting stopped. Create a new room.");
    auto type=env->GetObjectClass(activity);auto method=env->GetMethodID(type,"getLanHostError","()Ljava/lang/String;");
    auto value=method?static_cast<jstring>(env->CallObjectMethod(activity,method)):nullptr;
    std::string error;
    if(env->ExceptionCheck())env->ExceptionClear();
    if(value){auto bytes=env->GetStringUTFChars(value,nullptr);if(bytes){error=bytes;env->ReleaseStringUTFChars(value,bytes);}env->DeleteLocalRef(value);}
    env->DeleteLocalRef(type);env->DeleteLocalRef(activity);
    if(error.empty())return ModText::Tr("无法启动本机托管，请检查端口后重试。","Could not start local hosting. Check the port and retry.");
    return ModText::IsChinese()?"无法启动本机托管，请检查端口后重试。":error;
}
inline bool Start(uint16_t port,std::string& error) {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());if(!env){error=ModText::Tr("安卓托管暂不可用。","Android hosting unavailable.");return false;}
    auto activity=static_cast<jobject>(SDL_AndroidGetActivity());if(!activity){error=ModText::Tr("安卓托管暂不可用。","Android hosting unavailable.");return false;}
    auto type=env->GetObjectClass(activity);auto method=env->GetMethodID(type,"startLanHost","(I)V");
    if(method)env->CallVoidMethod(activity,method,jint(port));
    bool ok=method&&!env->ExceptionCheck();if(env->ExceptionCheck())env->ExceptionClear();
    env->DeleteLocalRef(type);env->DeleteLocalRef(activity);if(!ok)error=ModText::Tr("无法启动本机托管。","Could not start local hosting.");return ok;
}
inline void Stop() {
    auto* env=static_cast<JNIEnv*>(SDL_AndroidGetJNIEnv());if(!env)return;
    auto activity=static_cast<jobject>(SDL_AndroidGetActivity());if(!activity)return;
    auto type=env->GetObjectClass(activity);auto method=env->GetMethodID(type,"stopLanHost","()V");
    if(method)env->CallVoidMethod(activity,method);
    if(env->ExceptionCheck())env->ExceptionClear();env->DeleteLocalRef(type);env->DeleteLocalRef(activity);
}
}
#else
namespace LocalHostControl {
inline bool Copy(const std::string&){return false;}
inline uint16_t Port(){return 0;}inline bool Starting(){return false;}inline bool Alive(){return false;}
inline void Stop(){}inline bool Start(uint16_t,std::string& error){error="Local host requires Android.";return false;}
inline std::string Error(){return "Local host requires Android.";}
}
#endif
