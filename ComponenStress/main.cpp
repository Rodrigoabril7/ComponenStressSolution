#include <iostream>
#include <thread>
#include <windows.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

int main() {
    
    SetConsoleTitleA("ComponenStress");

    std::cout << "--- Initiating PC's monitoring ---" << std::endl;

    unsigned int hilos = std::thread::hardware_concurrency();

    if (hilos == 0) {
        std::cout << "[ERROR] Number of logical CPU's threads could not be determined." << std::endl;
    }
    else {
        std::cout << "[INFO] CPU DETECTED. There's " << hilos << " logical threads available." << std::endl;
    }

    std::cout << "\nPress ENTER to continue...";
    std::cin.get();

    return 0;
}
