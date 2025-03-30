#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>

#define MAX_VALUE 0xFFFF   // 16-bit max value
#define MID_RANGE 0x7FFF
#define PI 3.14159265358979323846

// Global flag to control program execution
volatile int run_flag = 1;
// Mutex for synchronizing parameter updates
pthread_mutex_t param_mutex;


// Enumeration for waveform types
typedef enum {
    WAVE_SINE,
    WAVE_SQUARE,
    WAVE_TRIANGULAR,
    WAVE_SAWTOOTH,
    WAVE_ARBITRARY  // Placeholder for arbitrary waveform loaded from disk
} WaveType;

// Structure to hold waveform parameters
typedef struct {
    WaveType type;
    double frequency; // in Hz
    double amplitude; // normalized (0 to 1)
    int steps;        // number of points per cycle
    // For an arbitrary waveform, you might add an array of values and its length.
} WaveParams;

WaveParams current_params;

// Terminal I/O settings for non-canonical mode (keyboard thread)
struct termios orig_termios;

// Restore original terminal mode
void reset_terminal_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

// Set terminal to raw mode for non-blocking input
void set_conio_terminal_mode(void) {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);
}

// Check if a key has been hit
int kbhit() {
    int byteswaiting;
    ioctl(STDIN_FILENO, FIONREAD, &byteswaiting);
    return byteswaiting;
}

// Read one character from standard input
int getch() {
    int r;
    unsigned char c;
    r = read(STDIN_FILENO, &c, sizeof(c));
    return (r < 0) ? -1 : c;
}

// Parse waveform type string into WaveType enum
WaveType parse_waveform(char *waveform) {
    if (strcmp(waveform, "sine") == 0) {
        return WAVE_SINE;
    } else if (strcmp(waveform, "square") == 0) {
        return WAVE_SQUARE;
    } else if (strcmp(waveform, "triangular") == 0) {
        return WAVE_TRIANGULAR;
    } else if (strcmp(waveform, "sawtooth") == 0) {
        return WAVE_SAWTOOTH;
    } else if (strcmp(waveform, "arbitrary") == 0) {
        return WAVE_ARBITRARY;
    } else {
        return WAVE_SINE;  // Default waveform
    }
}

// Load configuration from file (if exists)
void load_config(const char *filename, WaveParams *params) {
    FILE *fp = fopen(filename, "r");
    if (fp) {
        char type[32];
        if (fscanf(fp, "%31s %lf %lf %d", type, &(params->frequency), &(params->amplitude), &(params->steps)) == 4) {
            params->type = parse_waveform(type);
        }
        fclose(fp);
    }
}

// Save current configuration to file
void save_config(const char *filename, WaveParams *params) {
    FILE *fp = fopen(filename, "w");
    if (fp) {
        const char *type_str;
        switch (params->type) {
            case WAVE_SINE: type_str = "sine"; break;
            case WAVE_SQUARE: type_str = "square"; break;
            case WAVE_TRIANGULAR: type_str = "triangular"; break;
            case WAVE_SAWTOOTH: type_str = "sawtooth"; break;
            case WAVE_ARBITRARY: type_str = "arbitrary"; break;
            default: type_str = "sine"; break;
        }
        fprintf(fp, "%s %lf %lf %d\n", type_str, params->frequency, params->amplitude, params->steps);
        fclose(fp);
    }
}

// Signal handler for SIGINT to allow orderly shutdown
void sigint_handler(int signum) {
    (void)signum; // Unused parameter
    run_flag = 0;
}

// Waveform generation thread function
void *waveform_thread(void *arg) {
    (void)arg;  // Not used; using global current_params
    while (run_flag) {
        // Safely copy current parameters
        pthread_mutex_lock(&param_mutex);
        WaveParams params = current_params;
        pthread_mutex_unlock(&param_mutex);

        // Calculate period and delay per point
        double period = 1.0 / params.frequency;
        double delay = period / params.steps;
        struct timespec ts;
        ts.tv_sec = (time_t) delay;
        ts.tv_nsec = (long)((delay - ts.tv_sec) * 1e9);

        // Generate one complete cycle
        for (int i = 0; i < params.steps && run_flag; i++) {
            double value = 0.0;
            switch (params.type) {
                case WAVE_SINE: {
                    double angle = 2 * PI * i / params.steps;
                    // Sine returns values in [-1, 1]; adjust to [0,1] then scale
                    value = ((sin(angle) + 1.0) / 2.0) * params.amplitude;
                    break;
                }
                case WAVE_SQUARE: {
                    // Square: low for first half, high for second half
                    value = (i < params.steps / 2) ? 0.0 : params.amplitude;
                    break;
                }
                case WAVE_TRIANGULAR: {
                    if (i < params.steps / 2) {
                        value = ((double)i / (params.steps / 2)) * params.amplitude;
                    } else {
                        value = ((double)(params.steps - i) / (params.steps / 2)) * params.amplitude;
                    }
                    break;
                }
                case WAVE_SAWTOOTH: {
                    // Linear ramp from 0 to maximum over the cycle
                    value = ((double)i / params.steps) * params.amplitude;
                    break;
                }
                case WAVE_ARBITRARY: {
                    // Placeholder: For an arbitrary waveform, load data from disk.
                    // Here, we simulate with a sine wave.
                    double angle = 2 * PI * i / params.steps;
                    value = ((sin(angle) + 1.0) / 2.0) * params.amplitude;
                    break;
                }
                default:
                    value = 0.0;
            }
            // Scale value to 16-bit range [0, 0xffff]
            unsigned int output = (unsigned int)(value * MAX_VALUE);
            // In a real system, output would be written to the D/A port.
            // For simulation, print the value.
            printf("Output: 0x%04X\n", output);
            fflush(stdout);
            nanosleep(&ts, NULL);
        }
    }
    pthread_exit(NULL);
}

// Keyboard input thread function for real-time adjustments
void *keyboard_thread(void *arg) {
    (void)arg;
    set_conio_terminal_mode();
    printf("Keyboard control active.\n");
    printf("Use arrow keys to adjust parameters:\n");
    printf("   Up/Down: Increase/Decrease Frequency\n");
    printf("   Right/Left: Increase/Decrease Amplitude\n");
    printf("Press 'q' to quit.\n");

    while (run_flag) {
        if (kbhit()) {
            int ch = getch();
            if (ch == 27) { // Start of an escape sequence (arrow keys)
                if (kbhit() && getch() == '[') {
                    int arrow = getch();
                    pthread_mutex_lock(&param_mutex);
                    if (arrow == 'A') {  // Up arrow
                        current_params.frequency += 0.1;
                        printf("Frequency increased to %.2f Hz\n", current_params.frequency);
                    } else if (arrow == 'B') {  // Down arrow
                        if (current_params.frequency > 0.1)
                            current_params.frequency -= 0.1;
                        printf("Frequency decreased to %.2f Hz\n", current_params.frequency);
                    } else if (arrow == 'C') {  // Right arrow
                        if (current_params.amplitude < 1.0) {
                            current_params.amplitude += 0.05;
                            if (current_params.amplitude > 1.0)
                                current_params.amplitude = 1.0;
                        }
                        printf("Amplitude increased to %.2f\n", current_params.amplitude);
                    } else if (arrow == 'D') {  // Left arrow
                        if (current_params.amplitude > 0.05)
                            current_params.amplitude -= 0.05;
                        else
                            current_params.amplitude = 0.0;
                        printf("Amplitude decreased to %.2f\n", current_params.amplitude);
                    }
                    pthread_mutex_unlock(&param_mutex);
                }
            } else if (ch == 'q' || ch == 'Q') {
                run_flag = 0;
            }
        }
        usleep(10000);  // Sleep for 10ms
    }
    reset_terminal_mode();
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    // Set SIGINT handler for graceful shutdown (e.g., on Ctrl+C)
    signal(SIGINT, sigint_handler);

    // Initialize default parameters
    current_params.type = WAVE_SINE;
    current_params.frequency = 1.0;  // Hz
    current_params.amplitude = 1.0;    // Full amplitude
    current_params.steps = 100;        // 100 points per cycle

    // Initialize mutex
    pthread_mutex_init(&param_mutex, NULL);

    // Load settings from configuration file (if available)
    load_config("waveform.cfg", &current_params);

    // Override with command line arguments if provided:
    // Usage: program [waveform] [frequency] [amplitude] [steps]
    if (argc > 1) {
        current_params.type = parse_waveform(argv[1]);
    }
    if (argc > 2) {
        double freq = atof(argv[2]);
        if (freq > 0)
            current_params.frequency = freq;
        else
            printf("Invalid frequency value. Using default.\n");
    }
    if (argc > 3) {
        double amp = atof(argv[3]);
        if (amp >= 0 && amp <= 1.0)
            current_params.amplitude = amp;
        else
            printf("Invalid amplitude value. Using default.\n");
    }
    if (argc > 4) {
        int s = atoi(argv[4]);
        if (s > 0)
            current_params.steps = s;
        else
            printf("Invalid steps value. Using default.\n");
    }

    // if (getchar()) {     terminate main loop if char is input in terminal
    //     free();
    // }

    // Display the initial settings
    const char *wave_str;
    switch (current_params.type) {
        case WAVE_SINE: wave_str = "Sine"; break;
        case WAVE_SQUARE: wave_str = "Square"; break;
        case WAVE_TRIANGULAR: wave_str = "Triangular"; break;
        case WAVE_SAWTOOTH: wave_str = "Sawtooth"; break;
        case WAVE_ARBITRARY: wave_str = "Arbitrary"; break;
        default: wave_str = "Sine"; break;
    }
    printf("Waveform Generator Starting with Settings:\n");
    printf("Waveform: %s\n", wave_str);
    printf("Frequency: %.2f Hz\n", current_params.frequency);
    printf("Amplitude: %.2f\n", current_params.amplitude);
    printf("Steps per cycle: %d\n", current_params.steps);

    // Create threads for waveform generation and keyboard input
    pthread_t wave_tid, kb_tid;
    if (pthread_create(&wave_tid, NULL, waveform_thread, NULL)) {
        fprintf(stderr, "Error creating waveform thread.\n");
        return 1;
    }
    if (pthread_create(&kb_tid, NULL, keyboard_thread, NULL)) {
        fprintf(stderr, "Error creating keyboard thread.\n");
        return 1;
    }

    // Wait for both threads to finish
    pthread_join(wave_tid, NULL);
    pthread_join(kb_tid, NULL);

    // Save current configuration for next run
    save_config("waveform.cfg", &current_params);

    // Clean up mutex
    pthread_mutex_destroy(&param_mutex);

    printf("Waveform generator terminated gracefully.\n");
    return 0;
}