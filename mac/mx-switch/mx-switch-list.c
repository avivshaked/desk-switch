#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/hid/IOHIDKeys.h>
#include <IOKit/hid/IOHIDManager.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_VID 0x046D
#define DEFAULT_PID 0xB034
#define DEFAULT_USAGE_PAGE 0x0001
#define DEFAULT_USAGE 0x0002

typedef struct Options {
    unsigned vid;
    unsigned pid;
    unsigned usage_page;
    unsigned usage;
    bool has_pid;
    bool all;
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

static void print_usage(void)
{
    puts("mx-switch-list: read-only macOS HID inventory for Logitech MX Master 3S experiments");
    puts("");
    puts("Usage:");
    puts("  mx-switch-list [--all] [options]");
    puts("");
    puts("Options:");
    puts("  --all                    Print every HID device, not only target matches");
    puts("  --vidpid <VID:PID>       Match an exact vendor/product pair");
    puts("  --vid <number>           Default: 0x046D");
    puts("  --any-pid                Do not require the default product ID");
    puts("  --usage-page <number>    Default: 0x0001");
    puts("  --usage <number>         Default: 0x0002");
    puts("");
    puts("This program only reads IOHID metadata and has no write path.");
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
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }

    return true;
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

int main(int argc, char **argv)
{
    Options options;
    if (!parse_args(argc, argv, &options)) {
        return 2;
    }

    IOHIDManagerRef manager = IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDOptionsTypeNone);
    if (!manager) {
        fprintf(stderr, "IOHIDManagerCreate failed\n");
        return 1;
    }

    IOHIDManagerSetDeviceMatching(manager, NULL);

    CFSetRef devices = IOHIDManagerCopyDevices(manager);
    if (!devices) {
        puts("Matching interfaces: 0");
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

    int match_count = 0;
    for (CFIndex i = 0; i < device_count; i++) {
        HidInfo info;
        read_hid_info(device_list[i], &info);
        bool is_match = matches_options(&info, &options);
        if (is_match) {
            match_count++;
        }
        if (options.all || is_match) {
            print_hid_info(&info);
        }
    }

    if (!options.all) {
        printf("Matching interfaces: %d\n", match_count);
    }

    free(device_list);
    CFRelease(devices);
    CFRelease(manager);
    return 0;
}
