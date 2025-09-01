#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#define CSV_FILE "tracker.csv"
#define LINE_MAX_LEN 1024

static void today_iso(char out[11]) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    strftime(out, 11, "%Y-%m-%d", tm); // YYYY-MM-DD
}

static int file_has_header(FILE *f) {
    // Peek first line for "date,category,description,amount"
    char buf[LINE_MAX_LEN];
    long pos = ftell(f);
    rewind(f);
    if (fgets(buf, sizeof buf, f) == NULL) { fseek(f, pos, SEEK_SET); return 0; }
    // trim newline
    buf[strcspn(buf, "\r\n")] = 0;
    int ok = (strcmp(buf, "date,category,description,amount") == 0);
    fseek(f, pos, SEEK_SET);
    return ok;
}

static void ensure_header(void) {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) {
        // create with header
        f = fopen(CSV_FILE, "w");
        if (!f) { perror("opening tracker.csv"); exit(1); }
        fprintf(f, "date,category,description,amount\n");
        fclose(f);
        return;
    }
    int has = file_has_header(f);
    fclose(f);
    if (!has) {
        // Prepend header by rewriting (simple approach for fresh file)
        FILE *rf = fopen(CSV_FILE, "r");
        FILE *wf = fopen("tracker.tmp.csv", "w");
        if (!rf || !wf) { perror("header rewrite"); if (rf) fclose(rf); if (wf) fclose(wf); return; }
        fprintf(wf, "date,category,description,amount\n");
        char c;
        while ((c = (char)fgetc(rf)) != EOF) fputc(c, wf);
        fclose(rf); fclose(wf);
        remove(CSV_FILE);
        rename("tracker.tmp.csv", CSV_FILE);
    }
}

// Replace commas in-place with spaces so CSV stays valid
static void sanitize_commas(char *s) {
    for (; *s; ++s) if (*s == ',') *s = ' ';
}

// Join argv[start..end) with spaces into buf
static void join_args(char *buf, size_t bufsz, int argc, char *argv[], int start) {
    buf[0] = '\0';
    for (int i = start; i < argc; ++i) {
        strncat(buf, argv[i], bufsz - strlen(buf) - 1);
        if (i + 1 < argc) strncat(buf, " ", bufsz - strlen(buf) - 1);
    }
}

static int add_entry_cli(double amount, const char *category, const char *desc) {
    ensure_header();
    FILE *f = fopen(CSV_FILE, "a");
    if (!f) { perror("append tracker.csv"); return 1; }
    char date[11]; today_iso(date);
    // Copy + sanitize category/desc
    char cat_s[128]; snprintf(cat_s, sizeof cat_s, "%s", category ? category : "uncat");
    char desc_s[768]; snprintf(desc_s, sizeof desc_s, "%s", desc ? desc : "");
    sanitize_commas(cat_s);
    sanitize_commas(desc_s);
    fprintf(f, "%s,%s,%s,%.2f\n", date, cat_s, desc_s, amount);
    fclose(f);
    printf("Entry added ✅  (%s | %s | %s | %.2f)\n", date, cat_s, desc_s, amount);
    return 0;
}

static int list_entries(void) {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) { printf("No entries yet.\n"); return 0; }
    char line[LINE_MAX_LEN];
    // skip header if present
    if (fgets(line, sizeof line, f) == NULL) { fclose(f); return 0; }
    if (strncmp(line, "date,category,description,amount", 32) != 0) {
        // first line was data; print it
        // re-print it nicely
        // fallthrough: we already have it in `line`
        // but easier: rewind and treat like no header
        rewind(f);
    }
    double total = 0.0;
    printf("📒 Entries\n");
    printf("date        | category        | amount  | description\n");
    printf("------------+-----------------+---------+-------------------------\n");
    while (fgets(line, sizeof line, f)) {
        // trim newline
        line[strcspn(line, "\r\n")] = 0;
        // parse CSV (simple split on commas; we sanitized commas in fields)
        char *date = strtok(line, ",");
        char *cat  = strtok(NULL, ",");
        char *desc = strtok(NULL, ",");
        char *amtS = strtok(NULL, ",");
        if (!date || !cat || !desc || !amtS) continue;
        double amt = strtod(amtS, NULL);
        total += amt;
        printf("%-12s %-17s %7.2f  %s\n", date, cat, amt, desc);
    }
    printf("------------+-----------------+---------+-------------------------\n");
    printf("Total: %.2f\n", total);
    fclose(f);
    return 0;
}

static void interactive_shell(void) {
    printf("Tracker interactive mode. Commands:\n");
    printf("  add <amount> <category> <description>\n");
    printf("  list\n");
    printf("  quit\n");
    ensure_header();

    char buf[LINE_MAX_LEN];
    for (;;) {
        printf("> ");
        if (!fgets(buf, sizeof buf, stdin)) break;
        // trim newline
        buf[strcspn(buf, "\r\n")] = 0;
        if (buf[0] == '\0') continue;

        // Tokenize
        char *cmd = strtok(buf, " ");
        if (!cmd) continue;

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            break;
        } else if (strcmp(cmd, "list") == 0) {
            list_entries();
        } else if (strcmp(cmd, "add") == 0) {
            char *amountS = strtok(NULL, " ");
            char *category = strtok(NULL, " ");
            char *rest = strtok(NULL, ""); // remainder as description (may contain spaces)
            if (!amountS || !category || !rest) {
                printf("Usage: add <amount> <category> <description>\n");
                continue;
            }
            double amt = strtod(amountS, NULL);
            if (amt == 0 && (amountS[0] != '0' && amountS[1] != '\0')) {
                printf("Invalid amount.\n");
                continue;
            }
            add_entry_cli(amt, category, rest);
        } else {
            printf("Unknown command. Try: add | list | quit\n");
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        // No args → interactive mode
        interactive_shell();
        return 0;
    }

    // CLI subcommands
    if (strcmp(argv[1], "add") == 0) {
        if (argc < 5) {
            printf("Usage: %s add <amount> <category> <description>\n", argv[0]);
            return 1;
        }
        double amt = strtod(argv[2], NULL);
        if (amt == 0 && (argv[2][0] != '0' && argv[2][1] != '\0')) {
            printf("Invalid amount: %s\n", argv[2]);
            return 1;
        }
        char desc[800];
        join_args(desc, sizeof desc, argc, argv, 4);
        return add_entry_cli(amt, argv[3], desc);
    } else if (strcmp(argv[1], "list") == 0) {
        return list_entries();
    } else {
        printf("Unknown command: %s\n", argv[1]);
        printf("Usage: %s add <amount> <category> <description> | list\n", argv[0]);
        return 1;
    }
}
