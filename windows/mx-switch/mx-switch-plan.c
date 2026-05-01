#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
 * This planner intentionally has no Windows HID includes and links no HID
 * libraries. It is safe for payload review because it cannot enumerate,
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
    ReportKind report_kind;
    int slot;
} Options;

static void print_usage(void)
{
    puts("mx-switch-plan: dry-run planner for Logitech MX Master 3S Easy-Switch reports");
    puts("");
    puts("Usage:");
    puts("  mx-switch-plan.exe dry-run --slot <1|2|3> [options]");
    puts("");
    puts("Options:");
    puts("  --vidpid <VID:PID>          Default: 046D:C548");
    puts("  --usage-page <number>       Default: 0xFF00");
    puts("  --usage <number>            Default: 0x0001");
    puts("  --device-index <number>     Default: 0x02");
    puts("  --feature-index <number>    Default: 0x0A");
    puts("  --function-index <number>   Default: 0x1B");
    puts("  --report <short|long>       Default: short");
    puts("");
    puts("This program does not import Windows HID APIs and cannot send HID reports.");
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
    if (sscanf_s(text, "%15[^:/]:%15s", left, (unsigned)sizeof(left), right, (unsigned)sizeof(right)) != 2 &&
        sscanf_s(text, "%15[^:/]/%15s", left, (unsigned)sizeof(left), right, (unsigned)sizeof(right)) != 2) {
        return false;
    }

    return parse_number(left, 0xFFFF, vid) && parse_number(right, 0xFFFF, pid);
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

static int report_length(const Options *options)
{
    return options->report_kind == REPORT_LONG ? LONG_REPORT_LENGTH : SHORT_REPORT_LENGTH;
}

static void build_report(const Options *options, uint8_t report[LONG_REPORT_LENGTH])
{
    memset(report, 0, LONG_REPORT_LENGTH);

    /* Candidate short HID++ report used by community Bolt Easy-Switch examples. */
    report[0] = options->report_kind == REPORT_LONG ? 0x11 : 0x10;
    report[1] = (uint8_t)options->device_index;
    report[2] = (uint8_t)options->feature_index;
    report[3] = (uint8_t)options->function_index;

    /* Community examples map channel bytes as slot 1 = 0, slot 2 = 1, slot 3 = 2. */
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

    printf("Target VID/PID: 0x%04X:0x%04X\n", options.vid, options.pid);
    printf("Target usagePage/usage: 0x%04X/0x%04X\n", options.usage_page, options.usage);
    printf("Target slot: %d\n", options.slot);
    printf("Device index: 0x%02X\n", options.device_index);
    printf("Feature/function index: 0x%02X/0x%02X\n", options.feature_index, options.function_index);
    printf("Report kind: %s\n", options.report_kind == REPORT_LONG ? "long" : "short");
    printf("Output report length: %d\n", report_length(&options));
    printf("Output report payload: ");
    print_report(report, report_length(&options));
    puts("No HID APIs are linked. No HID report can be sent by this executable.");

    return 0;
}
