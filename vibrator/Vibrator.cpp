/*
 * Copyright (C) 2019 The Android Open Source Project
 * Copyright (C) 2023 StatiXOS
 * SPDX-License-Identifer: Apache-2.0
 */

#include "vibrator-impl/Vibrator.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <thread>
#include <map>
#include <fstream>
#include <string.h>

namespace aidl {
namespace android {
namespace hardware {
namespace vibrator {

std::map<int, std::string> haptic_nodes = {
    {1, "/sys/class/leds/vibrator/"},
    {2, "/sys/bus/i2c/drivers/aw8697_haptic/0-005a/"},
    {3, "/sys/bus/i2c/drivers/aw8697_haptic/1-005a/"},
    {4, "/sys/bus/i2c/drivers/aw8697_haptic/2-005a/"},
    {5, "/sys/bus/i2c/drivers/aw8697_haptic/3-005a/"},
    {6, "/sys/bus/i2c/drivers/awinic_haptic/0-005a/"},
    {7, "/sys/bus/i2c/drivers/awinic_haptic/1-005a/"},
    {8, "/sys/bus/i2c/drivers/awinic_haptic/2-005a/"},
    {9, "/sys/bus/i2c/drivers/awinic_haptic/3-005a/"},
    {10, "/sys/bus/i2c/drivers/awinic_haptic/9-005a/"},
    {11, "/sys/bus/i2c/drivers/aw86927_haptic/0-005a/"},
    {12, "/sys/bus/i2c/drivers/aw86927_haptic/1-005a/"},
    {13, "/sys/bus/i2c/drivers/aw86927_haptic/2-005a/"},
    {14, "/sys/bus/i2c/drivers/aw86927_haptic/3-005a/"},
};

static std::string HAPTIC_NODE;
static std::string HAPTIC_PROP_PREFIX = "vendor.vibrator.hal.";
static std::string HAPTIC_PROP_DURATION = "duration.";
static std::string HAPTIC_PROP_EFFECT_ID = "effect_id.";
static std::string HAPTIC_PROP_INDEX = "index.";
static std::string HAPTIC_PROP_MODE="mode.";

// Common haptic nodes
static std::string ACTIVATE_NODE = "activate";
static std::string ACTIVATE_MODE_NODE = "activate_mode";
static std::string EFFECT_ID_NODE = "effect_id";
static std::string INDEX_NODE = "index";
static std::string DURATION_NODE = "duration";

template <typename T>
static void write_haptic_node(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

template <typename T>
static inline int getProperty(std::string key, const T def) {
    return ::android::base::GetIntProperty(HAPTIC_PROP_PREFIX + key, def);
}

template <typename T>
static bool openNoCreate(const std::string &file, T *outStream) {
    auto mode = std::is_base_of_v<std::ostream, T> ? std::ios_base::out : std::ios_base::in;
    // Force 'in' mode to prevent file creation
    outStream->open(file, mode | std::ios_base::in);
    if (!*outStream) {
        LOG(ERROR) << "Failed to open %s (%d): %s" << file.c_str(), errno, strerror(errno);
        return false;
    }
    return true;
}

ndk::ScopedAStatus Vibrator::getCapabilities(int32_t* _aidl_return) {
    LOG(VERBOSE) << "Vibrator reporting capabilities";
    *_aidl_return = IVibrator::CAP_ON_CALLBACK | IVibrator::CAP_PERFORM_CALLBACK;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::off() {
    LOG(VERBOSE) << "Vibrator off";
    /* Reset index before triggering another set of haptics */
    write_haptic_node(HAPTIC_NODE + INDEX_NODE, 0);
    /* Reset mode before triggering another set of haptics */
    write_haptic_node(HAPTIC_NODE + ACTIVATE_MODE_NODE, 1);
    write_haptic_node(HAPTIC_NODE + ACTIVATE_NODE, 0);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::on(int32_t timeoutMs,
                                const std::shared_ptr<IVibratorCallback>& callback) {
    write_haptic_node(HAPTIC_NODE + DURATION_NODE, timeoutMs);
    write_haptic_node(HAPTIC_NODE + ACTIVATE_NODE, 1);

    if (callback != nullptr) {
        // Note that thread lambdas aren't using implicit capture [=], to avoid capturing "this",
        // which may be asynchronously destructed.
        // If "this" is needed, use [sharedThis = this->ref<Vibrator>()].
        std::thread([timeoutMs, callback] {
            usleep(timeoutMs * 1000);
        }).detach();
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::perform(Effect effect, EffectStrength strength,
                                     const std::shared_ptr<IVibratorCallback>& callback,
                                     int32_t* _aidl_return) {
    ndk::ScopedAStatus status;
    uint32_t index = 0;
    uint32_t timeMs = 0;
    uint32_t activate_mode = 1;
    uint32_t effect_id = 0;
    std::ofstream stream;

    for (auto& i: haptic_nodes) {
        std::string triggerNode = i.second + ACTIVATE_NODE;
        if (!openNoCreate(triggerNode, &stream))
            continue;
        else
            HAPTIC_NODE = i.second;
            break;
    }

    LOG(INFO) << "Vibrator perform";

    switch (effect) {
        case Effect::TICK:
            LOG(INFO) << "Vibrator effect set to TICK";
            activate_mode = getProperty(HAPTIC_PROP_MODE + "tick", 0);
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "tick", 2);
            index = getProperty(HAPTIC_PROP_INDEX + "tick", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "tick", 30);
            break;
        case Effect::TEXTURE_TICK:
            LOG(INFO) << "Vibrator effect set to TEXTURE_TICK";
            activate_mode = getProperty(HAPTIC_PROP_MODE + "texure_tick", 0);
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "texture_tick", 2);
            index = getProperty(HAPTIC_PROP_INDEX + "texure_tick", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "texture_tick", 30);
            break;
        case Effect::CLICK:
            LOG(INFO) << "Vibrator effect set to CLICK";
            activate_mode = getProperty(HAPTIC_PROP_MODE + "click", 0);
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "click", 0);
            index = getProperty(HAPTIC_PROP_INDEX + "click", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "click", 50);
            break;
        case Effect::HEAVY_CLICK:
            LOG(INFO) << "Vibrator effect set to HEAVY_CLICK";
            activate_mode = getProperty(HAPTIC_PROP_MODE + "heavy_click", 0);
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "heavy_click", 5);
            index = getProperty(HAPTIC_PROP_INDEX + "heavy_click", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "heavy_click", 70);
            break;
        case Effect::DOUBLE_CLICK:
            LOG(INFO) << "Vibrator effect set to DOUBLE_CLICK";
            /* double click isn't mapped as an effect_id in most awinic drivers, so
             * run the effect twice to get the effect.
             */
            activate_mode = getProperty(HAPTIC_PROP_MODE + "double_click", 0);
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "double_click", 1);
            index = getProperty(HAPTIC_PROP_INDEX + "double_click", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "double_click", 50);
            write_haptic_node(HAPTIC_NODE + ACTIVATE_MODE_NODE, activate_mode);
            write_haptic_node(HAPTIC_NODE + EFFECT_ID_NODE, effect_id);
            write_haptic_node(HAPTIC_NODE + INDEX_NODE, index);
            on(timeMs, nullptr);
            usleep(timeMs * 1000);
            /* set values again, as Vibrator::on() may trigger Vibrator::off() after playing the first
             * set of haptics, leading to reset of index, effect_id and activate_mode.
             */
            activate_mode = getProperty(HAPTIC_PROP_MODE + "double_click", 0);
            // change to tick for better differentiated haptic
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "tick", 2);
            index = getProperty(HAPTIC_PROP_INDEX + "double_click", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "double_click", 50);
            break;
        case Effect::THUD:
            LOG(INFO) << "Vibrator effect set to THUD";
            activate_mode = getProperty(HAPTIC_PROP_MODE + "thud", 0);
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "thud", 3);
            index = getProperty(HAPTIC_PROP_INDEX + "thud", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "thud", 70);
            break;
        case Effect::POP:
            LOG(INFO) << "Vibrator effect set to POP";
            activate_mode = getProperty(HAPTIC_PROP_MODE + "pop", 0);
            effect_id = getProperty(HAPTIC_PROP_EFFECT_ID + "pop", 4);
            index = getProperty(HAPTIC_PROP_INDEX + "pop", NULL);
            timeMs = getProperty(HAPTIC_PROP_DURATION + "pop", 30);
            break;
        default:
            return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    /* Setup mode */
    write_haptic_node(HAPTIC_NODE + ACTIVATE_MODE_NODE, activate_mode);

    /* Setup effect id */
    write_haptic_node(HAPTIC_NODE + EFFECT_ID_NODE, effect_id);

    /* Setup effect index */
    write_haptic_node(HAPTIC_NODE + INDEX_NODE, index);

    if (callback != nullptr) {
        std::thread([callback, timeMs] {
            usleep(timeMs * 1000);
            callback->onComplete();
        }).detach();
    }

    status = on(timeMs, nullptr);
    if (!status.isOk()) {
        return status;
    } else {
        *_aidl_return = timeMs;
        return ndk::ScopedAStatus::ok();
    }
}

ndk::ScopedAStatus Vibrator::getSupportedEffects(std::vector<Effect>* _aidl_return) {
    *_aidl_return = {
        Effect::TICK,
        Effect::TEXTURE_TICK,
        Effect::CLICK,
        Effect::HEAVY_CLICK,
        Effect::DOUBLE_CLICK,
        Effect::THUD,
        Effect::POP
    };

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus Vibrator::setAmplitude(float amplitude) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::setExternalControl(bool enabled) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionDelayMax(int32_t* maxDelayMs) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getCompositionSizeMax(int32_t* maxSize) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedPrimitives(std::vector<CompositePrimitive>* supported) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPrimitiveDuration(CompositePrimitive primitive,
                                                  int32_t* durationMs) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::compose(const std::vector<CompositeEffect>& composite,
                                     const std::shared_ptr<IVibratorCallback>& callback) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedAlwaysOnEffects(std::vector<Effect>* _aidl_return) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnEnable(int32_t id, Effect effect, EffectStrength strength) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::alwaysOnDisable(int32_t id) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getResonantFrequency(float *resonantFreqHz) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getQFactor(float *qFactor) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyResolution(float *freqResolutionHz) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getFrequencyMinimum(float *freqMinimumHz) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getBandwidthAmplitudeMap(std::vector<float> *_aidl_return) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwlePrimitiveDurationMax(int32_t *durationMs) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getPwleCompositionSizeMax(int32_t *maxSize) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::getSupportedBraking(std::vector<Braking> *supported) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus Vibrator::composePwle(const std::vector<PrimitivePwle> &composite,
                                         const std::shared_ptr<IVibratorCallback> &callback) {
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

}  // namespace vibrator
}  // namespace hardware
}  // namespace android
}  // namespace aidl
