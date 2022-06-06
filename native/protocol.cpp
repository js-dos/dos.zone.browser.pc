//
// Created by caiiiycuk on 22.03.21.
//
#include <napi.h>

#include <cstring>
#include <string>
#include <sstream>
#include <stdio.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#ifdef WIN32
#include <direct.h>
#include "protocol_dyn.h"

#define client_frame_set_size client_frame_set_size_impl 
#define client_frame_update_lines client_frame_update_lines_impl 
#define client_sound_init client_sound_init_impl 
#define client_sound_push client_sound_push_impl 
#define client_stdout client_stdout_impl 
#define client_log client_log_impl 
#define client_warn client_warn_impl 
#define client_error client_error_impl 
#define client_network_connected client_network_connected_impl 
#define client_network_disconnected client_network_disconnected_impl 
#define client_tick client_tick_impl 

#define server_run server_run_dyn
#define server_add_key server_add_key_dyn
#define server_mouse_moved server_mouse_moved_dyn
#define server_mouse_button server_mouse_button_dyn
#define server_mouse_sync server_mouse_sync_dyn
#define server_pause server_pause_dyn
#define server_resume server_resume_dyn
#define server_mute server_mute_dyn
#define server_unmute server_unmute_dyn
#define server_exit server_exit_dyn
#define server_network_connect server_network_connect_dyn
#define server_network_disconnect server_network_disconnect_dyn

#define zip_set_on_progress zip_set_on_progress_dyn
#define zip_from_fs zip_from_fs_dyn
#define zipfile_to_fs zipfile_to_fs_dyn
#define get_changes_mtime_ms get_changes_mtime_ms_dyn
#else
#include <protocol.h>
#include <unistd.h>
#include <jsdos-libzip.h>
#endif

class jsonstream {
    bool keyNext;
    std::string contents;
public:
    jsonstream();

    jsonstream &append(const char* str, bool addQuotes);
    jsonstream &operator<<(const char* str);
    jsonstream &operator<<(const std::string &str);
    jsonstream &operator<<(unsigned int v);
    jsonstream &operator<<(int v);
    jsonstream &operator<<(long v);
    jsonstream &operator<<(float v);
    jsonstream &operator<<(double v);

    const char *c_str() const;
    const std::string &std_str() const;

};

Napi::ThreadSafeFunction serverMessage;
Napi::ThreadSafeFunction soundPush;

std::string sessionId = "";
const std::string baseDir = "data";
double fsCreatedAt = 0;

std::atomic_int volume(256);

std::atomic_int frameWidth(0);
std::atomic_int frameHeight(0);
std::atomic_bool frameChanged;
std::mutex frameMutex;
char* frameRGBA = nullptr;
char* framePayload = nullptr;

std::mutex changesMutex;
std::atomic_bool postChanges(false);
ZipArchive changesPtr = nullptr;
int changesLength = 0;
char* changesData = nullptr;

std::mutex soundMutex;
constexpr int32_t soundBufferSize = 4096 * 4;
int32_t soundBufferUsed = 0;
float soundBuffer[soundBufferSize];
ma_device device;
bool useWebAudio = false;

std::mutex networkMutex;
std::atomic_bool networkChanges(false);
bool networkConnect = false;
int networkType = 0;
std::string address;
int port;

std::mutex inputMutex;
std::atomic_bool inputChanges(false);
enum InputType {
    KEY, MOUSE_MOVE, MOUSE_BUTTON
};
struct InputRecord {
    InputType type;
    KBD_KEYS key;
    bool pressed;
    uint64_t timeMs;
    float x;
    float y;
    bool relative;
    int button;
};
std::vector<InputRecord> inputs;
std::thread nativeThread;

inline bool exists(const char* name) {
#ifdef WIN32
    return GetFileAttributes(name) != INVALID_FILE_ATTRIBUTES;
#else
    return access(name, F_OK) != -1;
#endif
}

void postMessage(const jsonstream& stream) {
    std::string *payload = new std::string(stream.c_str());
    serverMessage.NonBlockingCall(payload, [](Napi::Env env, Napi::Function jsCallback, std::string *value) {
        jsCallback.Call({Napi::String::New(env, *value)});
        delete value;
    });
}

void client_frame_set_size(int width, int height) {
    std::lock_guard<std::mutex> g(frameMutex);

    frameWidth = width;
    frameHeight = height;

    if (framePayload != nullptr) {
        free(framePayload);
    }
    framePayload = (char*) malloc(frameWidth * frameHeight * 3 + frameHeight);
    memset(framePayload, 1, frameHeight);

    jsonstream stream;
    stream
        << "sessionId" << sessionId
        << "name" << "ws-frame-set-size"
        << "width" << width
        << "height" << height;
    postMessage(stream);
}

void client_frame_update_lines(uint32_t* lines, uint32_t batchCount, void* rgba) {
    std::lock_guard<std::mutex> g(frameMutex);

    if (frameRGBA != rgba) {
        frameRGBA = (char *) rgba;
    }

    frameChanged = true;

    for (uint32_t i = 0; i < batchCount; ++i) {
        uint32_t base = i * 3;
        uint32_t start = lines[base];
        uint32_t count = lines[base + 1];
        memset(framePayload + start, 1, count);
    }
}

void ma_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 numFrames) {
    std::lock_guard<std::mutex> g(soundMutex);

    static bool started = false;
    if (!started) {
        if (soundBufferUsed < soundBufferSize / 4) {
            return;
        }

        started = true;
    }

    int targetVolume = volume;
    auto copySize = std::min<int32_t>(numFrames, soundBufferUsed);
    if (targetVolume > 250) {
        memcpy(pOutput, soundBuffer, copySize * 4);
    } else {
        float fVolume = targetVolume / 256.0f;
        float *dst = (float*) pOutput;
        float *src = soundBuffer;
        for (int i = 0; i < copySize; ++i) {
            *dst = *src * fVolume;
            ++src;
            ++dst;
        }
    }

    if (copySize < soundBufferUsed) {
        auto rest = soundBufferUsed - copySize;
        memmove(soundBuffer, soundBuffer + copySize, rest * 4);
        soundBufferUsed = rest;
    } else {
        soundBufferUsed = 0;
    }
}

void client_sound_init(int freq) {
    ma_device_config config  = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32; 
    config.playback.channels = 1;             
    config.sampleRate        = freq;          
    config.dataCallback      = ma_callback;  

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        printf("-- unable to start mini audio, fallback to web audio --\n");
        useWebAudio = true;
        jsonstream stream;
        stream
            << "sessionId" << sessionId
            << "name" << "ws-sound-init"
            << "freq" << freq;
        postMessage(stream);
        return;
    }

    ma_device_start(&device);
    printf("-- miniaudio started --\n");

    jsonstream stream;
    stream
            << "sessionId" << sessionId
            << "name" << "ws-sound-init"
            << "freq" << 0; // turn off web audio
    postMessage(stream);
}

void client_sound_push(const float* samples, int num_samples) {
    std::lock_guard<std::mutex> g(soundMutex);
    if (num_samples > soundBufferSize - soundBufferUsed) {
        return;
    }

    memcpy(soundBuffer + soundBufferUsed, samples, num_samples * sizeof(float));
    soundBufferUsed += num_samples;

    if (!useWebAudio) {
        return;
    }

    soundPush.NonBlockingCall([](Napi::Env env, Napi::Function jsCallback) {
        std::lock_guard<std::mutex> g(soundMutex);
        if (soundBufferUsed == 0) {
            return;
        }

        auto samples = Napi::ArrayBuffer::New(env, soundBufferUsed * sizeof(float));
        memcpy(samples.Data(), soundBuffer, soundBufferUsed * sizeof(float));
        jsCallback.Call({samples});
        soundBufferUsed = 0;
    });
}

void client_stdout(const char* data, uint32_t amount) {
    if (amount == 0 || strlen(data) == 0) {
        return;
    }

    for (auto i = 0; i < amount; ++i) {
        if (data[i] == '\\') {
            const_cast<char*>(data)[i] = '/';
        }
    }

    jsonstream stream;
    stream
            << "sessionId" << sessionId
            << "name" << "ws-stdout"
            << "message" << data;
    postMessage(stream);
}

void client_log(const char* tag, const char* message) {
    jsonstream stream;
    stream
            << "sessionId" << sessionId
            << "name" << "ws-log"
            << "tag" << tag
            << "message" << message;
    postMessage(stream);
}

void client_warn(const char* tag, const char* message) {
    jsonstream stream;
    stream
            << "sessionId" << sessionId
            << "name" << "ws-warn"
            << "tag" << tag
            << "message" << message;
    postMessage(stream);
}

void client_error(const char* tag, const char* message) {
    jsonstream stream;
    stream
            << "sessionId" << sessionId
            << "name" << "ws-err"
            << "tag" << tag
            << "message" << message;
    postMessage(stream);
}

Napi::Number getFrameWidth(const Napi::CallbackInfo& info) {
    return {Napi::Number::New(info.Env(), frameWidth)};
}

Napi::Number getFrameHeight(const Napi::CallbackInfo& info) {
    return {Napi::Number::New(info.Env(), frameHeight)};
}

Napi::Number updateFrame(const Napi::CallbackInfo& info) {
    auto outcome = info[0].As<Napi::ArrayBuffer>();
    std::lock_guard<std::mutex> g(frameMutex);

    if (framePayload == nullptr || frameRGBA == nullptr ||
        frameWidth == 0 || frameHeight == 0 ||
        !frameChanged) {
        return {Napi::Number::New(info.Env(), 0)};
    }

    frameChanged = false;

    int pitch = frameWidth * 3;
    int offset = frameHeight;
    for (int l = 0; l < frameHeight; ++l) {
        if (framePayload[l] == 0) {
            continue;
        }

        auto rgb = framePayload + offset;
        auto rgbEnd = rgb + pitch;
        auto rgba = frameRGBA + (l * frameWidth * 4);

        while (rgb != rgbEnd) {
            *(rgb++) = *(rgba++);
            *(rgb++) = *(rgba++);
            *(rgb++) = *(rgba++);
            ++rgba;
        }

        offset += pitch;
    }

    if (offset == frameHeight) {
        return {Napi::Number::New(info.Env(), 0)};
    }

    memcpy(outcome.Data(), framePayload, offset);
    memset(framePayload, 0, frameHeight);

    return {Napi::Number::New(info.Env(), offset)};
}

void sendMessage(const Napi::CallbackInfo& info) {
    std::string payload = info[0].As<Napi::String>().Utf8Value();

    std::istringstream payloadStream(payload);
    jsonstream stream;

    std::string name;
    payloadStream >> name;

    if (name == "wc-install") {
        payloadStream
            >> sessionId;

        stream
            << "name" << "ws-ready"
            << "sessionId" << sessionId;

        postMessage(stream);
    } else if (name == "wc-run") {
        nativeThread = std::thread([]() {
            jsonstream stream;
#ifdef WIN32
            _chdir(baseDir.c_str());
#else
            chdir(baseDir.c_str());
#endif

            if (exists("bundle_0.zip")) {
                zip_set_on_progress([](const char* file, int extracted, int total) {
                    jsonstream progress;
                    progress
                        << "name" << "ws-extract-progress"
                        << "index" << 0
                        << "file" << file
                        << "extracted" << extracted
                        << "count" << total
                        << "sessionId" << sessionId;
                    postMessage(progress);
                });
                zipfile_to_fs("bundle_0.zip");
                zip_set_on_progress(nullptr);
                remove("bundle_0.zip");
            }

            fsCreatedAt = get_changes_mtime_ms();

            std::this_thread::sleep_for(std::chrono::seconds(2));

            if (exists("bundle_1.zip")) {
                zipfile_to_fs("bundle_1.zip");
                remove("bundle_1.zip");
            }

            stream
                << "name" << "ws-server-ready"
                << "sessionId" << sessionId;

            postMessage(stream);
        
            server_run();
            if (!useWebAudio) {
                ma_device_uninit(&device); 
            }
            {
                jsonstream exit;
                exit
                    << "name" << "ws-exit"
                    << "sessionId" << sessionId;
                postMessage(exit);
            }
        });
    } else if (name == "wc-pause") {
        server_pause();
    } else if (name == "wc-resume") {
        server_resume();
    } else if (name == "wc-mute") {
        server_mute();
    } else if (name == "wc-unmute") {
        server_unmute();
    } else if (name == "wc-exit") {
        server_exit();
    } else if (name == "wc-pack-fs-to-bundle") {
        postChanges = true;
    } else if (name == "wc-connect") {
        std::lock_guard<std::mutex> g(networkMutex);
        payloadStream >> sessionId
                      >> networkType
                      >> address
                      >> port;
        networkConnect = true;
        networkChanges = true;
    } else if (name == "wc-disconnect") {
        std::lock_guard<std::mutex> g(networkMutex);
        payloadStream >> sessionId
                      >> networkType;
        networkConnect = false;
        networkChanges = true;
    } else {
        // should not happen
        printf("unhandled client message '%s'\n", name.c_str());
        abort();
    }
}

void addKey(const Napi::CallbackInfo& info) {
    auto key = info[0].As<Napi::Number>().Int32Value();
    auto pressed = info[1].As<Napi::Number>().Int32Value();
    uint64_t timeMs = info[2].As<Napi::Number>().Int64Value();
    std::lock_guard<std::mutex> g(inputMutex);

    inputs.push_back(InputRecord {
       .type = KEY,
       .key = (KBD_KEYS) key,
       .pressed = pressed == 1,
       .timeMs = timeMs,
    });
    inputChanges = true;
}

void setVolume(const Napi::CallbackInfo& info) {
    auto fVolume = info[0].As<Napi::Number>().FloatValue();
    volume = fVolume * 256;
}

void mouseMove(const Napi::CallbackInfo& info) {
    auto x = info[0].As<Napi::Number>().FloatValue();
    auto y = info[1].As<Napi::Number>().FloatValue();
    auto relative = info[2].As<Napi::Boolean>().Value();
    uint64_t timeMs = info[3].As<Napi::Number>().Int64Value();
    std::lock_guard<std::mutex> g(inputMutex);

    inputs.push_back(InputRecord {
            .type = MOUSE_MOVE,
            .timeMs = timeMs,
            .x = x,
            .y = y,
            .relative = relative == 1,
    });
    inputChanges = true;
}

void mouseButton(const Napi::CallbackInfo& info) {
    auto button = info[0].As<Napi::Number>().Int32Value();
    auto pressed = info[1].As<Napi::Number>().Int32Value();
    uint64_t timeMs = info[2].As<Napi::Number>().Int64Value();
    std::lock_guard<std::mutex> g(inputMutex);

    inputs.push_back(InputRecord {
            .type = MOUSE_BUTTON,
            .pressed = pressed == 1,
            .timeMs = timeMs,
            .button = button,
    });
    inputChanges = true;
}


Napi::Number getChangesSize(const Napi::CallbackInfo& info) {
    std::lock_guard<std::mutex> g(changesMutex);
    return {Napi::Number::New(info.Env(), changesLength)};
}

void getChanges(const Napi::CallbackInfo& info) {
    auto outcome = info[0].As<Napi::ArrayBuffer>();
    std::lock_guard<std::mutex> g(changesMutex);

    if (changesPtr == nullptr || changesData == nullptr || changesLength == 0) {
        return;
    }

    memcpy(outcome.Data(), changesData, changesLength);

    // TODO: understand why it segfaults on windows
#ifdef FREECHANGES_PTR
    free(changesPtr);
#endif

    changesPtr = nullptr;
    changesLength = 0;
    changesData = nullptr;
}

// json stream
jsonstream::jsonstream() : keyNext(true) {}

jsonstream &jsonstream::operator<<(unsigned int v) {
    return this->append(std::to_string(v).c_str(), false);
}

jsonstream &jsonstream::operator<<(int v) {
    return this->append(std::to_string(v).c_str(), false);
}

jsonstream &jsonstream::operator<<(long v) {
    return this->append(std::to_string(v).c_str(), false);
}

jsonstream &jsonstream::operator<<(float v) {
    return this->append(std::to_string(v).c_str(), false);
}

jsonstream &jsonstream::operator<<(double v) {
    return this->append(std::to_string(v).c_str(), false);
}

jsonstream &jsonstream::operator<<(const std::string &str) {
    return this->append(str.c_str(), true);
}

jsonstream &jsonstream::operator<<(const char *str) {
    return this->append(str, true);
}

jsonstream &jsonstream::append(const char* str, bool addQuotes) {
    if (keyNext || addQuotes) {
        contents += "\"";
    }

    contents += str;

    if (keyNext) {
        contents += "\":";
    } else if (addQuotes) {
        contents += "\",";
    } else {
        contents += ",";
    }

    keyNext = !keyNext;
    return *this;
}

const char *jsonstream::c_str() const {
    return contents.c_str();
}

const std::string &jsonstream::std_str() const {
    return contents;
}

void client_network_connected(NetworkType networkType, const char* address, uint32_t port) {
    jsonstream stream;
    stream
            << "sessionId" << sessionId
            << "name" << "ws-connected"
            << "networkType" << (int) networkType
            << "address" << address
            << "port" << port;
    postMessage(stream);
}
void client_network_disconnected(NetworkType networkType) {
    jsonstream stream;
    stream
        << "sessionId" << sessionId
        << "name" << "ws-disconnected"
        << "networkType" << (int) networkType;
    postMessage(stream);
}

void client_tick() {
    if (inputChanges && inputMutex.try_lock()) {
        if (inputChanges) {
            inputChanges = false;
            std::vector<InputRecord> copy = inputs;
            inputs.clear();
            inputMutex.unlock();

            for (const auto& next: copy) {
                switch (next.type) {
                    case KEY:
                        server_add_key(next.key, next.pressed, next.timeMs);
                        break;
                    case MOUSE_MOVE:
                        server_mouse_moved(next.x, next.y, next.relative, next.timeMs);
                        break;
                    case MOUSE_BUTTON:
                        server_mouse_button(next.button, next.pressed, next.timeMs);
                        break;
                    default:
                        abort();
                }
            }
        } else {
            inputMutex.unlock();
        }
    }

    if (postChanges && changesMutex.try_lock()) {
        if (postChanges) {
            postChanges = false;
            
            changesPtr = zip_from_fs(fsCreatedAt);
            changesLength = ((uint32_t *) changesPtr)[0];
            changesData = (char *) changesPtr + sizeof(uint32_t);
            
            changesMutex.unlock();

            jsonstream stream;
            stream
                    << "name" << "ws-persist"
                    << "sessionId" << sessionId;
            postMessage(stream);
        } else {
            changesMutex.unlock();
        }
    }

    if (networkChanges && networkMutex.try_lock()) {
        if (networkChanges) {
            networkChanges = false;
            auto cNetworkType = (NetworkType) networkType;
            auto cAddress = address;
            auto cPort = port;
            auto cNetworkConnect = networkConnect;
            networkMutex.unlock();

            if (cNetworkConnect) {
                auto index = cAddress.find(".jj.dos.zone");
                if (index != std::string::npos) {
                    cAddress = cAddress.substr(0, index);
                    for (char & cAddres : cAddress) {
                        if (cAddres == '_') {
                            cAddres = '.';
                        }
                    }
                }
                server_network_connect(cNetworkType, cAddress.c_str(), cPort);
            } else {
                server_network_disconnect(cNetworkType);
            }
        } else {
            networkMutex.unlock();
        }

    }
}

void registerCallbacks(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
  
    serverMessage = Napi::ThreadSafeFunction::New(
        env,
        info[0].As<Napi::Function>(), // JavaScript function called asynchronously
        "serverMessage",              // Name
        0,                            // Unlimited queue
        1,                            // Only one thread will use this initially
        []( Napi::Env ) {             // Finalizer used to clean threads up
            nativeThread.join();
        }
    );
    
    soundPush = Napi::ThreadSafeFunction::New(
        env,
        info[1].As<Napi::Function>(), // JavaScript function called asynchronously
        "soundPush",                  // Name
        0,                            // Unlimited queue
        1,                            // Only one thread will use this initially
        []( Napi::Env ) {             // Finalizer used to clean threads up
            nativeThread.join();
        }
    );
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
#ifdef WIN32
  set_client_frame_set_size(client_frame_set_size_impl);
  set_client_frame_update_lines(client_frame_update_lines_impl);
  set_client_sound_init(client_sound_init_impl);
  set_client_sound_push(client_sound_push_impl);
  set_client_stdout(client_stdout_impl);
  set_client_log(client_log_impl);
  set_client_warn(client_warn_impl);
  set_client_error(client_error_impl);
  set_client_network_connected(client_network_connected_impl);
  set_client_network_disconnected(client_network_disconnected_impl);
  set_client_tick(client_tick_impl);
#endif
  printf("-- emulators new --\n");
  exports.Set("sendMessage", Napi::Function::New(env, sendMessage));
  exports.Set("getFrameWidth", Napi::Function::New(env, getFrameWidth));
  exports.Set("getFrameHeight", Napi::Function::New(env, getFrameHeight));
  exports.Set("updateFrame", Napi::Function::New(env, updateFrame));
  exports.Set("addKey", Napi::Function::New(env, addKey));
  exports.Set("mouseMove", Napi::Function::New(env, mouseMove));
  exports.Set("mouseButton", Napi::Function::New(env, mouseButton));
  exports.Set("getChangesSize", Napi::Function::New(env, getChangesSize));
  exports.Set("getChanges", Napi::Function::New(env, getChanges));
  exports.Set("registerCallbacks", Napi::Function::New(env, registerCallbacks));
  exports.Set("setVolume", Napi::Function::New(env, setVolume));
  return exports;
}

NODE_API_MODULE(emulators, Init)
