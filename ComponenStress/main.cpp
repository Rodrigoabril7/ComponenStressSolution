#include <thread>
#include <windows.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <pdh.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "pdh.lib")
// Directiva al enlazador para ocultar la consola negra inicial
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

typedef int nvmlReturn_t;
typedef void* nvmlDevice_t;
typedef struct { unsigned int gpu; unsigned int memory; } nvmlUtilization_t;
typedef nvmlReturn_t(*nvmlInit_t)(void);
typedef nvmlReturn_t(*nvmlDeviceGetHandleByIndex_t)(unsigned int, nvmlDevice_t*);
typedef nvmlReturn_t(*nvmlDeviceGetTemperature_t)(nvmlDevice_t, int, unsigned int*);
typedef nvmlReturn_t(*nvmlDeviceGetUtilizationRates_t)(nvmlDevice_t, nvmlUtilization_t*);

int main() {
    unsigned int hilos = std::thread::hardware_concurrency();

    HMODULE hNvml = LoadLibraryA("nvml.dll");
    nvmlDevice_t gpuDevice = nullptr;
    nvmlDeviceGetTemperature_t getGpuTemp = nullptr;
    nvmlDeviceGetUtilizationRates_t getGpuUtil = nullptr;
    bool hasNvidia = false;

    if (hNvml) {
        auto nvmlInit = (nvmlInit_t)GetProcAddress(hNvml, "nvmlInit_v2");
        auto getHandle = (nvmlDeviceGetHandleByIndex_t)GetProcAddress(hNvml, "nvmlDeviceGetHandleByIndex_v2");
        getGpuTemp = (nvmlDeviceGetTemperature_t)GetProcAddress(hNvml, "nvmlDeviceGetTemperature");
        getGpuUtil = (nvmlDeviceGetUtilizationRates_t)GetProcAddress(hNvml, "nvmlDeviceGetUtilizationRates");

        if (nvmlInit && nvmlInit() == 0 && getHandle) {
            getHandle(0, &gpuDevice);
            hasNvidia = true;
        }
    }

    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "ComponenStress - Performance Dashboard", nullptr, nullptr);
    if (window == nullptr) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.FontGlobalScale = 1.5f;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    PDH_HQUERY cpuQuery;
    PDH_HCOUNTER cpuTotal;
    PdhOpenQuery(NULL, NULL, &cpuQuery);
    PdhAddEnglishCounterA(cpuQuery, "\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);

    double ultimoTiempoEscaneo = glfwGetTime();
    float cpuLoad = 0.0f;
    unsigned int gpuTemp = 0;
    unsigned int gpuLoad = 0;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::Begin("Hardware Monitor", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("Thermal Monitoring System Initialized.");
        ImGui::Separator();
        ImGui::Text("Logical Threads detected: %d", hilos);

        double tiempoActual = glfwGetTime();
        if (tiempoActual - ultimoTiempoEscaneo >= 0.5) {
            PdhCollectQueryData(cpuQuery);
            PDH_FMT_COUNTERVALUE counterVal;
            PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
            cpuLoad = static_cast<float>(counterVal.doubleValue);

            if (hasNvidia && gpuDevice) {
                getGpuTemp(gpuDevice, 0, &gpuTemp);
                nvmlUtilization_t util;
                if (getGpuUtil(gpuDevice, &util) == 0) {
                    gpuLoad = util.gpu;
                }
            }
            ultimoTiempoEscaneo = tiempoActual;
        }

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "--- CPU INFO ---");
        ImGui::Text("CPU Load: %.1f %%", cpuLoad);
        ImGui::ProgressBar(cpuLoad / 100.0f, ImVec2(-1.0f, 0.0f), "CPU Load");
        ImGui::Text("CPU Temperature: [Requires Ring0 Driver]");

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "--- GPU INFO (NVIDIA) ---");
        if (hasNvidia) {
            ImGui::Text("GPU Temperature: %d C", gpuTemp);
            ImGui::Text("GPU Load: %d %%", gpuLoad);
            ImGui::ProgressBar(gpuLoad / 100.0f, ImVec2(-1.0f, 0.0f), "GPU Load");
        }
        else {
            ImGui::Text("No compatible NVIDIA GPU found.");
        }

        ImGui::Spacing();
        ImGui::Separator();
        if (ImGui::Button("Close Application")) {
            glfwSetWindowShouldClose(window, true);
        }

        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    PdhCloseQuery(cpuQuery);
    if (hNvml) FreeLibrary(hNvml);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}