## A header for C++ to easily log text to console with color.
#### [Key Features](https://github.com/PlayboyPrime/EasyConsoleColor?tab=readme-ov-file#key-features-1)
#### [How To Use](https://github.com/PlayboyPrime/EasyConsoleColor?tab=readme-ov-file#how-to-use-1)
#### [Example](https://github.com/PlayboyPrime/EasyConsoleColor?tab=readme-ov-file#example-1)

### Key Features
- Multithreaded
- Customizable
- Easy to use

### How To Use
- Simply add the `EasyConsoleColor.h` file into your project and include it
- Create a new instance of `ECC::EasyConsoleColor` like in the example code. You can also use `std::unique_ptr` so you dont have to delete it afterwards
- Call the `log` function of the new instance with a text you want to log
- Delete the instance if finished

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

