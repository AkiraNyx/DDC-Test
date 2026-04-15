/*
 * DDC/CI Monitor Test Tool
 * Tests DDC/CI availability, brightness/contrast control, and VCP features
 * for all connected monitors.
 *
 * Build: cl /EHsc /O2 ddc_test.cpp /link dxva2.lib user32.lib
 * Usage:
 *   ddc_test              Enumerate monitors and show DDC/CI info
 *   ddc_test --set <N>    Set all monitors brightness to N% (0-100)
 *   ddc_test --get        Show current brightness of all monitors
 */

#include <windows.h>
#include <highlevelmonitorconfigurationapi.h>
#include <physicalmonitorenumerationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "dxva2.lib")
#pragma comment(lib, "user32.lib")

static DWORD MapDDCToPercent(DWORD value, DWORD minVal, DWORD maxVal)
{
    if (maxVal <= minVal) return 0;
    return ((value - minVal) * 100) / (maxVal - minVal);
}

static DWORD MapPercentToDDC(DWORD percent, DWORD minVal, DWORD maxVal)
{
    if (percent > 100) percent = 100;
    if (maxVal <= minVal) return minVal;
    return minVal + (percent * (maxVal - minVal)) / 100;
}

static DWORD g_targetBrightness = 0;

static void TestMonitor(HMONITOR hMonitor, int monitorIndex)
{
    MONITORINFOEXW monInfo = {};
    monInfo.cbSize = sizeof(monInfo);
    if (!GetMonitorInfoW(hMonitor, &monInfo)) {
        printf("  [!] GetMonitorInfo failed, error: %lu\n", GetLastError());
        return;
    }

    printf("\n--- Monitor #%d ---\n", monitorIndex);
    printf("  Device: %ls\n", monInfo.szDevice);
    printf("  Area: (%ld,%ld)-(%ld,%ld)\n",
           monInfo.rcMonitor.left, monInfo.rcMonitor.top,
           monInfo.rcMonitor.right, monInfo.rcMonitor.bottom);
    printf("  Primary: %s\n",
           (monInfo.dwFlags & MONITORINFOF_PRIMARY) ? "Yes" : "No");

    DWORD numPhysical = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numPhysical)) {
        printf("  [!] GetNumberOfPhysicalMonitors failed, error: %lu\n",
               GetLastError());
        return;
    }

    printf("  Physical monitors: %lu\n", numPhysical);

    PHYSICAL_MONITOR *physicalMonitors = (PHYSICAL_MONITOR *)
        malloc(sizeof(PHYSICAL_MONITOR) * numPhysical);
    if (!physicalMonitors) {
        printf("  [!] Memory allocation failed\n");
        return;
    }

    if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, numPhysical,
                                          physicalMonitors)) {
        printf("  [!] GetPhysicalMonitors failed, error: %lu\n",
               GetLastError());
        free(physicalMonitors);
        return;
    }

    for (DWORD i = 0; i < numPhysical; i++) {
        printf("\n  Physical monitor #%lu:\n", i);
        printf("    Description: %ls\n",
               physicalMonitors[i].szPhysicalMonitorDescription);

        HANDLE hPhysical = physicalMonitors[i].hPhysicalMonitor;

        // Brightness (VCP 0x10)
        DWORD minBrightness = 0, curBrightness = 0, maxBrightness = 0;
        if (GetMonitorBrightness(hPhysical,
                                  &minBrightness,
                                  &curBrightness,
                                  &maxBrightness)) {
            printf("    [OK] DDC/CI Brightness available\n");
            printf("    Range: %lu - %lu\n", minBrightness, maxBrightness);
            printf("    Current: %lu (raw DDC/CI value)\n", curBrightness);
            printf("    Current: %lu%% (mapped)\n",
                   MapDDCToPercent(curBrightness, minBrightness, maxBrightness));

            if (minBrightness == 0 && maxBrightness == 100) {
                printf("    Mapping: 1:1 (no conversion needed)\n");
            } else {
                printf("    Mapping: range conversion [%lu-%lu] <-> [0-100]\n",
                       minBrightness, maxBrightness);
            }
        } else {
            printf("    [X] DDC/CI Brightness NOT available (error: %lu)\n",
                   GetLastError());
        }

        // Contrast (VCP 0x12)
        DWORD minContrast = 0, curContrast = 0, maxContrast = 0;
        if (GetMonitorContrast(hPhysical,
                                &minContrast,
                                &curContrast,
                                &maxContrast)) {
            printf("    [OK] DDC/CI Contrast available\n");
            printf("    Range: %lu - %lu, Current: %lu\n",
                   minContrast, maxContrast, curContrast);
        } else {
            printf("    [X] DDC/CI Contrast NOT available\n");
        }

        // Capabilities
        DWORD capFlags = 0, colorTempFlags = 0;
        if (GetMonitorCapabilities(hPhysical, &capFlags, &colorTempFlags)) {
            printf("    Capability flags: 0x%lx\n", capFlags);
            printf("    Color temp flags: 0x%lx\n", colorTempFlags);
        }

        // Low-level VCP 0x10
        MC_VCP_CODE_TYPE codeType;
        DWORD currentValue = 0, maxValue = 0;
        if (GetVCPFeatureAndVCPFeatureReply(hPhysical, 0x10,
                                             &codeType,
                                             &currentValue,
                                             &maxValue)) {
            printf("    [VCP 0x10] Brightness (low-level): cur=%lu, max=%lu, type=%s\n",
                   currentValue, maxValue,
                   (codeType == MC_MOMENTARY) ? "Momentary" : "Continuous");
        }
    }

    DestroyPhysicalMonitors(numPhysical, physicalMonitors);
    free(physicalMonitors);
}

typedef struct { int count; } ENUM_CONTEXT;

static BOOL CALLBACK EnumProc_Info(HMONITOR hMonitor, HDC hdcMonitor,
                                    LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor; (void)lprcMonitor;
    ENUM_CONTEXT *ctx = (ENUM_CONTEXT *)dwData;
    ctx->count++;
    TestMonitor(hMonitor, ctx->count);
    return TRUE;
}

static BOOL CALLBACK EnumProc_SetBrightness(HMONITOR hMonitor, HDC hdcMonitor,
                                              LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor; (void)lprcMonitor;
    ENUM_CONTEXT *ctx = (ENUM_CONTEXT *)dwData;
    ctx->count++;

    DWORD numPhysical = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numPhysical))
        return TRUE;

    PHYSICAL_MONITOR *pm = (PHYSICAL_MONITOR *)
        malloc(sizeof(PHYSICAL_MONITOR) * numPhysical);
    if (!pm) return TRUE;

    if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, numPhysical, pm)) {
        free(pm);
        return TRUE;
    }

    for (DWORD i = 0; i < numPhysical; i++) {
        DWORD minB = 0, curB = 0, maxB = 0;
        if (GetMonitorBrightness(pm[i].hPhysicalMonitor, &minB, &curB, &maxB)) {
            DWORD targetDDC = MapPercentToDDC(g_targetBrightness, minB, maxB);
            if (SetMonitorBrightness(pm[i].hPhysicalMonitor, targetDDC)) {
                printf("  Monitor #%d.%lu: brightness set to %lu%% (DDC: %lu, range %lu-%lu)\n",
                       ctx->count, i, g_targetBrightness, targetDDC, minB, maxB);
            } else {
                printf("  Monitor #%d.%lu: set failed, error: %lu\n",
                       ctx->count, i, GetLastError());
            }
        } else {
            printf("  Monitor #%d.%lu: DDC/CI not available, skipped\n",
                   ctx->count, i);
        }
    }

    DestroyPhysicalMonitors(numPhysical, pm);
    free(pm);
    return TRUE;
}

static BOOL CALLBACK EnumProc_GetBrightness(HMONITOR hMonitor, HDC hdcMonitor,
                                              LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor; (void)lprcMonitor;
    ENUM_CONTEXT *ctx = (ENUM_CONTEXT *)dwData;
    ctx->count++;

    DWORD numPhysical = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numPhysical))
        return TRUE;

    PHYSICAL_MONITOR *pm = (PHYSICAL_MONITOR *)
        malloc(sizeof(PHYSICAL_MONITOR) * numPhysical);
    if (!pm) return TRUE;

    if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, numPhysical, pm)) {
        free(pm);
        return TRUE;
    }

    for (DWORD i = 0; i < numPhysical; i++) {
        DWORD minB = 0, curB = 0, maxB = 0;
        if (GetMonitorBrightness(pm[i].hPhysicalMonitor, &minB, &curB, &maxB)) {
            printf("  Monitor #%d.%lu: %lu%% (DDC: %lu, range %lu-%lu)\n",
                   ctx->count, i,
                   MapDDCToPercent(curB, minB, maxB), curB, minB, maxB);
        } else {
            printf("  Monitor #%d.%lu: DDC/CI not available\n",
                   ctx->count, i);
        }
    }

    DestroyPhysicalMonitors(numPhysical, pm);
    free(pm);
    return TRUE;
}

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(65001);

    printf("========================================\n");
    printf("  DDC/CI Monitor Test Tool\n");
    printf("========================================\n");

    if (argc >= 2 && strcmp(argv[1], "--get") == 0) {
        printf("\nCurrent brightness:\n");
        ENUM_CONTEXT ctx = {};
        EnumDisplayMonitors(NULL, NULL, EnumProc_GetBrightness, (LPARAM)&ctx);
        if (ctx.count == 0) printf("  No monitors found.\n");
        return 0;
    }

    if (argc >= 3 && strcmp(argv[1], "--set") == 0) {
        g_targetBrightness = (DWORD)atoi(argv[2]);
        if (g_targetBrightness > 100) g_targetBrightness = 100;

        printf("\nSetting all DDC/CI monitors to %lu%%...\n", g_targetBrightness);

        ENUM_CONTEXT ctx = {};
        EnumDisplayMonitors(NULL, NULL, EnumProc_SetBrightness, (LPARAM)&ctx);

        printf("\nDone, processed %d monitor(s)\n", ctx.count);
        return 0;
    }

    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        printf("\nUsage:\n");
        printf("  ddc_test              Enumerate monitors and show DDC/CI info\n");
        printf("  ddc_test --get        Show current brightness of all monitors\n");
        printf("  ddc_test --set <N>    Set all monitors brightness to N%% (0-100)\n");
        printf("  ddc_test --help       Show this help\n");
        return 0;
    }

    // Default: enumerate and test all monitors
    ENUM_CONTEXT ctx = {};
    EnumDisplayMonitors(NULL, NULL, EnumProc_Info, (LPARAM)&ctx);

    printf("\n========================================\n");
    printf("Summary: %d monitor(s) detected\n", ctx.count);
    printf("========================================\n");

    if (ctx.count == 0) {
        printf("\n[!] No monitors detected.\n");
        printf("    Make sure your monitor is connected and powered on.\n");
    }

    printf("\nUsage:\n");
    printf("  ddc_test              Enumerate monitors and show DDC/CI info\n");
    printf("  ddc_test --get        Show current brightness of all monitors\n");
    printf("  ddc_test --set <N>    Set all monitors brightness to N%% (0-100)\n");

    return 0;
}
