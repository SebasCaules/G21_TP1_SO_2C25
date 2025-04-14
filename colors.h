#ifndef COLORS_H
#define COLORS_H

static const char *bodyColors[] = {
    "\033[31m",      // Red
    "\033[32m",      // Green
    "\033[33m",      // Yellow
    "\033[34m",      // Blue
    "\033[35m",      // Magenta
    "\033[36m",      // Cyan
    "\033[1;31m",    // Bold Red (distinct from normal red)
    "\033[38;5;213m", // Rose (approximated using 256-color palette)
    "\033[38;5;214m" // Orange (approximated using 256-color palette)
};

//master doesn't use it
__attribute__((unused)) static const char *headColors[] = {
    "\033[97;41m",   // White text on red background
    "\033[97;42m",   // White text on green background
    "\033[97;43m",   // White text on yellow background
    "\033[97;44m",   // White text on blue background
    "\033[97;45m",   // White text on magenta background
    "\033[97;46m",   // White text on cyan background
    "\033[97;101m",  // White text on bold red background
    "\033[97;48;5;213m", // White text on rose background
    "\033[97;48;5;214m" // White text on orange background
};

// Reset color
static const char *reset = "\033[0m";

#endif // COLORS_H