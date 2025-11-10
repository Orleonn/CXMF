#pragma once

#include <iostream>



namespace cmd
{

using std::cout;
using std::cin;
using std::cerr;
using std::clog;

using std::endl;
using std::ends;
using std::flush;



/*
	GENERIC
*/

// Clear screen and reset cursor
inline std::ostream& clear(std::ostream& s)
{
	return s.write("\033[2J\033[3J\033[H", 11);
}

// Clear from cursor position to end of screen
inline std::ostream& clear_after(std::ostream& s)
{
	return s.write("\033[0J", 4);
}

// Clear from start of screen to the cursor position
inline std::ostream& clear_before(std::ostream& s)
{
	return s.write("\033[1J", 4);
}

// Clear line and return cursor
inline std::ostream& clearl(std::ostream& s)
{
	return s.write("\033[2K\r", 5);
}

// Clear from cursor position to end of line
inline std::ostream& clearl_after(std::ostream& s)
{
	return s.write("\033[0K", 4);
}

// Clear from start of line to the cursor position
inline std::ostream& clearl_before(std::ostream& s)
{
	return s.write("\033[1K", 4);
}

// Save cursor position
inline std::ostream& save_cpos(std::ostream& s)
{
	return s.write("\033[s", 3);
}

// Restore the last saved cursor position
inline std::ostream& restore_cpos(std::ostream& s)
{
	return s.write("\033[u", 3);
}

// Reset all modes (styles and colors)
inline std::ostream& reset(std::ostream& s)
{
	return s.write("\033[0m", 4);
}

// Show cursor
inline std::ostream& show_cursor(std::ostream& s)
{
	return s.write("\033[?25h", 6);
}

// Hide cursor
inline std::ostream& hide_cursor(std::ostream& s)
{
	return s.write("\033[?25l", 6);
}



/*
	TEXT COLORS
*/

// White text
inline std::ostream& clr_white(std::ostream& s)
{
	return s.write("\033[38;5;255m", 11);
}

// Gray text
inline std::ostream& clr_gray(std::ostream& s)
{
	return s.write("\033[38;5;7m", 9);
}

// Black text
inline std::ostream& clr_black(std::ostream& s)
{
	return s.write("\033[38;5;0m", 9);
}

// Red text
inline std::ostream& clr_red(std::ostream& s)
{
	return s.write("\033[38;5;196m", 11);
}

// Green text
inline std::ostream& clr_green(std::ostream& s)
{
	return s.write("\033[38;5;46m", 10);
}

// Blue text
inline std::ostream& clr_blue(std::ostream& s)
{
	return s.write("\033[38;5;33m", 10);
}

// Cyan text
inline std::ostream& clr_cyan(std::ostream& s)
{
	return s.write("\033[38;5;51m", 10);
}

// Purple text
inline std::ostream& clr_purple(std::ostream& s)
{
	return s.write("\033[38;5;141m", 11);
}

// Yellow text
inline std::ostream& clr_yellow(std::ostream& s)
{
	return s.write("\033[38;5;227m", 11);
}

// Orange text
inline std::ostream& clr_orange(std::ostream& s)
{
	return s.write("\033[38;5;214m", 11);
}

// Pink text
inline std::ostream& clr_pink(std::ostream& s)
{
	return s.write("\033[38;5;219m", 11);
}

// Custom text color (True Color)
struct text_color
{
	uint8_t r, g, b;

	text_color(int red, int green, int blue)
		: r(static_cast<uint8_t>(red)), g(static_cast<uint8_t>(green)), b(static_cast<uint8_t>(blue))
	{}

	std::ostream& operator()(std::ostream& s) const
	{
		return s << "\033[38;2;" << int(r) << ';' << int(g) << ';' << int(b) << 'm';
	}
};

inline std::ostream& operator<<(std::ostream& s, const text_color& c)
{
	return c(s);
}



/*
	BACKGROUND COLORS
*/

// White background
inline std::ostream& bkg_clr_white(std::ostream& s)
{
	return s.write("\033[48;5;255m", 11);
}

// Gray background
inline std::ostream& bkg_clr_gray(std::ostream& s)
{
	return s.write("\033[48;5;7m", 9);
}

// Black background
inline std::ostream& bkg_clr_black(std::ostream& s)
{
	return s.write("\033[48;5;0m", 9);
}

// Red background
inline std::ostream& bkg_clr_red(std::ostream& s)
{
	return s.write("\033[48;5;196m", 11);
}

// Green background
inline std::ostream& bkg_clr_green(std::ostream& s)
{
	return s.write("\033[48;5;46m", 10);
}

// Blue background
inline std::ostream& bkg_clr_blue(std::ostream& s)
{
	return s.write("\033[48;5;33m", 10);
}

// Cyan background
inline std::ostream& bkg_clr_cyan(std::ostream& s)
{
	return s.write("\033[48;5;51m", 10);
}

// Purple background
inline std::ostream& bkg_clr_purple(std::ostream& s)
{
	return s.write("\033[48;5;141m", 11);
}

// Yellow background
inline std::ostream& bkg_clr_yellow(std::ostream& s)
{
	return s.write("\033[48;5;227m", 11);
}

// Orange background
inline std::ostream& bkg_clr_orange(std::ostream& s)
{
	return s.write("\033[48;5;214m", 11);
}

// Pink background
inline std::ostream& bkg_clr_pink(std::ostream& s)
{
	return s.write("\033[48;5;219m", 11);
}

// Custom background color (True Color)
struct background_color
{
	uint8_t r, g, b;

	background_color(int red, int green, int blue)
		: r(static_cast<uint8_t>(red)), g(static_cast<uint8_t>(green)), b(static_cast<uint8_t>(blue))
	{}

	std::ostream& operator()(std::ostream& s) const
	{
		return s << "\033[48;2;" << int(r) << ';' << int(g) << ';' << int(b) << 'm';
	}
};

inline std::ostream& operator<<(std::ostream& s, const background_color& c)
{
	return c(s);
}



/*
	MODES
*/

// Enable bold mode
inline std::ostream& bold_begin(std::ostream& s)
{
	return s.write("\033[1m", 4);
}

// Disable bold mode
inline std::ostream& bold_end(std::ostream& s)
{
	return s.write("\033[22m", 5);
}

// Enable dim/faint mode
inline std::ostream& faint_begin(std::ostream& s)
{
	return s.write("\033[2m", 4);
}

// Disable dim/faint mode
inline std::ostream& faint_end(std::ostream& s)
{
	return s.write("\033[22m", 5);
}

// Enable italic mode
inline std::ostream& italic_begin(std::ostream& s)
{
	return s.write("\033[3m", 4);
}

// Disable italic mode
inline std::ostream& italic_end(std::ostream& s)
{
	return s.write("\033[23m", 5);
}

// Enable underline mode
inline std::ostream& underline_begin(std::ostream& s)
{
	return s.write("\033[4m", 4);
}

// Disable underline mode
inline std::ostream& underline_end(std::ostream& s)
{
	return s.write("\033[24m", 5);
}

// Enable blinking mode
inline std::ostream& blink_begin(std::ostream& s)
{
	return s.write("\033[5m", 4);
}

// Disable blinking mode
inline std::ostream& blink_end(std::ostream& s)
{
	return s.write("\033[25m", 5);
}

// Enable inverse/reverse mode
inline std::ostream& inverse_begin(std::ostream& s)
{
	return s.write("\033[7m", 4);
}

// Disable inverse/reverse mode
inline std::ostream& inverse_end(std::ostream& s)
{
	return s.write("\033[27m", 5);
}

// Enable hidden/invisible mode
inline std::ostream& hidden_begin(std::ostream& s)
{
	return s.write("\033[8m", 4);
}

// Disable hidden/invisible mode
inline std::ostream& hidden_end(std::ostream& s)
{
	return s.write("\033[28m", 5);
}

// Enable strikethrough mode
inline std::ostream& strikethrough_begin(std::ostream& s)
{
	return s.write("\033[9m", 4);
}

// Disable strikethrough mode
inline std::ostream& strikethrough_end(std::ostream& s)
{
	return s.write("\033[29m", 5);
}

}  //namespace cmd
