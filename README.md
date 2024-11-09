## A header for C++ to easily log text to console with color.

### Key Features
- Multithreaded
- Customizable
- Easy to use

### Example
```c++
#include <string>

#include "EasyConsoleColor.h"

int main()
{
	ECC::EasyConsoleColor* ecc = new ECC::EasyConsoleColor{ {
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

	ecc->log("~R~Red ~G~(Green Green ~B~Not Blue) Red ~B~Blue Blue ~R,G~Red and Green");

	ecc->log<false>("~B,I~Type something in console: ~G,I~");
	std::string input;
	std::getline(ecc->get_input(), input);
	std::string escaped_input = ecc->escape_str(input);

	ecc->log("~G,I~Your Input: ~B,I~{}", escaped_input);
	ecc->flush_queue();
	ecc->get_output() << "Escaped: " << escaped_input;
	delete ecc;
}
```

### Result
![image](https://github.com/user-attachments/assets/5e09148d-9494-4d23-823a-2b4e05551ba0)

