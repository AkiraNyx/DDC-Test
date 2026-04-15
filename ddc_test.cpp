/*
 * DDC/CI Monitor Test Tool
 * Author: AkiraNyx
 *
 * Tests DDC/CI availability, brightness/contrast control, and VCP features
 * for all connected monitors. Supports English and Simplified Chinese.
 *
 * Build: cl /EHsc /O2 ddc_test.cpp /link dxva2.lib user32.lib
 * Usage:
 *   ddc_test              Enumerate monitors and show DDC/CI info
 *   ddc_test --set <N>    Set all monitors brightness to N% (0-100)
 *   ddc_test --get        Show current brightness of all monitors
 *   ddc_test --lang cn    Use Simplified Chinese output
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

#define VERSION "1.1.0"
#define AUTHOR  "AkiraNyx"

//
// Localization
//
static int g_langCN = 0; // 0=English, 1=Chinese

typedef struct {
    const char *banner;
    const char *author_line;
    const char *monitor_header;
    const char *device;
    const char *area;
    const char *primary_yes;
    const char *primary_no;
    const char *primary_label;
    const char *physical_count;
    const char *physical_header;
    const char *description;
    const char *brightness_ok;
    const char *brightness_range;
    const char *brightness_current_raw;
    const char *brightness_current_mapped;
    const char *mapping_1to1;
    const char *mapping_convert;
    const char *brightness_fail;
    const char *contrast_ok;
    const char *contrast_range;
    const char *contrast_fail;
    const char *cap_flags;
    const char *color_temp;
    const char *vcp_brightness;
    const char *summary;
    const char *no_monitors;
    const char *no_monitors_hint;
    const char *usage_header;
    const char *usage_enum;
    const char *usage_get;
    const char *usage_set;
    const char *usage_lang;
    const char *usage_help;
    const char *current_brightness;
    const char *setting_all;
    const char *done;
    const char *set_ok;
    const char *set_fail;
    const char *not_available;
    const char *getmoninfo_fail;
    const char *getphysmon_fail;
    const char *alloc_fail;
    const char *getnumphys_fail;
} Lang;

static const Lang LANG_EN = {
    "  DDC/CI Monitor Test Tool",
    "  Author: " AUTHOR "  |  Version: " VERSION,
    "--- Monitor #%d ---",
    "  Device: %ls",
    "  Area: (%ld,%ld)-(%ld,%ld)",
    "Yes", "No",
    "  Primary: %s",
    "  Physical monitors: %lu",
    "  Physical monitor #%lu:",
    "    Description: %ls",
    "    [OK] DDC/CI Brightness available",
    "    Range: %lu - %lu",
    "    Current: %lu (raw DDC/CI value)",
    "    Current: %lu%% (mapped)",
    "    Mapping: 1:1 (no conversion needed)",
    "    Mapping: range conversion [%lu-%lu] <-> [0-100]",
    "    [X] DDC/CI Brightness NOT available (error: %lu)",
    "    [OK] DDC/CI Contrast available",
    "    Range: %lu - %lu, Current: %lu",
    "    [X] DDC/CI Contrast NOT available",
    "    Capability flags: 0x%lx",
    "    Color temp flags: 0x%lx",
    "    [VCP 0x10] Brightness (low-level): cur=%lu, max=%lu, type=%s",
    "Summary: %d monitor(s) detected",
    "[!] No monitors detected.",
    "    Make sure your monitor is connected and powered on.",
    "Usage:",
    "  ddc_test              Enumerate monitors and show DDC/CI info",
    "  ddc_test --get        Show current brightness of all monitors",
    "  ddc_test --set <N>    Set all monitors brightness to N%% (0-100)",
    "  ddc_test --lang cn    Use Simplified Chinese output",
    "  ddc_test --help       Show this help",
    "Current brightness:",
    "Setting all DDC/CI monitors to %lu%%...",
    "Done, processed %d monitor(s)",
    "  Monitor #%d.%lu: brightness set to %lu%% (DDC: %lu, range %lu-%lu)",
    "  Monitor #%d.%lu: set failed, error: %lu",
    "  Monitor #%d.%lu: DDC/CI not available, skipped",
    "  [!] GetMonitorInfo failed, error: %lu",
    "  [!] GetPhysicalMonitors failed, error: %lu",
    "  [!] Memory allocation failed",
    "  [!] GetNumberOfPhysicalMonitors failed, error: %lu",
};

static const Lang LANG_CN = {
    "  DDC/CI \xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8" "\xe6\xb5\x8b\xe8\xaf\x95\xe5\xb7\xa5\xe5\x85\xb7",
    "  \xe4\xbd\x9c\xe8\x80\x85: " AUTHOR "  |  \xe7\x89\x88\xe6\x9c\xac: " VERSION,
    "--- \xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8 #%d ---",
    "  \xe8\xae\xbe\xe5\xa4\x87\xe5\x90\x8d\xe7\xa7\xb0: %ls",
    "  \xe5\x8c\xba\xe5\x9f\x9f: (%ld,%ld)-(%ld,%ld)",
    "\xe6\x98\xaf", "\xe5\x90\xa6",
    "  \xe4\xb8\xbb\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8: %s",
    "  \xe7\x89\xa9\xe7\x90\x86\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8" "\xe6\x95\xb0\xe9\x87\x8f: %lu",
    "  \xe7\x89\xa9\xe7\x90\x86\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8 #%lu:",
    "    \xe6\x8f\x8f\xe8\xbf\xb0: %ls",
    "    [OK] DDC/CI \xe4\xba\xae\xe5\xba\xa6" "\xe6\x8e\xa7\xe5\x88\xb6\xe5\x8f\xaf\xe7\x94\xa8",
    "    \xe4\xba\xae\xe5\xba\xa6" "\xe8\x8c\x83\xe5\x9b\xb4: %lu - %lu",
    "    \xe5\xbd\x93\xe5\x89\x8d\xe4\xba\xae\xe5\xba\xa6: %lu (DDC/CI \xe5\x8e\x9f\xe5\xa7\x8b\xe5\x80\xbc)",
    "    \xe5\xbd\x93\xe5\x89\x8d\xe4\xba\xae\xe5\xba\xa6: %lu%% (\xe6\x98\xa0\xe5\xb0\x84\xe5\x90\x8e)",
    "    \xe6\x98\xa0\xe5\xb0\x84: 1:1 (\xe6\x97\xa0\xe9\x9c\x80\xe8\xbd\xac\xe6\x8d\xa2)",
    "    \xe6\x98\xa0\xe5\xb0\x84: \xe8\x8c\x83\xe5\x9b\xb4" "\xe8\xbd\xac\xe6\x8d\xa2 [%lu-%lu] <-> [0-100]",
    "    [X] DDC/CI \xe4\xba\xae\xe5\xba\xa6" "\xe6\x8e\xa7\xe5\x88\xb6\xe4\xb8\x8d\xe5\x8f\xaf\xe7\x94\xa8 (\xe9\x94\x99\xe8\xaf\xaf\xe7\xa0\x81: %lu)",
    "    [OK] DDC/CI \xe5\xaf\xb9\xe6\xaf\x94\xe5\xba\xa6" "\xe6\x8e\xa7\xe5\x88\xb6\xe5\x8f\xaf\xe7\x94\xa8",
    "    \xe5\xaf\xb9\xe6\xaf\x94\xe5\xba\xa6" "\xe8\x8c\x83\xe5\x9b\xb4: %lu - %lu, \xe5\xbd\x93\xe5\x89\x8d: %lu",
    "    [X] DDC/CI \xe5\xaf\xb9\xe6\xaf\x94\xe5\xba\xa6" "\xe6\x8e\xa7\xe5\x88\xb6\xe4\xb8\x8d\xe5\x8f\xaf\xe7\x94\xa8",
    "    \xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8" "\xe8\x83\xbd\xe5\x8a\x9b\xe6\xa0\x87\xe5\xbf\x97: 0x%lx",
    "    \xe6\x94\xaf\xe6\x8c\x81\xe7\x9a\x84\xe8\x89\xb2\xe6\xb8\xa9: 0x%lx",
    "    [VCP 0x10] \xe4\xba\xae\xe5\xba\xa6 (\xe4\xbd\x8e\xe7\xba\xa7" "API): \xe5\xbd\x93\xe5\x89\x8d=%lu, \xe6\x9c\x80\xe5\xa4\xa7=%lu, \xe7\xb1\xbb\xe5\x9e\x8b=%s",
    "\xe6\x80\xbb\xe7\xbb\x93: \xe6\xa3\x80\xe6\xb5\x8b\xe5\x88\xb0 %d \xe5\x8f\xb0\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8",
    "[!] \xe6\x9c\xaa\xe6\xa3\x80\xe6\xb5\x8b\xe5\x88\xb0\xe4\xbb\xbb\xe4\xbd\x95\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8\xe3\x80\x82",
    "    \xe8\xaf\xb7\xe7\xa1\xae\xe4\xbf\x9d\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8\xe5\xb7\xb2\xe8\xbf\x9e\xe6\x8e\xa5\xe5\xb9\xb6\xe5\xa4\x84\xe4\xba\x8e\xe5\xbc\x80\xe5\x90\xaf\xe7\x8a\xb6\xe6\x80\x81\xe3\x80\x82",
    "\xe7\x94\xa8\xe6\xb3\x95:",
    "  ddc_test              \xe6\x9e\x9a\xe4\xb8\xbe\xe6\x89\x80\xe6\x9c\x89\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8\xe5\xb9\xb6\xe6\x98\xbe\xe7\xa4\xba DDC/CI \xe4\xbf\xa1\xe6\x81\xaf",
    "  ddc_test --get        \xe6\x98\xbe\xe7\xa4\xba\xe6\x89\x80\xe6\x9c\x89\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8\xe5\xbd\x93\xe5\x89\x8d\xe4\xba\xae\xe5\xba\xa6",
    "  ddc_test --set <N>    \xe8\xae\xbe\xe7\xbd\xae\xe6\x89\x80\xe6\x9c\x89\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8\xe4\xba\xae\xe5\xba\xa6\xe4\xb8\xba N%% (0-100)",
    "  ddc_test --lang cn    \xe4\xbd\xbf\xe7\x94\xa8\xe7\xae\x80\xe4\xbd\x93\xe4\xb8\xad\xe6\x96\x87\xe8\xbe\x93\xe5\x87\xba",
    "  ddc_test --help       \xe6\x98\xbe\xe7\xa4\xba\xe5\xb8\xae\xe5\x8a\xa9",
    "\xe5\xbd\x93\xe5\x89\x8d\xe4\xba\xae\xe5\xba\xa6:",
    "\xe8\xae\xbe\xe7\xbd\xae\xe6\x89\x80\xe6\x9c\x89 DDC/CI \xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8\xe4\xba\xae\xe5\xba\xa6\xe5\x88\xb0 %lu%%...",
    "\xe5\xae\x8c\xe6\x88\x90\xef\xbc\x8c\xe5\x85\xb1\xe5\xa4\x84\xe7\x90\x86 %d \xe5\x8f\xb0\xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8",
    "  \xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8 #%d.%lu: \xe4\xba\xae\xe5\xba\xa6\xe8\xae\xbe\xe7\xbd\xae\xe4\xb8\xba %lu%% (DDC\xe5\x80\xbc: %lu, \xe8\x8c\x83\xe5\x9b\xb4 %lu-%lu)",
    "  \xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8 #%d.%lu: \xe8\xae\xbe\xe7\xbd\xae\xe5\xa4\xb1\xe8\xb4\xa5, \xe9\x94\x99\xe8\xaf\xaf\xe7\xa0\x81: %lu",
    "  \xe6\x98\xbe\xe7\xa4\xba\xe5\x99\xa8 #%d.%lu: DDC/CI \xe4\xb8\x8d\xe5\x8f\xaf\xe7\x94\xa8, \xe8\xb7\xb3\xe8\xbf\x87",
    "  [!] GetMonitorInfo \xe5\xa4\xb1\xe8\xb4\xa5, \xe9\x94\x99\xe8\xaf\xaf\xe7\xa0\x81: %lu",
    "  [!] GetPhysicalMonitors \xe5\xa4\xb1\xe8\xb4\xa5, \xe9\x94\x99\xe8\xaf\xaf\xe7\xa0\x81: %lu",
    "  [!] \xe5\x86\x85\xe5\xad\x98\xe5\x88\x86\xe9\x85\x8d\xe5\xa4\xb1\xe8\xb4\xa5",
    "  [!] GetNumberOfPhysicalMonitors \xe5\xa4\xb1\xe8\xb4\xa5, \xe9\x94\x99\xe8\xaf\xaf\xe7\xa0\x81: %lu",
};

static const Lang *L = &LANG_EN;

//
// Helpers
//
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

//
// Parse --lang from argv (can appear anywhere)
//
static void ParseLang(int argc, char *argv[])
{
    for (int i = 1; i < argc - 1; i++) {
        if (strcmp(argv[i], "--lang") == 0) {
            if (strcmp(argv[i + 1], "cn") == 0 ||
                strcmp(argv[i + 1], "zh") == 0 ||
                strcmp(argv[i + 1], "zh-cn") == 0) {
                g_langCN = 1;
                L = &LANG_CN;
            }
        }
    }
}

static void TestMonitor(HMONITOR hMonitor, int monitorIndex)
{
    MONITORINFOEXW monInfo = {};
    monInfo.cbSize = sizeof(monInfo);
    if (!GetMonitorInfoW(hMonitor, &monInfo)) {
        printf(L->getmoninfo_fail, GetLastError());
        printf("\n");
        return;
    }

    printf("\n");
    printf(L->monitor_header, monitorIndex);
    printf("\n");
    printf(L->device, monInfo.szDevice);
    printf("\n");
    printf(L->area,
           monInfo.rcMonitor.left, monInfo.rcMonitor.top,
           monInfo.rcMonitor.right, monInfo.rcMonitor.bottom);
    printf("\n");
    printf(L->primary_label,
           (monInfo.dwFlags & MONITORINFOF_PRIMARY) ? L->primary_yes : L->primary_no);
    printf("\n");

    DWORD numPhysical = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &numPhysical)) {
        printf(L->getnumphys_fail, GetLastError());
        printf("\n");
        return;
    }

    printf(L->physical_count, numPhysical);
    printf("\n");

    PHYSICAL_MONITOR *physicalMonitors = (PHYSICAL_MONITOR *)
        malloc(sizeof(PHYSICAL_MONITOR) * numPhysical);
    if (!physicalMonitors) {
        printf("%s\n", L->alloc_fail);
        return;
    }

    if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, numPhysical,
                                          physicalMonitors)) {
        printf(L->getphysmon_fail, GetLastError());
        printf("\n");
        free(physicalMonitors);
        return;
    }

    for (DWORD i = 0; i < numPhysical; i++) {
        printf("\n");
        printf(L->physical_header, i);
        printf("\n");
        printf(L->description, physicalMonitors[i].szPhysicalMonitorDescription);
        printf("\n");

        HANDLE hPhysical = physicalMonitors[i].hPhysicalMonitor;

        DWORD minBrightness = 0, curBrightness = 0, maxBrightness = 0;
        if (GetMonitorBrightness(hPhysical, &minBrightness, &curBrightness, &maxBrightness)) {
            printf("%s\n", L->brightness_ok);
            printf(L->brightness_range, minBrightness, maxBrightness); printf("\n");
            printf(L->brightness_current_raw, curBrightness); printf("\n");
            printf(L->brightness_current_mapped,
                   MapDDCToPercent(curBrightness, minBrightness, maxBrightness)); printf("\n");

            if (minBrightness == 0 && maxBrightness == 100) {
                printf("%s\n", L->mapping_1to1);
            } else {
                printf(L->mapping_convert, minBrightness, maxBrightness); printf("\n");
            }
        } else {
            printf(L->brightness_fail, GetLastError()); printf("\n");
        }

        DWORD minContrast = 0, curContrast = 0, maxContrast = 0;
        if (GetMonitorContrast(hPhysical, &minContrast, &curContrast, &maxContrast)) {
            printf("%s\n", L->contrast_ok);
            printf(L->contrast_range, minContrast, maxContrast, curContrast); printf("\n");
        } else {
            printf("%s\n", L->contrast_fail);
        }

        DWORD capFlags = 0, colorTempFlags = 0;
        if (GetMonitorCapabilities(hPhysical, &capFlags, &colorTempFlags)) {
            printf(L->cap_flags, capFlags); printf("\n");
            printf(L->color_temp, colorTempFlags); printf("\n");
        }

        MC_VCP_CODE_TYPE codeType;
        DWORD currentValue = 0, maxValue = 0;
        if (GetVCPFeatureAndVCPFeatureReply(hPhysical, 0x10, &codeType, &currentValue, &maxValue)) {
            printf(L->vcp_brightness, currentValue, maxValue,
                   (codeType == MC_MOMENTARY) ? "Momentary" : "Continuous"); printf("\n");
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

    PHYSICAL_MONITOR *pm = (PHYSICAL_MONITOR *)malloc(sizeof(PHYSICAL_MONITOR) * numPhysical);
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
                printf(L->set_ok, ctx->count, i, g_targetBrightness, targetDDC, minB, maxB);
                printf("\n");
            } else {
                printf(L->set_fail, ctx->count, i, GetLastError()); printf("\n");
            }
        } else {
            printf(L->not_available, ctx->count, i); printf("\n");
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

    PHYSICAL_MONITOR *pm = (PHYSICAL_MONITOR *)malloc(sizeof(PHYSICAL_MONITOR) * numPhysical);
    if (!pm) return TRUE;

    if (!GetPhysicalMonitorsFromHMONITOR(hMonitor, numPhysical, pm)) {
        free(pm);
        return TRUE;
    }

    for (DWORD i = 0; i < numPhysical; i++) {
        DWORD minB = 0, curB = 0, maxB = 0;
        if (GetMonitorBrightness(pm[i].hPhysicalMonitor, &minB, &curB, &maxB)) {
            printf("  Monitor #%d.%lu: %lu%% (DDC: %lu, range %lu-%lu)\n",
                   ctx->count, i, MapDDCToPercent(curB, minB, maxB), curB, minB, maxB);
        } else {
            printf(L->not_available, ctx->count, i); printf("\n");
        }
    }

    DestroyPhysicalMonitors(numPhysical, pm);
    free(pm);
    return TRUE;
}

static void PrintUsage(void)
{
    printf("\n%s\n", L->usage_header);
    printf("%s\n", L->usage_enum);
    printf("%s\n", L->usage_get);
    printf("%s\n", L->usage_set);
    printf("%s\n", L->usage_lang);
    printf("%s\n", L->usage_help);
}

int main(int argc, char *argv[])
{
    SetConsoleOutputCP(65001);
    ParseLang(argc, argv);

    printf("========================================\n");
    printf("%s\n", L->banner);
    printf("%s\n", L->author_line);
    printf("========================================\n");

    if (argc >= 2 && strcmp(argv[1], "--get") == 0) {
        printf("\n%s\n", L->current_brightness);
        ENUM_CONTEXT ctx = {};
        EnumDisplayMonitors(NULL, NULL, EnumProc_GetBrightness, (LPARAM)&ctx);
        if (ctx.count == 0) printf("  %s\n", L->no_monitors);
        return 0;
    }

    if (argc >= 3 && strcmp(argv[1], "--set") == 0) {
        g_targetBrightness = (DWORD)atoi(argv[2]);
        if (g_targetBrightness > 100) g_targetBrightness = 100;

        printf("\n");
        printf(L->setting_all, g_targetBrightness); printf("\n");

        ENUM_CONTEXT ctx = {};
        EnumDisplayMonitors(NULL, NULL, EnumProc_SetBrightness, (LPARAM)&ctx);

        printf("\n");
        printf(L->done, ctx.count); printf("\n");
        return 0;
    }

    if (argc >= 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        PrintUsage();
        return 0;
    }

    // Default: enumerate and test
    ENUM_CONTEXT ctx = {};
    EnumDisplayMonitors(NULL, NULL, EnumProc_Info, (LPARAM)&ctx);

    printf("\n========================================\n");
    printf(L->summary, ctx.count); printf("\n");
    printf("========================================\n");

    if (ctx.count == 0) {
        printf("\n%s\n", L->no_monitors);
        printf("%s\n", L->no_monitors_hint);
    }

    PrintUsage();
    return 0;
}
