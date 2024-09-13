// AirDensityWin32.cpp
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <functional>
#include <tuple>
#include <cmath>
#include <stdexcept>
#include "resource.h"

#pragma comment(lib, "comctl32.lib")

// Constants
const double g = 9.80665;  // gravitational acceleration (m/s^2)
const double R = 287.05;   // specific gas constant for dry air (J/kg·K)

// Window handle
HWND hWnd = NULL;

// Input controls
HWND hAltitudeEdit, hAltitudeCombo;
HWND hTemperatureEdit, hTemperatureCombo;
HWND hPressureEdit, hPressureCombo;

// Output control
HWND hOutputText;

// Font handle
HFONT hFont = NULL;

// Function prototypes
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
void CreateControls(HWND hWnd);
void CalculateISA();
void ClearInputs();
std::tuple<double, double, double, double, double> extended_isa_model(double altitude_m, double P_sea_level, double T_sea_level);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    // Create a modern font
    hFont = CreateFont(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    // Register window class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"AirDensityWin32Class";
    wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));

    if (!RegisterClassEx(&wcex)) {
        MessageBox(NULL, L"Window Registration Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // Create window
    hWnd = CreateWindow(
        L"AirDensityWin32Class",
        L"Extended ISA Model",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 350,
        NULL, NULL, hInstance, NULL
    );

    if (hWnd == NULL) {
        MessageBox(NULL, L"Window Creation Failed!", L"Error!", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    // Message loop
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        CreateControls(hWnd);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) {  // Calculate button
            CalculateISA();
        }
        else if (LOWORD(wParam) == 2) {  // Clear button
            ClearInputs();
        }
        break;
    case WM_DESTROY:
        DeleteObject(hFont);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void CreateControls(HWND hWnd) {
    // Create groupboxes
    HWND hInputGroup = CreateWindow(L"BUTTON", L"Input Parameters", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        10, 10, 420, 160, hWnd, NULL, NULL, NULL);
    HWND hResultsGroup = CreateWindow(L"BUTTON", L"Results", WS_VISIBLE | WS_CHILD | BS_GROUPBOX,
        10, 180, 420, 120, hWnd, NULL, NULL, NULL);

    // Create static texts
    CreateWindow(L"STATIC", L"Altitude:", WS_VISIBLE | WS_CHILD, 20, 30, 120, 20, hWnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"Sea-level Temperature:", WS_VISIBLE | WS_CHILD, 20, 60, 180, 20, hWnd, NULL, NULL, NULL);
    CreateWindow(L"STATIC", L"Sea-level Pressure:", WS_VISIBLE | WS_CHILD, 20, 90, 180, 20, hWnd, NULL, NULL, NULL);

    // Create edit controls
    hAltitudeEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        200, 30, 150, 24, hWnd, NULL, NULL, NULL);
    hTemperatureEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        200, 60, 150, 24, hWnd, NULL, NULL, NULL);
    hPressureEdit = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        200, 90, 150, 24, hWnd, NULL, NULL, NULL);

    // Create combo boxes
    hAltitudeCombo = CreateWindow(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        360, 30, 60, 100, hWnd, NULL, NULL, NULL);
    hTemperatureCombo = CreateWindow(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        360, 60, 60, 100, hWnd, NULL, NULL, NULL);
    hPressureCombo = CreateWindow(L"COMBOBOX", NULL, WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        360, 90, 60, 100, hWnd, NULL, NULL, NULL);

    // Add items to combo boxes
    SendMessage(hAltitudeCombo, CB_ADDSTRING, 0, (LPARAM)L"feet");
    SendMessage(hAltitudeCombo, CB_ADDSTRING, 0, (LPARAM)L"meters");
    SendMessage(hAltitudeCombo, CB_SETCURSEL, 0, 0);

    SendMessage(hTemperatureCombo, CB_ADDSTRING, 0, (LPARAM)L"°C");
    SendMessage(hTemperatureCombo, CB_ADDSTRING, 0, (LPARAM)L"°F");
    SendMessage(hTemperatureCombo, CB_SETCURSEL, 0, 0);

    SendMessage(hPressureCombo, CB_ADDSTRING, 0, (LPARAM)L"hPa");
    SendMessage(hPressureCombo, CB_ADDSTRING, 0, (LPARAM)L"inHg");
    SendMessage(hPressureCombo, CB_SETCURSEL, 0, 0);

    // Create calculate button
    CreateWindow(L"BUTTON", L"Calculate", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        120, 130, 100, 25, hWnd, (HMENU)1, NULL, NULL);

    // Create clear button
    CreateWindow(L"BUTTON", L"Clear", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        230, 130, 100, 25, hWnd, (HMENU)2, NULL, NULL);

    // Create output text
    hOutputText = CreateWindow(L"STATIC", L"", WS_VISIBLE | WS_CHILD,
        20, 200, 400, 90, hWnd, NULL, NULL, NULL);

    // Apply the font to all controls
    EnumChildWindows(hWnd, [](HWND hwnd, LPARAM lParam) -> BOOL {
        SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
        return TRUE;
        }, 0);
}

void CalculateISA() {
    wchar_t buffer[256];
    double altitude, T_sea_level, P_sea_level;

    // Get input values
    GetWindowText(hAltitudeEdit, buffer, 256);
    altitude = _wtof(buffer);
    GetWindowText(hTemperatureEdit, buffer, 256);
    T_sea_level = _wtof(buffer);
    GetWindowText(hPressureEdit, buffer, 256);
    P_sea_level = _wtof(buffer);

    // Convert units if necessary
    if (SendMessage(hAltitudeCombo, CB_GETCURSEL, 0, 0) == 0) {  // feet
        altitude *= 0.3048;  // convert to meters
    }
    if (SendMessage(hTemperatureCombo, CB_GETCURSEL, 0, 0) == 1) {  // Fahrenheit
        T_sea_level = (T_sea_level - 32) * 5.0 / 9.0;
    }
    if (SendMessage(hPressureCombo, CB_GETCURSEL, 0, 0) == 1) {  // inHg
        P_sea_level *= 33.8639;  // convert to hPa
    }

    P_sea_level *= 100;  // Convert hPa to Pa

    try {
        double T, P, rho, percent_P, percent_rho;
        std::tie(T, P, rho, percent_P, percent_rho) = extended_isa_model(altitude, P_sea_level, T_sea_level);

        // Convert results back to selected units
        if (SendMessage(hAltitudeCombo, CB_GETCURSEL, 0, 0) == 0) {  // feet
            altitude /= 0.3048;
        }
        if (SendMessage(hTemperatureCombo, CB_GETCURSEL, 0, 0) == 1) {  // Fahrenheit
            T = T * 9.0 / 5.0 + 32;
        }
        if (SendMessage(hPressureCombo, CB_GETCURSEL, 0, 0) == 1) {  // inHg
            P /= 33.8639;
        }

        swprintf(buffer, 256,
            L"At %.2f %s:\n"
            L"Temperature = %.2f %s\n"
            L"Pressure = %.2f %s (%.2f%% of sea level)\n"
            L"Density = %.6f kg/m³ (%.2f%% of sea level)",
            altitude,
            SendMessage(hAltitudeCombo, CB_GETCURSEL, 0, 0) == 0 ? L"feet" : L"meters",
            T,
            SendMessage(hTemperatureCombo, CB_GETCURSEL, 0, 0) == 0 ? L"°C" : L"°F",
            P,
            SendMessage(hPressureCombo, CB_GETCURSEL, 0, 0) == 0 ? L"hPa" : L"inHg",
            percent_P,
            rho,
            percent_rho
        );

        SetWindowText(hOutputText, buffer);
    }
    catch (const std::exception& e) {
        MessageBoxA(hWnd, e.what(), "Calculation Error", MB_OK | MB_ICONERROR);
    }
}

void ClearInputs() {
    // Clear all input fields
    SetWindowText(hAltitudeEdit, L"");
    SetWindowText(hTemperatureEdit, L"");
    SetWindowText(hPressureEdit, L"");

    // Reset units to default
    SendMessage(hAltitudeCombo, CB_SETCURSEL, 0, 0);  // feet
    SendMessage(hTemperatureCombo, CB_SETCURSEL, 0, 0);  // °C
    SendMessage(hPressureCombo, CB_SETCURSEL, 0, 0);  // hPa

    // Clear output text
    SetWindowText(hOutputText, L"");
}

std::tuple<double, double, double, double, double> extended_isa_model(double altitude_m, double P_sea_level, double T_sea_level) {
    double T_sea_level_K = T_sea_level + 273.15;
    double T, P;

    struct Layer {
        double max_alt;
        std::function<double(double)> temp_func;
        std::function<double(double, double, double)> press_func;
    };

    std::vector<Layer> layers = {
        {11000, [&](double h) { return T_sea_level_K - 0.0065 * h; },
                [&](double h, double T, double P0) { return P0 * std::pow(T / T_sea_level_K, g / (0.0065 * R)); }},
        {20000, [&](double h) { return T_sea_level_K - 0.0065 * 11000; },
                [&](double h, double T, double P0) { return P0 * 0.22336 * std::exp(-g * (h - 11000) / (R * T)); }},
        {32000, [&](double h) { return 216.65 + 0.001 * (h - 20000); },
                [&](double h, double T, double P0) { return P0 * 0.05403 * std::pow(T / 216.65, -g / (0.001 * R)); }},
        {47000, [&](double h) { return 228.65; },
                [&](double h, double T, double P0) { return P0 * 0.00856 * std::exp(-g * (h - 32000) / (R * T)); }},
        {51000, [&](double h) { return 228.65 + 0.0028 * (h - 47000); },
                [&](double h, double T, double P0) { return P0 * 0.00109 * std::pow(T / 228.65, -g / (0.0028 * R)); }},
        {71000, [&](double h) { return 270.65; },
                [&](double h, double T, double P0) { return P0 * 0.00066 * std::exp(-g * (h - 51000) / (R * T)); }},
        {84852, [&](double h) { return 270.65 - 0.0028 * (h - 71000); },
                [&](double h, double T, double P0) { return P0 * 0.00003 * std::pow(T / 270.65, -g / (-0.0028 * R)); }}
    };

    for (const auto& layer : layers) {
        if (altitude_m <= layer.max_alt) {
            T = layer.temp_func(altitude_m);
            P = layer.press_func(altitude_m, T, P_sea_level);
            break;
        }
    }

    if (altitude_m > 84852) {
        throw std::runtime_error("Altitude exceeds the limits of this extended ISA model");
    }

    double rho = P / (R * T);
    double T_C = T - 273.15;
    double P_hPa = P / 100;
    double rho_kg_m3 = rho;

    double percent_pressure = (P_hPa / (P_sea_level / 100)) * 100;
    double percent_density = (rho_kg_m3 / (P_sea_level / (R * T_sea_level_K))) * 100;

    return std::make_tuple(T_C, P_hPa, rho_kg_m3, percent_pressure, percent_density);
}