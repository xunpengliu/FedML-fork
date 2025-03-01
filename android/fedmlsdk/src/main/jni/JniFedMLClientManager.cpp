#ifndef FEDML_ANDROID_JNIFEDMLCLIENTMANAGER_CPP
#define FEDML_ANDROID_JNIFEDMLCLIENTMANAGER_CPP

#include <map>
#include <mutex>
#include "JniFedMLClientManager.h"
#include "jniAssist.h"
#include "FedMLClientManager.h"

// java callback object ref map
std::map<jlong, jobject> globalCallbackMap;
// globalCallbackMap lock
std::mutex globalCallbackMapMutex;

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT jlong JNICALL Java_ai_fedml_edge_nativemobilenn_NativeFedMLClientManager_create
(JNIEnv *, jobject) {
    LOGD("NativeFedMLClientManager.create");
    return reinterpret_cast<jlong >(new FedMLClientManager());
}

/*
 * Class:     ai_fedml_edge_nativemobilenn_NativeFedMLClientManager
 * Method:    release
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_ai_fedml_edge_nativemobilenn_NativeFedMLClientManager_release
        (JNIEnv *env, jobject, jlong ptr) {
    LOGD("NativeFedMLClientManager<%lx>.release", ptr);
    // add lock, protect globalCallbackMap concurrent access
    std::unique_lock<std::mutex> lock(globalCallbackMapMutex);
    // check callback ref exists
    auto find = globalCallbackMap.find(ptr);
    if (find != globalCallbackMap.end()) {
        // release callback ref
        jobject globalCallback = find->second;
        globalCallbackMap.erase(ptr);
        env->DeleteGlobalRef(globalCallback);
    }
    // unlock
    lock.unlock();
    auto *pFedMLClientManager = reinterpret_cast<FedMLClientManager *>(ptr);
    delete pFedMLClientManager;
}

/*
 * Class:     ai_fedml_edge_nativemobilenn_NativeFedMLClientManager
 * Method:    init
 * Signature: (ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;IIIDIIIILai/fedml/edge/nativemobilenn/TrainingCallback;)V
 */
JNIEXPORT void JNICALL Java_ai_fedml_edge_nativemobilenn_NativeFedMLClientManager_init
(JNIEnv *env, jobject , jlong ptr,
 jstring modelCachePath, jstring dataCachePath, jstring dataSet,
 jint trainSize, jint testSize, jint batchSizeNum, jdouble learningRate, jint epochNum,
 jobject trainingCallback) {
    // model and dataset path
    LOGD("===================JNIEXPORT void JNICALL Java_ai_fedml_edge_nativemobilenn_NativeFedMLClientManager_init=========");
    const char *modelPath = env->GetStringUTFChars(modelCachePath, nullptr);
    const char *dataPath = env->GetStringUTFChars(dataCachePath, nullptr);
    const char *datasetType = env->GetStringUTFChars(dataSet, nullptr);
    LOGD("NativeFedMLClientManager<%lx>.init(%s, %s, %s, %d, %d, %d, %f, %d)", ptr,
         modelPath, dataPath, datasetType, trainSize, testSize,
         batchSizeNum, learningRate, epochNum);
    // callback function
    jobject globalCallback = env->NewGlobalRef(trainingCallback);
    // add lock, protect globalCallbackMap concurrent access
    std::unique_lock<std::mutex> lock(globalCallbackMapMutex);
    globalCallbackMap[ptr] = globalCallback;
    // unlock
    lock.unlock();
    jmethodID onProgressMethodID = getMethodIdByNameAndSig(env, trainingCallback, "onProgress","(F)V");
    jmethodID onAccuracyMethodID = getMethodIdByNameAndSig(env, trainingCallback, "onAccuracy","(IF)V");
    jmethodID onLossMethodID = getMethodIdByNameAndSig(env, trainingCallback, "onLoss", "(IF)V");
    LOGD("NativeFedMLClientManager<%lx>.init onProgressMid=%p,onLossMid=%p,onAccuracyMid=%p", ptr,
         onProgressMethodID, onLossMethodID, onAccuracyMethodID);

    auto onProgressCallback = [ptr, env, onProgressMethodID](float progress) {
        // add lock, protect globalCallbackMap concurrent access
        std::unique_lock<std::mutex> lock(globalCallbackMapMutex);
        jobject callback = globalCallbackMap[ptr];
        // unlock
        lock.unlock();
        LOGD("NativeFedMLClientManager<%lx> <%p>.onProgressCallback(%f) env=%p onProgressMid=%p", ptr,
             callback, progress, env, onProgressMethodID);
        env->CallVoidMethod(callback, onProgressMethodID, (jfloat) progress);
    };

    auto onAccuracyCallback = [ptr, env, onAccuracyMethodID](int epoch, float acc) {
        // add lock, protect globalCallbackMap concurrent access
        std::unique_lock<std::mutex> lock(globalCallbackMapMutex);
        jobject callback = globalCallbackMap[ptr];
        // unlock
        lock.unlock();
        LOGD("NativeFedMLClientManager<%lx> <%p>.onAccuracyCallback(%d, %f) env=%p onAccuracyMid=%p", ptr,
             callback, epoch, acc, env, onAccuracyMethodID);
        env->CallVoidMethod(callback, onAccuracyMethodID, (jint) epoch, (jfloat) acc);
    };

    auto onLossCallback = [ptr, env, onLossMethodID](int epoch, float loss) {
        // add lock, protect globalCallbackMap concurrent access
        std::unique_lock<std::mutex> lock(globalCallbackMapMutex);
        jobject callback = globalCallbackMap[ptr];
        // unlock
        lock.unlock();
        LOGD("NativeFedMLClientManager<%lx> <%p>.onLossCallback(%d, %f) env=%p onLossMid=%p", ptr,
             callback, epoch, loss, env, onLossMethodID);
        env->CallVoidMethod(callback, onLossMethodID, (jint) epoch, (jfloat) loss);
    };

    // FedMLClientManager object ptr
    auto *pFedMLClientManager = reinterpret_cast<FedMLClientManager *>(ptr);
    pFedMLClientManager->init(modelPath, dataPath, datasetType,
                              (int) trainSize, (int) testSize, (int) batchSizeNum, (double) learningRate, (int) epochNum,
                              onProgressCallback, onAccuracyCallback, onLossCallback);
    LOGD("NativeFedMLClientManager<%lx>.initialed", ptr);
    onProgressCallback(0.01);
    env->ReleaseStringUTFChars(modelCachePath, modelPath);
    env->ReleaseStringUTFChars(dataCachePath, dataPath);
    env->ReleaseStringUTFChars(dataSet, datasetType);
}

/*
 * Class:     ai_fedml_edge_nativemobilenn_NativeFedMLClientManager
 * Method:    train
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ai_fedml_edge_nativemobilenn_NativeFedMLClientManager_train
(JNIEnv *env, jobject, jlong ptr) {
    LOGD("NativeFedMLClientManager<%lx>.train", ptr);
    auto *pFedMLClientManager = reinterpret_cast<FedMLClientManager *>(ptr);
    std::string result = pFedMLClientManager->train();
    return env->NewStringUTF(result.data());
}

/*
 * Class:     ai_fedml_edge_nativemobilenn_NativeFedMLClientManager
 * Method:    getEpochAndLoss
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ai_fedml_edge_nativemobilenn_NativeFedMLClientManager_getEpochAndLoss
(JNIEnv *env, jobject, jlong ptr) {
    LOGD("NativeFedMLClientManager<%lx>.getEpochAndLoss", ptr);
    auto *pFedMLClientManager = reinterpret_cast<FedMLClientManager *>(ptr);
    std::string result = pFedMLClientManager->getEpochAndLoss();
    return env->NewStringUTF(result.data());
}

/*
 * Class:     ai_fedml_edge_nativemobilenn_NativeFedMLClientManager
 * Method:    stopTraining
 * Signature: ()Z
 */
JNIEXPORT jboolean JNICALL Java_ai_fedml_edge_nativemobilenn_NativeFedMLClientManager_stopTraining
        (JNIEnv *, jobject, jlong ptr) {
LOGD("NativeFedMLClientManager<%lx>.stopTraining", ptr);
auto *pFedMLClientManager = reinterpret_cast<FedMLClientManager *>(ptr);
return pFedMLClientManager->stopTraining();
}

#ifdef __cplusplus
}
#endif
#endif //FEDML_ANDROID_JNIFEDMLCLIENTMANAGER_CPP
