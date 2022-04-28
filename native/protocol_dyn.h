#ifndef _PROTOCOL_DYN_H_
#define _PROTOCOL_DYN_H_

#include <protocol.h>
#include <jsdos-libzip.h>

typedef void (__cdecl *client_frame_set_size_fn)(int width, int height);
typedef void (__cdecl *client_frame_update_lines_fn)(uint32_t* lines, uint32_t count, void* rgba);
typedef void (__cdecl *client_sound_init_fn)(int freq);
typedef void (__cdecl *client_sound_push_fn)(const float* samples, int num_samples);
typedef void (__cdecl *client_stdout_fn)(const char* data, uint32_t amount);
typedef void (__cdecl *client_log_fn)(const char* tag, const char* message);
typedef void (__cdecl *client_warn_fn)(const char* tag, const char* message);
typedef void (__cdecl *client_error_fn)(const char* tag, const char* message);
typedef void (__cdecl *client_network_connected_fn)(NetworkType networkType, const char* address, uint32_t port);
typedef void (__cdecl *client_network_disconnected_fn)(NetworkType networkType);
typedef void (__cdecl *client_tick_fn)();

typedef void (__cdecl *zip_progress_fn) (const char *file, int extracted, int total);

#ifdef MSVC
extern "C" __declspec(dllimport) void __cdecl set_client_frame_set_size(client_frame_set_size_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_frame_update_lines(client_frame_update_lines_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_sound_init(client_sound_init_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_sound_push(client_sound_push_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_stdout(client_stdout_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_log(client_log_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_warn(client_warn_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_error(client_error_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_network_connected(client_network_connected_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_network_disconnected(client_network_disconnected_fn ptr);
extern "C" __declspec(dllimport) void __cdecl set_client_tick(client_tick_fn ptr);
#else
extern "C" __declspec(dllexport) void __cdecl set_client_frame_set_size(client_frame_set_size_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_frame_update_lines(client_frame_update_lines_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_sound_init(client_sound_init_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_sound_push(client_sound_push_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_stdout(client_stdout_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_log(client_log_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_warn(client_warn_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_error(client_error_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_network_connected(client_network_connected_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_network_disconnected(client_network_disconnected_fn ptr);
extern "C" __declspec(dllexport) void __cdecl set_client_tick(client_tick_fn ptr);
#endif

#ifdef MSVC
extern "C" __declspec(dllimport) int __cdecl server_run_dyn();
extern "C" __declspec(dllimport) void __cdecl server_add_key_dyn(KBD_KEYS key, bool pressed, uint64_t pressedMs);
extern "C" __declspec(dllimport) void __cdecl server_mouse_moved_dyn(float x, float y, bool relative, uint64_t movedMs);
extern "C" __declspec(dllimport) void __cdecl server_mouse_button_dyn(int button, bool pressed, uint64_t pressedMs);
extern "C" __declspec(dllimport) void __cdecl server_mouse_sync_dyn(uint64_t syncMs);
extern "C" __declspec(dllimport) void __cdecl server_pause_dyn();
extern "C" __declspec(dllimport) void __cdecl server_resume_dyn();
extern "C" __declspec(dllimport) void __cdecl server_mute_dyn();
extern "C" __declspec(dllimport) void __cdecl server_unmute_dyn();
extern "C" __declspec(dllimport) void __cdecl server_exit_dyn();
extern "C" __declspec(dllimport) void __cdecl server_network_connect_dyn(NetworkType networkType, const char* address, uint32_t port);
extern "C" __declspec(dllimport) void __cdecl server_network_disconnect_dyn(NetworkType networkType);
#else
extern "C" __declspec(dllexport) int __cdecl server_run_dyn();
extern "C" __declspec(dllexport) void __cdecl server_add_key_dyn(KBD_KEYS key, bool pressed, uint64_t pressedMs);
extern "C" __declspec(dllexport) void __cdecl server_mouse_moved_dyn(float x, float y, bool relative, uint64_t movedMs);
extern "C" __declspec(dllexport) void __cdecl server_mouse_button_dyn(int button, bool pressed, uint64_t pressedMs);
extern "C" __declspec(dllexport) void __cdecl server_mouse_sync_dyn(uint64_t syncMs);
extern "C" __declspec(dllexport) void __cdecl server_pause_dyn();
extern "C" __declspec(dllexport) void __cdecl server_resume_dyn();
extern "C" __declspec(dllexport) void __cdecl server_mute_dyn();
extern "C" __declspec(dllexport) void __cdecl server_unmute_dyn();
extern "C" __declspec(dllexport) void __cdecl server_exit_dyn();
extern "C" __declspec(dllexport) void __cdecl server_network_connect_dyn(NetworkType networkType, const char* address, uint32_t port);
extern "C" __declspec(dllexport) void __cdecl server_network_disconnect_dyn(NetworkType networkType);
#endif

#ifdef MSVC
extern "C" __declspec(dllimport) void __cdecl zip_set_on_progress_dyn(zip_progress_fn progress);
extern "C" __declspec(dllimport) ZipArchive __cdecl zip_from_fs_dyn(double changedAfterMs);
extern "C" __declspec(dllimport) int __cdecl zipfile_to_fs_dyn(const char *fileName);
extern "C" __declspec(dllimport) double __cdecl get_changes_mtime_ms_dyn();
#else
extern "C" __declspec(dllexport) void __cdecl zip_set_on_progress_dyn(zip_progress_fn progress);
extern "C" __declspec(dllexport) ZipArchive __cdecl zip_from_fs_dyn(double changedAfterMs);
extern "C" __declspec(dllexport) int __cdecl zipfile_to_fs_dyn(const char *fileName);
extern "C" __declspec(dllexport) double __cdecl get_changes_mtime_ms_dyn();
#endif

#endif // _PROTOCOL_DYN_H_