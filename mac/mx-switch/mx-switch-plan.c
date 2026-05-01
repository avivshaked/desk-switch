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

typedef enum ReportKind {
    REPORT_SHORT,
    REPORT_LONG
} ReportKind;

typedef enum SendMode {
    SEND_WITHOUT_REPORT_ID,
    SEND_WITH_REPORT_ID
} SendMode;

/*
 * This planner intentionally has no IOKit includes and links no HID
 * frameworks. It is safe for payload review because it cannot enumerate,
 * open, or write to devices.
 */
typedef struct Options {
    unsigned vid;
    unsigned pid;
    unsigned usage_page;
    unsigned usage;
    unsigned device_index;
    unsigned feature_index;
    unsigned function_index;
    bool has_pid;
    ReportKind report_kind;
    SendMode send_mode;
    int slot;
} Options;

static void print_usage(void)
{
    puts("mx-switch-plan: dry-run planner for macOS Logitech MX Master 3S Easy-Switch reports");
    puts("");
    puts("Usage:");
    puts("  mx-switch-plan dry-run --slot <1|2|3> [options]");
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
    puts("");
    puts("This program does not import IOKit and cannot send HID reports.");
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

    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage();
        return false;
    }

    if (strcmp(argv[1], "dry-run") != 0) {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        print_usage();
        return false;
    }

    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--slot") == 0 && i + 1 < argc) {
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
        } else {
            fprintf(stderr, "Unknown or incomplete option: %s\n", argv[i]);
            return false;
        }
    }

    if (options->slot == 0) {
        fprintf(stderr, "dry-run requires --slot <1|2|3>\n");
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

static void print_report(const uint8_t *report, int length)
{
    for (int i = 0; i < length; i++) {
        printf("%s0x%02X", i == 0 ? "" : ",", report[i]);
    }
    puts("");
}

int main(int argc, char **argv)
{
    Options options;
    uint8_t report[LONG_REPORT_LENGTH];

    if (!parse_args(argc, argv, &options)) {
        return 2;
    }

    build_report(&options, report);

    printf("Target VID: 0x%04X\n", options.vid);
    if (options.has_pid) {
        printf("Target PID: 0x%04X\n", options.pid);
    } else {
        puts("Target PID: wildcard");
    }
    printf("Target usagePage/usage: 0x%04X/0x%04X\n", options.usage_page, options.usage);
    printf("Target slot: %d\n", options.slot);
    printf("Device index: 0x%02X\n", options.device_index);
    printf("Feature/function index: 0x%02X/0x%02X\n", options.feature_index, options.function_index);
    printf("Report kind: %s\n", options.report_kind == REPORT_LONG ? "long" : "short");
    printf("Full HID++ report length: %d\n", full_report_length(&options));
    printf("Full HID++ report payload: ");
    print_report(report, full_report_length(&options));
    printf("IOHID report ID: 0x%02X\n", report_id(&options));

    if (options.send_mode == SEND_WITHOUT_REPORT_ID) {
        printf("IOHID send mode: without-report-id\n");
        printf("IOHID report bytes: ");
        print_report(report + 1, full_report_length(&options) - 1);
        printf("IOHID report byte length: %d\n", full_report_length(&options) - 1);
    } else {
        printf("IOHID send mode: with-report-id\n");
        printf("IOHID report bytes: ");
        print_report(report, full_report_length(&options));
        printf("IOHID report byte length: %d\n", full_report_length(&options));
    }

    puts("No IOKit APIs are linked. No HID report can be sent by this executable.");
    return 0;
}
