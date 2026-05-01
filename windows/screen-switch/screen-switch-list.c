#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <physicalmonitorenumerationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_SOURCE_VCP 0x60

typedef struct MonitorContext {
    int display_index;
} MonitorContext;

static const char *input_source_name(DWORD value)
{
    /*
     * MCCS VCP 0x60 common values. Monitors are not perfectly consistent, so
     * treat these labels as hints and trust the monitor capabilities string.
     */
    switch (value) {
    case 0x01: return "VGA-1";
    case 0x02: return "VGA-2";
    case 0x03: return "DVI-1";
    case 0x04: return "DVI-2";
    case 0x05: return "Composite-1";
    case 0x06: return "Composite-2";
    case 0x07: return "S-Video-1";
    case 0x08: return "S-Video-2";
    case 0x09: return "Tuner-1";
    case 0x0A: return "Tuner-2";
    case 0x0B: return "Tuner-3";
    case 0x0C: return "Component-1";
    case 0x0D: return "Component-2";
    case 0x0E: return "Component-3";
    case 0x0F: return "DisplayPort-1";
    case 0x10: return "DisplayPort-2";
    case 0x11: return "HDMI-1";
    case 0x12: return "HDMI-2";
    case 0x13: return "USB-C";
    default: return "unknown/vendor-specific";
    }
}

static void print_caps_line(const char *caps)
{
    const char *vcp = strstr(caps, "vcp(");
    if (vcp) {
        printf("    capabilities vcp excerpt: %.240s\n", vcp);
    } else {
        printf("    capabilities excerpt: %.240s\n", caps);
    }
}

static void read_capabilities(HANDLE monitor)
{
    DWORD length = 0;
    if (!GetCapabilitiesStringLength(monitor, &length) || length == 0) {
        printf("    capabilities: unavailable (error %lu)\n", GetLastError());
        return;
    }

    char *caps = (char *)calloc(length + 1, 1);
    if (!caps) {
        printf("    capabilities: out of memory\n");
        return;
    }

    if (!CapabilitiesRequestAndCapabilitiesReply(monitor, caps, length)) {
        printf("    capabilities: read failed (error %lu)\n", GetLastError());
        free(caps);
        return;
    }

    print_caps_line(caps);
    free(caps);
}

static void read_input_source(HANDLE monitor)
{
    MC_VCP_CODE_TYPE type;
    DWORD current = 0;
    DWORD maximum = 0;

    if (!GetVCPFeatureAndVCPFeatureReply(monitor, INPUT_SOURCE_VCP, &type, &current, &maximum)) {
        printf("    input source VCP 0x60: unavailable (error %lu)\n", GetLastError());
        return;
    }

    printf("    input source VCP 0x60: current=0x%02lX (%s), maximum=0x%02lX, type=%s\n",
           current,
           input_source_name(current),
           maximum,
           type == MC_SET_PARAMETER ? "set" : "momentary");
}

static BOOL CALLBACK enum_monitor_proc(HMONITOR hmonitor, HDC hdc, LPRECT rect, LPARAM data)
{
    (void)hdc;

    MonitorContext *context = (MonitorContext *)data;
    context->display_index++;

    MONITORINFOEXW info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);

    if (GetMonitorInfoW(hmonitor, (MONITORINFO *)&info)) {
        wprintf(L"Display monitor %d: %ls\n", context->display_index, info.szDevice);
        printf("  bounds: left=%ld top=%ld right=%ld bottom=%ld\n",
               rect->left,
               rect->top,
               rect->right,
               rect->bottom);
    } else {
        printf("Display monitor %d: GetMonitorInfo failed (error %lu)\n",
               context->display_index,
               GetLastError());
    }

    DWORD physical_count = 0;
    if (!GetNumberOfPhysicalMonitorsFromHMONITOR(hmonitor, &physical_count) || physical_count == 0) {
        printf("  physical monitors: unavailable (error %lu)\n\n", GetLastError());
        return TRUE;
    }

    PHYSICAL_MONITOR *physical = (PHYSICAL_MONITOR *)calloc(physical_count, sizeof(*physical));
    if (!physical) {
        printf("  physical monitors: out of memory\n\n");
        return TRUE;
    }

    if (!GetPhysicalMonitorsFromHMONITOR(hmonitor, physical_count, physical)) {
        printf("  physical monitors: enumeration failed (error %lu)\n\n", GetLastError());
        free(physical);
        return TRUE;
    }

    for (DWORD i = 0; i < physical_count; i++) {
        wprintf(L"  physical monitor %lu: %ls\n", i + 1, physical[i].szPhysicalMonitorDescription);
        read_input_source(physical[i].hPhysicalMonitor);
        read_capabilities(physical[i].hPhysicalMonitor);
    }

    DestroyPhysicalMonitors(physical_count, physical);
    free(physical);
    puts("");
    return TRUE;
}

int main(void)
{
    MonitorContext context;
    memset(&context, 0, sizeof(context));

    if (!EnumDisplayMonitors(NULL, NULL, enum_monitor_proc, (LPARAM)&context)) {
        fprintf(stderr, "EnumDisplayMonitors failed: %lu\n", GetLastError());
        return 1;
    }

    printf("Display monitors enumerated: %d\n", context.display_index);
    return 0;
}

