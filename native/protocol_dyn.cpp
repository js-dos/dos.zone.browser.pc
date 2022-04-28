#include "protocol_dyn.h"

client_frame_set_size_fn client_frame_set_size_ptr = nullptr;
client_frame_update_lines_fn client_frame_update_lines_ptr = nullptr;
client_sound_init_fn client_sound_init_ptr = nullptr;
client_sound_push_fn client_sound_push_ptr = nullptr;
client_stdout_fn client_stdout_ptr = nullptr;
client_log_fn client_log_ptr = nullptr;
client_warn_fn client_warn_ptr = nullptr;
client_error_fn client_error_ptr = nullptr;
client_network_connected_fn client_network_connected_ptr = nullptr;
client_network_disconnected_fn client_network_disconnected_ptr = nullptr;
client_tick_fn client_tick_ptr = nullptr;
zip_progress_fn zip_progress_ptr = nullptr;


extern "C" __declspec(dllexport) void __cdecl set_client_frame_set_size(client_frame_set_size_fn ptr) {
    client_frame_set_size_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_frame_update_lines(client_frame_update_lines_fn ptr) {
    client_frame_update_lines_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_sound_init(client_sound_init_fn ptr) {
    client_sound_init_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_sound_push(client_sound_push_fn ptr) {
    client_sound_push_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_stdout(client_stdout_fn ptr){
    client_stdout_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_log(client_log_fn ptr) {
    client_log_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_warn(client_warn_fn ptr) {
    client_warn_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_error(client_error_fn ptr) {
    client_error_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_network_connected(client_network_connected_fn ptr) {
    client_network_connected_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_network_disconnected(client_network_disconnected_fn ptr) {
    client_network_disconnected_ptr = ptr;
}
extern "C" __declspec(dllexport) void __cdecl set_client_tick(client_tick_fn ptr) {
    client_tick_ptr = ptr;
}

extern "C" __declspec(dllexport) int __cdecl server_run_dyn() {
    return server_run();
}
extern "C" __declspec(dllexport) void __cdecl server_add_key_dyn(KBD_KEYS key, bool pressed, uint64_t pressedMs){
    server_add_key(key, pressed, pressedMs);
}
extern "C" __declspec(dllexport) void __cdecl server_mouse_moved_dyn(float x, float y, bool relative, uint64_t movedMs) {
    server_mouse_moved(x, y, relative, movedMs);
}
extern "C" __declspec(dllexport) void __cdecl server_mouse_button_dyn(int button, bool pressed, uint64_t pressedMs) {
    server_mouse_button(button, pressed, pressedMs);
}
extern "C" __declspec(dllexport) void __cdecl server_mouse_sync_dyn(uint64_t syncMs) {
    server_mouse_sync(syncMs);
}
extern "C" __declspec(dllexport) void __cdecl server_pause_dyn() {
    server_pause();
}
extern "C" __declspec(dllexport) void __cdecl server_resume_dyn() {
    server_resume();
}
extern "C" __declspec(dllexport) void __cdecl server_mute_dyn() {
    server_mute();
}
extern "C" __declspec(dllexport) void __cdecl server_unmute_dyn() {
    server_unmute();
}
extern "C" __declspec(dllexport) void __cdecl server_exit_dyn() {
    server_exit();
}
extern "C" __declspec(dllexport) void __cdecl server_network_connect_dyn(NetworkType networkType, const char* address, uint32_t port) {
    server_network_connect(networkType, address, port);
}
extern "C" __declspec(dllexport) void __cdecl server_network_disconnect_dyn(NetworkType networkType) {
    server_network_disconnect(networkType);
}
extern "C" __declspec(dllexport) void __cdecl zip_set_on_progress_dyn(zip_progress_fn progress) {
    zip_progress_ptr = progress;
    zip_set_on_progress([](const char *file, int extracted, int total) {
        if (zip_progress_ptr) {
            zip_progress_ptr(file, extracted, total);
        }
    });
}
extern "C" __declspec(dllexport) ZipArchive __cdecl zip_from_fs_dyn(double changedAfterMs) {
    return zip_from_fs(changedAfterMs);
}
extern "C" __declspec(dllexport) int __cdecl zipfile_to_fs_dyn(const char *fileName) {
    return zipfile_to_fs(fileName);
}
extern "C" __declspec(dllexport) double __cdecl get_changes_mtime_ms_dyn() {
    return get_changes_mtime_ms();
}

void client_frame_set_size(int width, int height) {
    if (client_frame_set_size_ptr) {
        client_frame_set_size_ptr(width, height);
    }
}

void client_frame_update_lines(uint32_t* lines, uint32_t count, void* rgba) {
    if (client_frame_update_lines_ptr) {
        client_frame_update_lines_ptr(lines, count, rgba);
    }
}

void client_sound_init(int freq) {
    if (client_sound_init_ptr) {
        client_sound_init_ptr(freq);
    }
}

void client_sound_push(const float* samples, int num_samples) {
    if (client_sound_push_ptr) {
        client_sound_push_ptr(samples, num_samples);
    }
}

void client_stdout(const char* data, uint32_t amount) {
    if (client_stdout_ptr) {
        client_stdout_ptr(data, amount);
    }
}

void client_log(const char* tag, const char* message) {
    if (client_log_ptr) {
        client_log_ptr(tag, message);
    }
}

void client_warn(const char* tag, const char* message) {
    if (client_warn_ptr) {
        client_warn_ptr(tag, message);
    }
}

void client_error(const char* tag, const char* message) {
    if (client_error_ptr) {
        client_error_ptr(tag, message);
    }
}

void client_network_connected(NetworkType networkType, const char* address, uint32_t port) {
    if (client_network_connected_ptr) {
        client_network_connected_ptr(networkType, address, port);
    }
}

void client_network_disconnected(NetworkType networkType) {
    if (client_network_disconnected_ptr) {
        client_network_disconnected_ptr(networkType);
    }
}

void client_tick() {
    if (client_tick_ptr) {
        client_tick_ptr();
    }
}