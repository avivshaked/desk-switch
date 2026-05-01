#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_VID 0x046D
#define DEFAULT_PID 0xB034
#define DEFAULT_USAGE_PAGE 0x0001
#define DEFAULT_USAGE 0x0002
#define DEFAULT_DEVICE_INDEX 0xFF
#define DEFAULT_FEATURE_INDEX 0x0A
#define DEFAULT_FUNCTION_INDEX 0x1B
#define SHORT_REPORT_LENGTH 7
#define LONG_REPORT_LENGTH 20
#define IORETURN_NOT_PERMITTED ((IOReturn)0xE00002E2)

typedef enum ReportKind {
    REPORT_SHORT,
    REPORT_LONG
} ReportKind;

typedef enum SendMode {
    SEND_WITHOUT_REPORT_ID,
    SEND_WITH_REPORT_ID
} SendMode;

typedef enum Command {
    CMD_NONE,
    CMD_LIST,
    CMD_DRY_RUN,
    CMD_SWITCH
} Command;

typedef struct Options {
    Command command;
    unsigned vid;
    unsigned pid;
    unsigned usage_page;
    unsigned usage;
    unsigned device_index;
    unsigned feature_index;
    unsigned function_index;
    bool has_pid;
    bool execute;
    bool all;
    bool skip_open;
    bool seize;
    ReportKind report_kind;
    SendMode send_mode;
    int slot;
    char registry_path[1024];
} Options;

typedef struct HidInfo {
    unsigned vid;
    unsigned pid;
    unsigned usage_page;
    unsigned usage;
    unsigned input_report_length;
    unsigned output_report_length;
    unsigned feature_report_length;
    char manufacturer[256];
    char product[256];
    char transport[128];
    char serial[256];
    char registry_path[1024];
} HidInfo;

typedef struct Selection {
    IOHIDDeviceRef device;
    HidInfo info;
    int match_count;
} Selection;

static void print_usage(void)
{
    puts("mx-switch-write: guarded macOS Logitech MX Master 3S Easy-Switch helper");
    puts("");
    puts("Usage:");
    puts("  mx-switch-write list [--all] [options]");
    puts("  mx-switch-write dry-run --slot <1|2|3> [options]");
    puts("  mx-switch-write switch --slot <1|2|3> --execute [options]");
    puts("");
    puts("Options:");
    puts("  --vidpid <VID:PID>          Match an exact vendor/product pair");
    puts("  --vid <number>              Default: 0x046D");
    puts("  --any-pid                   Do not require the default product ID");
    puts("  --usage-page <number>       Default: 0x0001");
    puts("  --usage <number>            Default: 0x0002");
    puts("  --device-index <number>     Default: 0xFF");
    puts("  --feature-index <number>    Default: 0x0A");
    puts("  --function-index <number>   Default: 0x1B");
    puts("  --report <short|long>       Default: long");
    puts("  --send <without-report-id|with-report-id>");
    puts("                                Default: with-report-id");
    puts("  --registry-path <path>      Open one exact IORegistry path from list output");
    puts("  --skip-open                 Try IOHIDDeviceSetReport without IOHIDDeviceOpen");
    puts("  --seize                     Open with kIOHIDOptionsTypeSeizeDevice");
    puts("");
    puts("Safety:");
    puts("  dry-run prints the report and does not open a HID device.");
    puts("  switch refuses to write unless --execute is present.");
}

static bool parse_number(const char *text, unsigned max, unsigned *out)
{
    char *end = NULL;
    unsigned long value = strtoul(text, &end, 0);
    if (end == text || *end != '\0' || value > max) {
        return false;
    }
    *out = (unsigned)value;
    return true;
}

static bool parse_vidpid(const char *text, unsigned *vid, unsigned *pid)
{
    char left[16] = {0};
    char right[16] = {0};
    if (sscanf(text, "%15[^:/]:%15s", left, right) != 2 &&
        sscanf(text, "%15[^:/]/%15s", left, right) != 2) {
        return false;
    }

    return parse_number(left, 0xFFFF, vid) && parse_number(right, 0xFFFF, pid);
}

static void init_options(Options *options)
{
    memset(options, 0, sizeof(*options));
    options->vid = DEFAULT_VID;
    options->pid = DEFAULT_PID;
    options->has_pid = true;
    options->usage_page = DEFAULT_USAGE_PAGE;
    options->usage = DEFAULT_USAGE;
    options->device_index = DEFAULT_DEVICE_INDEX;
    options->feature_index = DEFAULT_FEATURE_INDEX;
    options->function_index = DEFAULT_FUNCTION_INDEX;
    options->report_kind = REPORT_LONG;
    options->send_mode = SEND_WITH_REPORT_ID;
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
        } else if (strcmp(argv[i], "--skip-open") == 0) {
            options->skip_open = true;
        } else if (strcmp(argv[i], "--seize") == 0) {
            options->seize = true;
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
            options->has_pid = true;
        } else if (strcmp(argv[i], "--vid") == 0 && i + 1 < argc) {
            if (!parse_number(argv[++i], 0xFFFF, &options->vid)) {
                fprintf(stderr, "Invalid --vid value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--any-pid") == 0) {
            options->has_pid = false;
        } else if (strcmp(argv[i], "--usage-page") == 0 && i + 1 < argc) {
            if (!parse_number(argv[++i], 0xFFFF, &options->usage_page)) {
                fprintf(stderr, "Invalid --usage-page value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--usage") == 0 && i + 1 < argc) {
            if (!parse_number(argv[++i], 0xFFFF, &options->usage)) {
                fprintf(stderr, "Invalid --usage value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--device-index") == 0 && i + 1 < argc) {
            if (!parse_number(argv[++i], 0xFF, &options->device_index)) {
                fprintf(stderr, "Invalid --device-index value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--feature-index") == 0 && i + 1 < argc) {
            if (!parse_number(argv[++i], 0xFF, &options->feature_index)) {
                fprintf(stderr, "Invalid --feature-index value\n");
                return false;
            }
        } else if (strcmp(argv[i], "--function-index") == 0 && i + 1 < argc) {
            if (!parse_number(argv[++i], 0xFF, &options->function_index)) {
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
        } else if (strcmp(argv[i], "--send") == 0 && i + 1 < argc) {
            const char *mode = argv[++i];
            if (strcmp(mode, "without-report-id") == 0) {
                options->send_mode = SEND_WITHOUT_REPORT_ID;
            } else if (strcmp(mode, "with-report-id") == 0) {
                options->send_mode = SEND_WITH_REPORT_ID;
            } else {
                fprintf(stderr, "--send must be without-report-id or with-report-id\n");
                return false;
            }
        } else if (strcmp(argv[i], "--registry-path") == 0 && i + 1 < argc) {
            snprintf(options->registry_path, sizeof(options->registry_path), "%s", argv[++i]);
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
        fprintf(stderr, "Refusing to write. Add --execute only after reviewing dry-run output.\n");
        return false;
    }

    return true;
}

static int full_report_length(const Options *options)
{
    return options->report_kind == REPORT_LONG ? LONG_REPORT_LENGTH : SHORT_REPORT_LENGTH;
}

static uint8_t report_id(const Options *options)
{
    return (uint8_t)(options->report_kind == REPORT_LONG ? 0x11 : 0x10);
}

static void build_report(const Options *options, uint8_t report[LONG_REPORT_LENGTH])
{
    memset(report, 0, LONG_REPORT_LENGTH);
    report[0] = report_id(options);
    report[1] = (uint8_t)options->device_index;
    report[2] = (uint8_t)options->feature_index;
    report[3] = (uint8_t)options->function_index;
    report[4] = (uint8_t)(options->slot - 1);
}

static const uint8_t *iohid_report_bytes(const Options *options, const uint8_t report[LONG_REPORT_LENGTH])
{
    return options->send_mode == SEND_WITHOUT_REPORT_ID ? report + 1 : report;
}

static int iohid_report_length(const Options *options)
{
    int full_length = full_report_length(options);
    return options->send_mode == SEND_WITHOUT_REPORT_ID ? full_length - 1 : full_length;
}

static void print_report(const uint8_t *report, int length)
{
    for (int i = 0; i < length; i++) {
        printf("%s0x%02X", i == 0 ? "" : ",", report[i]);
    }
    puts("");
}

static void print_target(const Options *options)
{
    printf("Target VID: 0x%04X\n", options->vid);
    if (options->has_pid) {
        printf("Target PID: 0x%04X\n", options->pid);
    } else {
        puts("Target PID: wildcard");
    }
    printf("Target usagePage/usage: 0x%04X/0x%04X\n", options->usage_page, options->usage);
    if (options->registry_path[0]) {
        printf("Target registry path: %s\n", options->registry_path);
    }
}

static void print_payload(const Options *options)
{
    uint8_t report[LONG_REPORT_LENGTH];
    build_report(options, report);

    printf("Target slot: %d\n", options->slot);
    printf("Device index: 0x%02X\n", options->device_index);
    printf("Feature/function index: 0x%02X/0x%02X\n", options->feature_index, options->function_index);
    printf("Report kind: %s\n", options->report_kind == REPORT_LONG ? "long" : "short");
    printf("Full HID++ report length: %d\n", full_report_length(options));
    printf("Full HID++ report payload: ");
    print_report(report, full_report_length(options));
    printf("IOHID report ID: 0x%02X\n", report_id(options));
    printf("IOHID send mode: %s\n",
           options->send_mode == SEND_WITHOUT_REPORT_ID ? "without-report-id" : "with-report-id");
    printf("IOHID report bytes: ");
    print_report(iohid_report_bytes(options, report), iohid_report_length(options));
    printf("IOHID report byte length: %d\n", iohid_report_length(options));
}

static unsigned cf_number_property(IOHIDDeviceRef device, CFStringRef key)
{
    CFTypeRef value = IOHIDDeviceGetProperty(device, key);
    if (!value || CFGetTypeID(value) != CFNumberGetTypeID()) {
        return 0;
    }

    int number = 0;
    if (!CFNumberGetValue((CFNumberRef)value, kCFNumberIntType, &number) || number < 0) {
        return 0;
    }
    return (unsigned)number;
}

static void cf_string_property(IOHIDDeviceRef device, CFStringRef key, char *buffer, size_t buffer_size)
{
    buffer[0] = '\0';
    CFTypeRef value = IOHIDDeviceGetProperty(device, key);
    if (!value || CFGetTypeID(value) != CFStringGetTypeID()) {
        return;
    }

    if (!CFStringGetCString((CFStringRef)value, buffer, buffer_size, kCFStringEncodingUTF8)) {
        buffer[0] = '\0';
    }
}

static void registry_path_property(IOHIDDeviceRef device, char *buffer, size_t buffer_size)
{
    buffer[0] = '\0';

    io_service_t service = IOHIDDeviceGetService(device);
    if (service == MACH_PORT_NULL) {
        return;
    }

    io_string_t path;
    kern_return_t result = IORegistryEntryGetPath(service, kIOServicePlane, path);
    if (result != KERN_SUCCESS) {
        return;
    }

    snprintf(buffer, buffer_size, "%s", path);
}

static void read_hid_info(IOHIDDeviceRef device, HidInfo *info)
{
    memset(info, 0, sizeof(*info));
    info->vid = cf_number_property(device, CFSTR(kIOHIDVendorIDKey));
    info->pid = cf_number_property(device, CFSTR(kIOHIDProductIDKey));
    info->usage_page = cf_number_property(device, CFSTR(kIOHIDPrimaryUsagePageKey));
    info->usage = cf_number_property(device, CFSTR(kIOHIDPrimaryUsageKey));
    info->input_report_length = cf_number_property(device, CFSTR(kIOHIDMaxInputReportSizeKey));
    info->output_report_length = cf_number_property(device, CFSTR(kIOHIDMaxOutputReportSizeKey));
    info->feature_report_length = cf_number_property(device, CFSTR(kIOHIDMaxFeatureReportSizeKey));
    cf_string_property(device, CFSTR(kIOHIDManufacturerKey), info->manufacturer, sizeof(info->manufacturer));
    cf_string_property(device, CFSTR(kIOHIDProductKey), info->product, sizeof(info->product));
    cf_string_property(device, CFSTR(kIOHIDTransportKey), info->transport, sizeof(info->transport));
    cf_string_property(device, CFSTR(kIOHIDSerialNumberKey), info->serial, sizeof(info->serial));
    registry_path_property(device, info->registry_path, sizeof(info->registry_path));
}

static bool matches_options(const HidInfo *info, const Options *options)
{
    if (options->registry_path[0]) {
        return strcmp(info->registry_path, options->registry_path) == 0;
    }
    if (info->vid != options->vid) {
        return false;
    }
    if (options->has_pid && info->pid != options->pid) {
        return false;
    }
    return info->usage_page == options->usage_page && info->usage == options->usage;
}

static void print_hid_info(const HidInfo *info)
{
    printf("%04X:%04X usagePage=0x%04X usage=0x%04X\n",
           info->vid,
           info->pid,
           info->usage_page,
           info->usage);
    printf("  report lengths: input=%u output=%u feature=%u\n",
           info->input_report_length,
           info->output_report_length,
           info->feature_report_length);
    printf("  manufacturer: %s\n", info->manufacturer[0] ? info->manufacturer : "(unknown)");
    printf("  product:      %s\n", info->product[0] ? info->product : "(unknown)");
    printf("  transport:    %s\n", info->transport[0] ? info->transport : "(unknown)");
    printf("  serial:       %s\n", info->serial[0] ? info->serial : "(unknown)");
    printf("  registry:     %s\n\n", info->registry_path[0] ? info->registry_path : "(unknown)");
}

static void print_ioreturn_error(const char *operation, IOReturn result)
{
    fprintf(stderr, "%s failed: 0x%08X\n", operation, result);
    if (result == IORETURN_NOT_PERMITTED) {
        fprintf(stderr,
                "macOS denied HID access. Grant Input Monitoring permission to the app running this command, then rerun it.\n");
    }
}

static int enumerate_hids(const Options *options, Selection *selection)
{
    memset(selection, 0, sizeof(*selection));

    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) {
        fprintf(stderr, "IOHIDManagerCreate failed\n");
        return 1;
    }

    IOHIDManagerSetDeviceMatching(manager, NULL);

    CFSetRef devices = IOHIDManagerCopyDevices(manager);
    if (!devices) {
        CFRelease(manager);
        return 0;
    }

    CFIndex device_count = CFSetGetCount(devices);
    IOHIDDeviceRef *device_list = (IOHIDDeviceRef *)calloc((size_t)device_count, sizeof(*device_list));
    if (!device_list) {
        fprintf(stderr, "Out of memory\n");
        CFRelease(devices);
        CFRelease(manager);
        return 1;
    }

    CFSetGetValues(devices, (const void **)device_list);

    for (CFIndex i = 0; i < device_count; i++) {
        HidInfo info;
        read_hid_info(device_list[i], &info);
        bool is_match = matches_options(&info, options);
        if (options->all || is_match) {
            print_hid_info(&info);
        }
        if (!options->all && is_match) {
            selection->match_count++;
            if (selection->match_count == 1) {
                selection->device = device_list[i];
                CFRetain(selection->device);
                selection->info = info;
            }
        }
    }

    free(device_list);
    CFRelease(devices);
    CFRelease(manager);
    return 0;
}

static int dry_run(const Options *options)
{
    print_target(options);
    print_payload(options);
    puts("No HID report was sent.");
    return 0;
}

static int switch_slot(const Options *options)
{
    uint8_t report[LONG_REPORT_LENGTH];
    build_report(options, report);

    Selection selection;
    int enum_result = enumerate_hids(options, &selection);
    if (enum_result != 0) {
        return enum_result;
    }

    if (selection.match_count == 0) {
        fprintf(stderr, "No matching HID interface found.\n");
        return 1;
    }
    if (selection.match_count > 1) {
        fprintf(stderr, "More than one matching HID interface found. Re-run with --registry-path for the exact target.\n");
        if (selection.device) {
            CFRelease(selection.device);
        }
        return 1;
    }

    puts("Selected HID interface:");
    print_hid_info(&selection.info);
    print_payload(options);

    if (!options->skip_open) {
        IOOptionBits open_options = options->seize ? kIOHIDOptionsTypeSeizeDevice : kIOHIDOptionsTypeNone;
        if (options->seize) {
            puts("Opening with kIOHIDOptionsTypeSeizeDevice.");
        }
        IOReturn open_result = IOHIDDeviceOpen(selection.device, open_options);
        if (open_result != kIOReturnSuccess) {
            print_ioreturn_error("IOHIDDeviceOpen", open_result);
            CFRelease(selection.device);
            return 1;
        }
    } else {
        puts("Skipping explicit IOHIDDeviceOpen before IOHIDDeviceSetReport.");
    }

    printf("Writing IOHID output report to selected device.\n");
    IOReturn write_result = IOHIDDeviceSetReport(selection.device,
                                                kIOHIDReportTypeOutput,
                                                report_id(options),
                                                iohid_report_bytes(options, report),
                                                (CFIndex)iohid_report_length(options));
    if (write_result != kIOReturnSuccess) {
        print_ioreturn_error("IOHIDDeviceSetReport", write_result);
        if (!options->skip_open) {
            IOHIDDeviceClose(selection.device, kIOHIDOptionsTypeNone);
        }
        CFRelease(selection.device);
        return 1;
    }

    puts("Wrote IOHID output report.");
    if (!options->skip_open) {
        IOHIDDeviceClose(selection.device, kIOHIDOptionsTypeNone);
    }
    CFRelease(selection.device);
    return 0;
}

int main(int argc, char **argv)
{
    Options options;
    if (!parse_args(argc, argv, &options)) {
        return 2;
    }

    if (options.command == CMD_LIST) {
        Selection selection;
        int result = enumerate_hids(&options, &selection);
        if (!options.all) {
            printf("Matching interfaces: %d\n", selection.match_count);
        }
        if (selection.device) {
            CFRelease(selection.device);
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
