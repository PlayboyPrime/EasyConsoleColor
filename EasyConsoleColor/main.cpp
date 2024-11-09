#include <string>

#include "EasyConsoleColor.h"

int main()
{
	ECC::EasyConsoleColor ecc = { {
		.thread_delay_ms = 200,
		.create_console = false,

		.suppressor = "\\",
		.prefix = "~",
		.suffix = "~",
		.seperator = ",",
		.group_begin = "(",
		.group_end = ")",

		.FG_BLUE = "B",
		.FG_GREEN = "G",
		.FG_RED = "R",
		.FG_INTENSITY = "I",
		.BG_BLUE = "BB",
		.BG_GREEN = "BG",
		.BG_RED = "BR",
		.BG_INTENSITY = "BI",
		.RESET = "RST"
	} };

	ecc.log("~R~Red ~G~(Green Green ~B~Not Blue) Red ~B~Blue Blue ~R,G~Red and Green");

	ecc.log<false>("~B,I~Type something in console: ~G,I~");
	std::string input;
	std::getline(ecc.get_input(), input);
	std::string escaped_input = ecc.escape_str(input);

	ecc.log("~G,I~Your Input: ~B,I~{}", escaped_input);
	ecc.flush_queue();
	ecc.get_output() << "Escaped: " << escaped_input;
}