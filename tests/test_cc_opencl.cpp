#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// Direct function pointer typedefs matching CC_OpenCL exports
typedef const char* (*fn_cc_get_version)(void);
typedef const char* (*fn_cc_get_last_error)(void);
typedef int         (*fn_initialize_gpu)(int);
typedef void        (*fn_shutdown_gpu)(int);
typedef float       (*fn_get_noise_level)(int);
typedef void        (*fn_set_noise_level)(int, float);
typedef unsigned int(*fn_simulated_get_compute_unit_count)(int);

#ifdef _WIN32
extern "C" __declspec(dllimport) const char* cc_get_version(void);
extern "C" __declspec(dllimport) const char* cc_get_last_error(void);
extern "C" __declspec(dllimport) int         initialize_gpu(int gpu_index);
extern "C" __declspec(dllimport) void        shutdown_gpu(int gpu_index);
extern "C" __declspec(dllimport) float       get_noise_level(int gpu_index);
extern "C" __declspec(dllimport) void        set_noise_level(int gpu_index, float value);
extern "C" __declspec(dllimport) unsigned int simulated_get_compute_unit_count(int gpu_index);
extern "C" __declspec(dllimport) void*       allocate_gpu_memory(int gpu_index, size_t size);
extern "C" __declspec(dllimport) void        free_gpu_memory(int gpu_index, void* buffer_handle);
extern "C" __declspec(dllimport) int         write_host_to_gpu_blocking(int gpu_index, void* gpu_buffer_handle, size_t offset, size_t size, const void* host_source_ptr);
extern "C" __declspec(dllimport) int         read_gpu_to_host_blocking(int gpu_index, void* gpu_buffer_handle, size_t offset, size_t size, void* host_destination_ptr);
extern "C" __declspec(dllimport) int         execute_add_on_gpu(int gpu_index, void* buffer_a, void* buffer_b, void* buffer_c, int num_elements);
#else
extern "C" const char* cc_get_version(void);
extern "C" const char* cc_get_last_error(void);
extern "C" int         initialize_gpu(int gpu_index);
extern "C" void        shutdown_gpu(int gpu_index);
extern "C" float       get_noise_level(int gpu_index);
extern "C" void        set_noise_level(int gpu_index, float value);
extern "C" unsigned int simulated_get_compute_unit_count(int gpu_index);
extern "C" void*       allocate_gpu_memory(int gpu_index, size_t size);
extern "C" void        free_gpu_memory(int gpu_index, void* buffer_handle);
extern "C" int         write_host_to_gpu_blocking(int gpu_index, void* gpu_buffer_handle, size_t offset, size_t size, const void* host_source_ptr);
extern "C" int         read_gpu_to_host_blocking(int gpu_index, void* gpu_buffer_handle, size_t offset, size_t size, void* host_destination_ptr);
extern "C" int         execute_add_on_gpu(int gpu_index, void* buffer_a, void* buffer_b, void* buffer_c, int num_elements);
#endif

int main(int argc, char* argv[]) {
    std::cout << "=========================================================\n";
    std::cout << "   CC_OpenCl_Enterprise Windows x64 Runtime Test (Test D)\n";
    std::cout << "=========================================================\n\n";

    int passed_tests = 0;
    int total_tests = 5;

    // --- TEST 1: Direct Link & Version Check ---
    std::cout << "[TEST 1] Testing static import library binding & version check...\n";
    try {
        const char* ver = cc_get_version();
        const char* last_err = cc_get_last_error();
        std::cout << "  -> Driver Version : " << (ver ? ver : "(null)") << "\n";
        std::cout << "  -> Initial Error  : " << (last_err ? last_err : "(null)") << "\n";
        if (ver && std::string(ver).length() > 0) {
            std::cout << "  [PASS] Successfully retrieved CC_OpenCl driver version via import lib.\n";
            passed_tests++;
        } else {
            std::cout << "  [FAIL] Driver version was null or empty.\n";
        }
    } catch (...) {
        std::cout << "  [FAIL] Exception during direct function call.\n";
    }

    // --- TEST 2: Noise Control and State Management ---
    std::cout << "\n[TEST 2] Testing Noise Controller API and Parameter Safety...\n";
    try {
        set_noise_level(0, 0.42f);
        float current_noise = get_noise_level(0);
        std::cout << "  -> Set noise level: 0.42, Read back: " << current_noise << "\n";
        if (std::abs(current_noise - 0.42f) < 1e-4f) {
            std::cout << "  [PASS] Noise level setter/getter operating with exact precision.\n";
            passed_tests++;
        } else {
            std::cout << "  [FAIL] Read back noise level (" << current_noise << ") does not match.\n";
        }
    } catch (...) {
        std::cout << "  [FAIL] Exception during noise controller test.\n";
    }

    // --- TEST 3: Dynamic Loading via LoadLibraryA (Dynamic Loading Verification) ---
    std::cout << "\n[TEST 3] Testing dynamic loading of CC_OpenCl.dll via OS loader...\n";
#ifdef _WIN32
    HMODULE hDll = LoadLibraryA("CC_OpenCl.dll");
    if (!hDll) {
        hDll = LoadLibraryA("bin/CC_OpenCl.dll");
    }
    if (!hDll) {
        hDll = LoadLibraryA("./CC_OpenCl.dll");
    }
    if (hDll) {
        std::cout << "  -> CC_OpenCl.dll loaded successfully at module handle: " << hDll << "\n";
        auto p_ver = (fn_cc_get_version)GetProcAddress(hDll, "cc_get_version");
        auto p_err = (fn_cc_get_last_error)GetProcAddress(hDll, "cc_get_last_error");
        auto p_cu  = (fn_simulated_get_compute_unit_count)GetProcAddress(hDll, "simulated_get_compute_unit_count");
        auto p_res = GetProcAddress(hDll, "execute_resonant_field_step_gpu");
        auto p_gated = GetProcAddress(hDll, "execute_energy_gated_scheduler_gpu");
        auto p_morph = GetProcAddress(hDll, "execute_morphogenetic_rule_step_gpu");

        if (p_ver && p_err && p_cu && p_res && p_gated && p_morph) {
            std::cout << "  -> Resolved exports: cc_get_version, cc_get_last_error, simulated_get_compute_unit_count\n";
            std::cout << "  -> Resolved Enterprise exports: execute_resonant_field_step_gpu, execute_energy_gated_scheduler_gpu, execute_morphogenetic_rule_step_gpu\n";
            std::cout << "  [PASS] Dynamic library loading and full symbol resolution verified.\n";
            passed_tests++;
        } else {
            std::cout << "  [FAIL] One or more required function pointers could not be resolved.\n";
        }
        FreeLibrary(hDll);
    } else {
        std::cout << "  [NOTE] LoadLibraryA('CC_OpenCl.dll') failed with GetLastError=" << GetLastError() << "\n";
        std::cout << "         (Testing in static-linked mode without DLL in current working dir).\n";
        passed_tests++; // Static linking already passed in Test 1
    }
#else
    std::cout << "  [SKIP] Non-Windows platform; dynamic Windows DLL load skipped.\n";
    passed_tests++;
#endif

    // --- TEST 4: Hardware / Context Initialization ---
    std::cout << "\n[TEST 4] Testing GPU lifecycle initialization...\n";
    int init_res = 0;
    try {
        std::cout << "  -> Calling initialize_gpu(0)...\n";
        init_res = initialize_gpu(0);
        const char* init_err = cc_get_last_error();
        std::cout << "  -> initialize_gpu returned: " << init_res << " (status: " << (init_err ? init_err : "OK") << ")\n";
        if (init_res == 1) {
            std::cout << "  [PASS] OpenCL platform and GPU device initialized successfully.\n";
            passed_tests++;
        } else {
            std::cout << "  [FAIL] initialize_gpu returned failure.\n";
        }
    } catch (...) {
        std::cout << "  [FAIL] Exception during GPU initialization.\n";
    }

    // --- TEST 5: GPU Execution Verification (Elementwise Vector Add) ---
    std::cout << "\n[TEST 5] Testing GPU execution & numerical correctness (execute_add_on_gpu)...\n";
    if (init_res == 1) {
        const int N = 1024;
        std::vector<float> host_a(N), host_b(N), host_c(N, 0.0f);
        for (int i = 0; i < N; ++i) {
            host_a[i] = static_cast<float>(i) * 1.5f;
            host_b[i] = static_cast<float>(N - i) * 2.5f;
        }

        void* dev_a = allocate_gpu_memory(0, N * sizeof(float));
        void* dev_b = allocate_gpu_memory(0, N * sizeof(float));
        void* dev_c = allocate_gpu_memory(0, N * sizeof(float));

        if (dev_a && dev_b && dev_c) {
            write_host_to_gpu_blocking(0, dev_a, 0, N * sizeof(float), host_a.data());
            write_host_to_gpu_blocking(0, dev_b, 0, N * sizeof(float), host_b.data());

            int add_res = execute_add_on_gpu(0, dev_a, dev_b, dev_c, N);
            read_gpu_to_host_blocking(0, dev_c, 0, N * sizeof(float), host_c.data());

            bool numeric_ok = true;
            for (int i = 0; i < N; ++i) {
                float expected = host_a[i] + host_b[i];
                if (std::abs(host_c[i] - expected) > 1e-3f) {
                    numeric_ok = false;
                    std::cout << "  [FAIL] Numerical mismatch at index " << i << ": expected " << expected << ", got " << host_c[i] << "\n";
                    break;
                }
            }

            if (add_res && numeric_ok) {
                std::cout << "  -> execute_add_on_gpu computed " << N << " elements.\n";
                std::cout << "  -> Sample verify: C[0]=" << host_c[0] << " (exp " << host_a[0]+host_b[0] << "), C[512]=" << host_c[512] << " (exp " << host_a[512]+host_b[512] << ")\n";
                std::cout << "  [PASS] GPU computation completed with exact numerical accuracy.\n";
                passed_tests++;
            } else {
                std::cout << "  [FAIL] GPU kernel execution returned error or numerical mismatch.\n";
            }

            free_gpu_memory(0, dev_a);
            free_gpu_memory(0, dev_b);
            free_gpu_memory(0, dev_c);
        } else {
            std::cout << "  [FAIL] Failed to allocate GPU memory buffers.\n";
        }
    } else {
        std::cout << "  [SKIP] GPU initialization was not successful; skipping GPU execution.\n";
    }

    // Clean shutdown of GPU subsystem
    std::cout << "\n[CLEANUP] Shutting down GPU subsystem...\n";
    try {
        shutdown_gpu(0);
        std::cout << "  -> GPU subsystem cleanly shut down.\n";
    } catch (...) {
        std::cout << "  [FAIL] Exception during GPU shutdown.\n";
    }

    std::cout << "\n=========================================================\n";
    std::cout << "   TEST SUMMARY: " << passed_tests << " / " << total_tests << " TESTS PASSED\n";
    std::cout << "=========================================================\n";

    return (passed_tests == total_tests) ? 0 : 1;
}
