//
// Created by caiiiycuk on 22.03.21.
//
#include <napi.h>

#include <protocol.h>

#include <cstring>
#include <string>
#include <sstream>
#include <unistd.h>
#include <stdio.h>
#include <jsdos-libzip.h>

#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

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

std::string sessionId = "";
const std::string baseDir = "data";
double fsCreatedAt = 0;

std::atomic_int frameWidth(0);
std::atomic_int frameHeight(0);
std::atomic_bool frameChanged;
std::mutex frameMutex;
char* frameRGBA = nullptr;
char* framePayload = nullptr;

std::mutex changesMutex;
std::atomic_bool postChanges(false);
int changesLength = 0;
char* changesData = nullptr;

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
    return access(name, F_OK) != -1;
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

void client_sound_init(int freq) {
    // TODO
    jsonstream stream;
    stream
            << "sessionId" << sessionId
            << "name" << "ws-sound-init"
            << "freq" << 0; // turn off web audio
    postMessage(stream);
}

void client_sound_push(const float* samples, int num_samples) {
    // TODO
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
            chdir(baseDir.c_str());

            if (exists("bundle_0.zip")) {
                zipfile_to_fs("bundle_0.zip");
                remove("bundle_0.zip");
            }

            fsCreatedAt = get_changes_mtime_ms();

            std::this_thread::sleep_for(std::chrono::seconds(1));

            if (exists("bundle_1.zip")) {
                zipfile_to_fs("bundle_1.zip");
                remove("bundle_1.zip");
            }

            stream
                << "name" << "ws-server-ready"
                << "sessionId" << sessionId;

            postMessage(stream);
        
            server_run();

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
    return {Napi::Number::New(info.Env(), changesLength)};
}

void getChanges(const Napi::CallbackInfo& info) {
    auto outcome = info[0].As<Napi::ArrayBuffer>();
    std::lock_guard<std::mutex> g(changesMutex);

    if (changesData == nullptr || changesLength == 0) {
        return;
    }

    memcpy(outcome.Data(), changesData, changesLength);
    free(changesData);
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

void server_loop() {
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

            auto packed = zip_from_fs(fsCreatedAt);
            changesLength = ((uint32_t *) packed)[0];
            changesData = (char *) packed + 4;

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

void registerServerMessage(const Napi::CallbackInfo &info) {
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
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
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
  exports.Set("registerServerMessage", Napi::Function::New(env, registerServerMessage));
  return exports;
}

NODE_API_MODULE(emulators, Init)