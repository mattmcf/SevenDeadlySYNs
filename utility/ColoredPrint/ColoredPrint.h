#ifndef COLOREDPRINT_H
#define COLOREDPRINT_H

typedef enum
{
	COLOR_RESET = 0,
	COLOR_BOLD,
	COLOR_DARK,
	COLOR_UNDERLINE,
	COLOR_FLASH,
	COLOR_INVERT,
	COLOR_D_BLACK,
	COLOR_D_RED,
	COLOR_D_GREEN,
	COLOR_D_YELLOW,
	COLOR_D_BLUE,
	COLOR_D_MAGENTA,
	COLOR_D_CYAN,
	COLOR_D_WHITE,
	COLOR_BG_D_BLACK,
	COLOR_BG_D_RED,
	COLOR_BG_D_GREEN,
	COLOR_BG_D_YELLOW,
	COLOR_BG_D_BLUE,
	COLOR_BG_D_MAGENTA,
	COLOR_BG_D_CYAN,
	COLOR_BG_D_WHITE,
	COLOR_L_BLACK,
	COLOR_L_RED,
	COLOR_L_GREEN,
	COLOR_L_YELLOW,
	COLOR_L_BLUE,
	COLOR_L_MAGENTA,
	COLOR_L_CYAN,
	COLOR_L_WHITE,
	COLOR_BG_L_BLACK,
	COLOR_BG_L_RED,
	COLOR_BG_L_GREEN,
	COLOR_BG_L_YELLOW,
	COLOR_BG_L_BLUE,
	COLOR_BG_L_MAGENTA,
	COLOR_BG_L_CYAN,
	COLOR_BG_L_WHITE
} FORMAT_ARG;

// Registers a format. The format should be a 0 terminated array of FORMAT_ARG
//	FORMAT_ARG : (not claimed) The array of format specifiers
//	ret		   : (static) The id that when passed into format_printf will specify a format
int register_format(FORMAT_ARG formats[]);

// Print a string with a format
//	formatID : (static) The id of the format to be printed
//	format   : (not claimed) Just like printf
//  ...      : (not claimed) Just like printf
void format_printf(int formatID, char* format, ...);

#endif
