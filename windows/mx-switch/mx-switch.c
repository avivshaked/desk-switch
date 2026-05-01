#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <setupapi.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#define DEFAULT_VID 0x046D
#define DEFAULT_PID 0xC548
#define DEFAULT_USAGE_PAGE 0xFF00
#define DEFAULT_USAGE 0x0001
#define DEFAULT_DEVICE_INDEX 0x02
#define DEFAULT_FEATURE_INDEX 0x0A
#define DEFAULT_FUNCTION_INDEX 0x1B
#define SHORT_REPORT_LENGTH 7
#define LONG_REPORT_LENGTH 20

typedef enum ReportKind {
    REPORT_SHORT,
    REPORT_LONG
} ReportKind;

/*
 * HID-capable executable. Keep this narrow: it has a guarded write path and
 * should only be run for actual switching after reviewing the planner output.
 */
typedef enum Command {
    CMD_NONE,
    CMD_LIST,
    CMD_DRY_RUN,
    CMD_SWITCH
} Command;

typedef struct Options {
    Command command;
    USHORT vid;
    USHORT pid;
    USHORT usage_page;
    USHORT usage;
    BYTE device_index;
    BYTE feature_index;
    BYTE function_index;
    ReportKind report_kind;
    int slot;
    bool execute;
    bool all;
    bool read_response;
    int timeout_ms;
    wchar_t path[MAX_PATH * 4];
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
    puts("mx-switch: narrow Logitech MX Master 3S Easy-Switch helper");
    puts("");
    puts("Usage:");
    puts("  mx-switch.exe list [--all]");
    puts("  mx-switch.exe dry-run --slot <1|2|3> [options]");
    puts("  mx-switch.exe switch --slot <1|2|3> --execute [options]");
    puts("");
    puts("Options:");
    puts("  --vidpid <VID:PID>          Default: 046D:C548");
    puts("  --usage-page <number>       Default: 0xFF00");
    puts("  --usage <number>            Default: 0x0001");
    puts("  --device-index <number>     Default: 0x02");
    puts("  --feature-index <number>    Default: 0x0A");
    puts("  --function-index <number>   Default: 0x1B");
    puts("  --report <short|long>       Default: short");
    puts("  --path <device path>        Open one exact HID path");
    puts("  --read-response             Try to read one response after writing");
    puts("  --timeout-ms <number>       Response timeout, default: 500");
    puts("");
    puts("Safety:");
    puts("  dry-run prints the report and does not write.");
    puts("  switch refuses to write unless --execute is present.");
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

static bool parse_byte(const char *text, BYTE *out)
{
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 0);
    if (end == text || *end != '\0' || value > 0xFF) {
        return false;
    }
    *out = (BYTE)value;
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

static bool utf8ish_to_wide(const char *src, wchar_t *dst, size_t dst_count)
{
    int written = MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, (int)dst_count);
    if (written > 0) {
        return true;
    }

    written = MultiByteToWideChar(CP_ACP, 0, src, -1, dst, (int)dst_count);
    return written > 0;
}

static void init_options(Options *options)
{
    memset(options, 0, sizeof(*options));
    options->vid = DEFAULT_VID;
    options->pid = DEFAULT_PID;
    options->usage_page = DEFAULT_USAGE_PAGE;
    options->usage = DEFAULT_USAGE;
    options->device_index = DEFAULT_DEVICE_INDEX;
    options->feature_index = DEFAULT_FEATURE_INDEX;
    options->function_index = DEFAULT_FUNCTION_INDEX;
    options->report_kind = REPORT_SHORT;
    options->timeout_ms = 500;
}

static bool parse_args(int argc, char **argv, Options *options)
{
    init_options(options);

    if (argc < 2) {
        print_usage();
        return false;
    }

    if (strcmp(argv[1], "list") == 0) {
        options->command = CMD_LIST;
    } else if (strcmp(argv[1], "dry-run") == 0) {
        options->command = CMD_DRY_RUN;
    } else if (strcmp(argv[1], "switch") == 0) {
        options->command = CMD_SWITCH;
    } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage();
        return false;
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage();
        return false;
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) {
            options->all = true;
        } else if (strcmp(argv[i], "--execute") == 0) {
            options->execute = true;
        } else if (strcmp(argv[i], "--slot") == 0 && i + 1 < argc) {
            options->slot = atoi(argv[++i]);
            if (options->slot < 1 || options->slot > 3) {
                fprintf(stderr, "--slot must be 1, 2, or 3\n");
                return false;
            }
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
        } else if (strcmp(argv[i], "--device-index") == 0 && i + 1 < argc) {
            if (!parse_byte(argv[++i], &options->device_index)) {
                fprintf(stderr, "Invalid --device-index value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--feature-index") == 0 && i + 1 < argc) {
            if (!parse_byte(argv[++i], &options->feature_index)) {
                fprintf(stderr, "Invalid --feature-index value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--function-index") == 0 && i + 1 < argc) {
            if (!parse_byte(argv[++i], &options->function_index)) {
                fprintf(stderr, "Invalid --function-index value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--report") == 0 && i + 1 < argc) {
            const char *kind = argv[++i];
            if (strcmp(kind, "short") == 0) {
                options->report_kind = REPORT_SHORT;
            } else if (strcmp(kind, "long") == 0) {
                options->report_kind = REPORT_LONG;
            } else {
                fprintf(stderr, "--report must be short or long\n");
                return false;
            }
        } else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
            if (!utf8ish_to_wide(argv[++i], options->path, _countof(options->path))) {
                fprintf(stderr, "Invalid --path value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--read-response") == 0) {
            options->read_response = true;
        } else if (strcmp(argv[i], "--timeout-ms") == 0 && i + 1 < argc) {
            int timeout = atoi(argv[++i]);
            if (timeout < 0 || timeout > 10000) {
                fprintf(stderr, "--timeout-ms must be between 0 and 10000\n");
                return false;
            }
            options->timeout_ms = timeout;
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }

    if ((options->command == CMD_DRY_RUN || options->command == CMD_SWITCH) && options->slot == 0) {
        fprintf(stderr, "%s requires --slot <1|2|3>\n", argv[1]);
        return false;
    }

    if (options->command == CMD_SWITCH && !options->execute) {
        /* Deliberate friction: accidental switch commands must fail closed. */
        fprintf(stderr, "Refusing to write. Add --execute only after reviewing dry-run output.\n");
        return false;
    }

    return true;
}

static int report_length(const Options *options)
{
    return options->report_kind == REPORT_LONG ? LONG_REPORT_LENGTH : SHORT_REPORT_LENGTH;
}

static void build_report(const Options *options, BYTE report[LONG_REPORT_LENGTH])
{
    memset(report, 0, LONG_REPORT_LENGTH);

    /* Candidate short HID++ report used by community Bolt Easy-Switch examples. */
    report[0] = options->report_kind == REPORT_LONG ? 0x11 : 0x10;
    report[1] = options->device_index;
    report[2] = options->feature_index;
    report[3] = options->function_index;

    /* Community examples map channel bytes as slot 1 = 0, slot 2 = 1, slot 3 = 2. */
    report[4] = (BYTE)(options->slot - 1);
}

static void print_report(const BYTE *report, int length)
{
    for (int i = 0; i < length; i++) {
        printf("%s0x%02X", i == 0 ? "" : ",", report[i]);
    }
    puts("");
}

static HANDLE open_hid_path(const wchar_t *path, DWORD desired_access)
{
    return CreateFileW(path,
                       desired_access,
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

    /* desiredAccess = 0 keeps inventory reads metadata-only. */
    HANDLE handle = open_hid_path(path, 0);
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

static int enumerate_hids(const Options *options, HidInfo *selected, int *match_count)
{
    GUID hid_guid;
    HidD_GetHidGuid(&hid_guid);

    HDEVINFO devs = SetupDiGetClassDevsW(&hid_guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (devs == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "SetupDiGetClassDevs failed: %lu\n", GetLastError());
        return 1;
    }

    *match_count = 0;
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
                bool match = options->all || matches_options(&info, options);
                if (match) {
                    print_hid_info(&info);
                }
                if (!options->all && matches_options(&info, options)) {
                    (*match_count)++;
                    if (*match_count == 1 && selected) {
                        *selected = info;
                    }
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

    return 0;
}

static int dry_run(const Options *options)
{
    BYTE report[LONG_REPORT_LENGTH];
    build_report(options, report);

    printf("Target VID/PID: 0x%04X:0x%04X\n", options->vid, options->pid);
    printf("Target usagePage/usage: 0x%04X/0x%04X\n", options->usage_page, options->usage);
    printf("Target slot: %d\n", options->slot);
    printf("Device index: 0x%02X\n", options->device_index);
    printf("Feature/function index: 0x%02X/0x%02X\n", options->feature_index, options->function_index);
    printf("Report kind: %s\n", options->report_kind == REPORT_LONG ? "long" : "short");
    printf("Output report length: %d\n", report_length(options));
    printf("Output report payload: ");
    print_report(report, report_length(options));
    puts("No HID report was sent.");
    return 0;
}

static int switch_slot(const Options *options)
{
    BYTE report[LONG_REPORT_LENGTH];
    BYTE response[LONG_REPORT_LENGTH];
    build_report(options, report);

    HidInfo selected;
    int match_count = 0;

    if (options->path[0] != L'\0') {
        wcsncpy_s(selected.path, _countof(selected.path), options->path, _TRUNCATE);
        match_count = 1;
    } else {
        int enum_result = enumerate_hids(options, &selected, &match_count);
        if (enum_result != 0) {
            return enum_result;
        }
    }

    if (match_count == 0) {
        fprintf(stderr, "No matching HID interface found.\n");
        return 1;
    }
    if (match_count > 1) {
        fprintf(stderr, "More than one matching HID interface found. Re-run with --path for the exact target.\n");
        return 1;
    }

    wprintf(L"Opening for write: %ls\n", selected.path);
    printf("Writing output report: ");
    print_report(report, report_length(options));

    /* This is the only write-capable open in the program. */
    DWORD desired_access = options->read_response ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_WRITE;
    HANDLE handle = open_hid_path(selected.path, desired_access);
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFile for write failed: %lu\n", GetLastError());
        return 1;
    }

    DWORD written = 0;
    BOOL ok = WriteFile(handle, report, (DWORD)report_length(options), &written, NULL);
    DWORD write_error = GetLastError();

    if (!ok) {
        fprintf(stderr, "WriteFile failed: %lu\n", write_error);
        CloseHandle(handle);
        return 1;
    }

    printf("Wrote %lu bytes.\n", written);

    if (options->read_response) {
        COMMTIMEOUTS timeouts;
        memset(&timeouts, 0, sizeof(timeouts));
        timeouts.ReadTotalTimeoutConstant = (DWORD)options->timeout_ms;
        (void)SetCommTimeouts(handle, &timeouts);

        memset(response, 0, sizeof(response));
        DWORD read = 0;
        BOOL read_ok = ReadFile(handle, response, (DWORD)report_length(options), &read, NULL);
        DWORD read_error = GetLastError();

        if (read_ok) {
            printf("Read %lu response bytes: ", read);
            print_report(response, (int)read);
        } else {
            printf("ReadFile response failed: %lu\n", read_error);
        }
    }

    CloseHandle(handle);
    return written == (DWORD)report_length(options) ? 0 : 1;
}

int main(int argc, char **argv)
{
    Options options;
    if (!parse_args(argc, argv, &options)) {
        return 2;
    }

    if (options.command == CMD_LIST) {
        HidInfo selected;
        int match_count = 0;
        int result = enumerate_hids(&options, &selected, &match_count);
        if (!options.all) {
            printf("Matching interfaces: %d\n", match_count);
        }
        return result;
    }

    if (options.command == CMD_DRY_RUN) {
        return dry_run(&options);
    }

    if (options.command == CMD_SWITCH) {
        return switch_slot(&options);
    }

    print_usage();
    return 2;
}
