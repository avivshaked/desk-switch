#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define DEFAULT_VID 0x046D
#define DEFAULT_PID 0xC548
#define DEFAULT_USAGE_PAGE 0xFF00
#define DEFAULT_USAGE 0x0001

/*
 * Read-only inventory executable. It imports Windows HID/setup APIs, but it
 * contains no output-report payloads and no write handle path.
 */
typedef struct Options {
    USHORT vid;
    USHORT pid;
    USHORT usage_page;
    USHORT usage;
    bool all;
} Options;

typedef struct HidInfo {
    wchar_t path[MAX_PATH * 4];
    USHORT vid;
    USHORT pid;
    USAGE usage_page;
    USAGE usage;
    USHORT input_report_length;
    USHORT output_report_length;
    USHORT feature_report_length;
    wchar_t manufacturer[128];
    wchar_t product[128];
} HidInfo;

static void print_usage(void)
{
    puts("mx-switch-list: read-only HID inventory for Logitech MX Master 3S experiments");
    puts("");
    puts("Usage:");
    puts("  mx-switch-list.exe [--all] [options]");
    puts("");
    puts("Options:");
    puts("  --all                    Print every HID interface, not only target matches");
    puts("  --vidpid <VID:PID>       Default: 046D:C548");
    puts("  --usage-page <number>    Default: 0xFF00");
    puts("  --usage <number>         Default: 0x0001");
    puts("");
    puts("This program opens HID paths with metadata-only access and has no write path.");
}

static bool parse_u16(const char *text, USHORT *out)
{
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 0);
    if (end == text || *end != '\0' || value > 0xFFFF) {
        return false;
    }
    *out = (USHORT)value;
    return true;
}

static bool parse_vidpid(const char *text, USHORT *vid, USHORT *pid)
{
    char left[16] = {0};
    char right[16] = {0};
    if (sscanf_s(text, "%15[^:/]:%15s", left, (unsigned)_countof(left), right, (unsigned)_countof(right)) != 2 &&
        sscanf_s(text, "%15[^:/]/%15s", left, (unsigned)_countof(left), right, (unsigned)_countof(right)) != 2) {
        return false;
    }

    return parse_u16(left, vid) && parse_u16(right, pid);
}

static void init_options(Options *options)
{
    memset(options, 0, sizeof(*options));
    options->vid = DEFAULT_VID;
    options->pid = DEFAULT_PID;
    options->usage_page = DEFAULT_USAGE_PAGE;
    options->usage = DEFAULT_USAGE;
}

static bool parse_args(int argc, char **argv, Options *options)
{
    init_options(options);

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage();
            return false;
        } else if (strcmp(argv[i], "--all") == 0) {
            options->all = true;
        } else if (strcmp(argv[i], "--vidpid") == 0 && i + 1 < argc) {
            if (!parse_vidpid(argv[++i], &options->vid, &options->pid)) {
                fprintf(stderr, "Invalid --vidpid value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--usage-page") == 0 && i + 1 < argc) {
            if (!parse_u16(argv[++i], &options->usage_page)) {
                fprintf(stderr, "Invalid --usage-page value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--usage") == 0 && i + 1 < argc) {
            if (!parse_u16(argv[++i], &options->usage)) {
                fprintf(stderr, "Invalid --usage value\n");
                return false;
            }
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }

    return true;
}

static HANDLE open_hid_path_for_metadata(const wchar_t *path)
{
    /*
     * desiredAccess = 0 asks Windows for metadata access only. This lets
     * HidD_GetAttributes / HidP_GetCaps inspect the interface without a
     * read or write handle.
     */
    return CreateFileW(path,
                       0,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
}

static bool read_hid_info(const wchar_t *path, HidInfo *info)
{
    memset(info, 0, sizeof(*info));
    wcsncpy_s(info->path, _countof(info->path), path, _TRUNCATE);

    HANDLE handle = open_hid_path_for_metadata(path);
    if (handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    HIDD_ATTRIBUTES attributes;
    memset(&attributes, 0, sizeof(attributes));
    attributes.Size = sizeof(attributes);
    if (!HidD_GetAttributes(handle, &attributes)) {
        CloseHandle(handle);
        return false;
    }

    info->vid = attributes.VendorID;
    info->pid = attributes.ProductID;
    (void)HidD_GetManufacturerString(handle, info->manufacturer, sizeof(info->manufacturer));
    (void)HidD_GetProductString(handle, info->product, sizeof(info->product));

    PHIDP_PREPARSED_DATA preparsed = NULL;
    if (HidD_GetPreparsedData(handle, &preparsed)) {
        HIDP_CAPS caps;
        if (HidP_GetCaps(preparsed, &caps) == HIDP_STATUS_SUCCESS) {
            /* Usage page/usage identify the vendor-defined Logitech interface. */
            info->usage_page = caps.UsagePage;
            info->usage = caps.Usage;
            info->input_report_length = caps.InputReportByteLength;
            info->output_report_length = caps.OutputReportByteLength;
            info->feature_report_length = caps.FeatureReportByteLength;
        }
        HidD_FreePreparsedData(preparsed);
    }

    CloseHandle(handle);
    return true;
}

static bool matches_options(const HidInfo *info, const Options *options)
{
    return info->vid == options->vid &&
           info->pid == options->pid &&
           info->usage_page == options->usage_page &&
           info->usage == options->usage;
}

static void print_hid_info(const HidInfo *info)
{
    wprintf(L"%04X:%04X usagePage=0x%04X usage=0x%04X\n",
            info->vid,
            info->pid,
            info->usage_page,
            info->usage);
    wprintf(L"  report lengths: input=%u output=%u feature=%u\n",
            info->input_report_length,
            info->output_report_length,
            info->feature_report_length);
    wprintf(L"  manufacturer: %ls\n", info->manufacturer[0] ? info->manufacturer : L"(unknown)");
    wprintf(L"  product:      %ls\n", info->product[0] ? info->product : L"(unknown)");
    wprintf(L"  path:         %ls\n\n", info->path);
}

int main(int argc, char **argv)
{
    Options options;
    if (!parse_args(argc, argv, &options)) {
        return 2;
    }

    GUID hid_guid;
    HidD_GetHidGuid(&hid_guid);

    HDEVINFO devs = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devs == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "SetupDiGetClassDevs failed: %lu\n", GetLastError());
        return 1;
    }

    int match_count = 0;
    SP_DEVICE_INTERFACE_DATA interface_data;
    memset(&interface_data, 0, sizeof(interface_data));
    interface_data.cbSize = sizeof(interface_data);

    for (DWORD index = 0; SetupDiEnumDeviceInterfaces(devs, NULL, &hid_guid, index, &interface_data); index++) {
        DWORD required_size = 0;

        /*
         * First call obtains the variable buffer size required for the HID
         * device path; the second call below fills that buffer.
         */
        (void)SetupDiGetDeviceInterfaceDetailW(devs, &interface_data, NULL, 0, &required_size, NULL);
        if (required_size == 0) {
            continue;
        }

        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail =
            (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)calloc(1, required_size);
        if (!detail) {
            SetupDiDestroyDeviceInfoList(devs);
            fprintf(stderr, "Out of memory\n");
            return 1;
        }

        detail->cbSize = sizeof(*detail);
        if (SetupDiGetDeviceInterfaceDetailW(devs, &interface_data, detail, required_size, NULL, NULL)) {
            HidInfo info;
            if (read_hid_info(detail->DevicePath, &info)) {
                bool is_match = matches_options(&info, &options);
                if (is_match) {
                    match_count++;
                }
                if (options.all || is_match) {
                    print_hid_info(&info);
                }
            }
        }

        free(detail);
    }

    DWORD last_error = GetLastError();
    SetupDiDestroyDeviceInfoList(devs);

    if (last_error != ERROR_NO_MORE_ITEMS) {
        fprintf(stderr, "SetupDiEnumDeviceInterfaces failed: %lu\n", last_error);
        return 1;
    }

    if (!options.all) {
        printf("Matching interfaces: %d\n", match_count);
    }

    return 0;
}
