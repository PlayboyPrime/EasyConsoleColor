#define EASY_CONSOLE_COLOR_EXTRAS
#include "EasyConsoleColor.h"

int main()
{
	EasyConsoleColor::EasyConsoleColor c = { nullptr, (std::ofstream*)&std::cout, nullptr, {
		.thread_delay_ms = 50,
		.create_console = false,
		.attributes = {
			.suppressor = "\\",
			.prefix = "~",
			.suffix = "~",
			.seperator = ",",
			.FG_BLUE = "B",
			.FG_GREEN = "G",
			.FG_RED = "R",
			.FG_INTENSITY = "I",
			.BG_BLUE = "BB",
			.BG_GREEN = "BG",
			.BG_RED = "BR",
			.BG_INTENSITY = "BI",
			.RESET = "RST"
		}
	} };

    auto start = std::chrono::high_resolution_clock::now();
	c.log(c.red("RED") + " OKOK " + c.green("GREEN") + " HMM " + c.yellow("YELLOW ") + c.cyan_bg("CYAN BG") + "~R~R ~B~B ~G~G ~R,G~R,G ~R,B~R,B ~BR~BR ~BR,G~BR,G");
    auto finish = std::chrono::high_resolution_clock::now();
	c.log("~RST~TIME ~G,I~{}~RST~", std::chrono::duration_cast<std::chrono::microseconds>(finish-start).count());
}