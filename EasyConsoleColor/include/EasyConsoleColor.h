#pragma once
#ifndef _WIN32
static_assert(false, "This header is only compatible with Windows");
#endif

// Remove already included headers for better build time or to fix issues
#include <Windows.h>
#include <cassert>
#include <format>
#include <fstream>
#include <mutex>
#include <vector>

namespace EasyConsoleColor
{
	struct Attributes
	{
		std::string_view suppressor = "\\";
		std::string_view prefix = "~";
		std::string_view suffix = "~";
		std::string_view seperator = ",";
		// TODO: Add grouping EX: ~R~(TEXT) = ~R~TEXT~RST~ to get rid of reset

		std::string_view FG_BLUE = "B";
		std::string_view FG_GREEN = "G";
		std::string_view FG_RED = "R";
		std::string_view FG_INTENSITY = "I";
		std::string_view BG_BLUE = "BB";
		std::string_view BG_GREEN = "BG";
		std::string_view BG_RED = "BR";
		std::string_view BG_INTENSITY = "BI";
		std::string_view RESET = "RST";
	};

	struct Config
	{
		DWORD thread_delay_ms = 200;
		bool create_console = true;
		Attributes attributes;
	};

	class EasyConsoleColor
	{
		const Config m_config;
		HANDLE m_event;
		HANDLE m_thread;
		std::mutex m_mutex;
		std::ofstream* m_file = nullptr;
		std::ofstream* m_out = nullptr;
		std::ifstream* m_in = nullptr;

	public:
		EasyConsoleColor(std::ofstream* file, std::ofstream* out, std::ifstream* in, const Config& config) : m_config(config)
		{
			if (file) m_file = file;
			if (out) m_out = out;
			if (in) m_in = in;

			m_event = CreateEvent(nullptr, true, false, nullptr); assert(m_event != nullptr && "Failed to create event");
			m_thread = CreateThread(nullptr, 0, thread, this, 0, nullptr); assert(m_thread != nullptr && "Failed to create thread");

			if (m_config.create_console)
			{
				if (!AttachConsole(ATTACH_PARENT_PROCESS))
					assert(AllocConsole() && "Failed to create console");
			}
		}

		~EasyConsoleColor()
		{
			SetEvent(m_event);
			WaitForSingleObject(m_thread, INFINITE);

			if (m_config.create_console)
				FreeConsole();
		}

		EasyConsoleColor(const EasyConsoleColor&) = delete;
		EasyConsoleColor& operator=(const EasyConsoleColor&) = delete;
		EasyConsoleColor(EasyConsoleColor&&) = delete;
		EasyConsoleColor& operator=(EasyConsoleColor&&) = delete;


		static DWORD thread(LPVOID lpThreadParameter)
		{
			EasyConsoleColor* console = (EasyConsoleColor*)lpThreadParameter;

			while (WaitForSingleObject(console->m_event, console->m_config.thread_delay_ms) == WAIT_TIMEOUT)
				console->flush();

			console->log("~RST~");
			console->flush();
			ExitThread(0);
		}

		void flush()
		{
			std::lock_guard lock(m_mutex);
			if (m_file) m_file->flush();
			if (m_out) m_out->flush();
		}

		template <typename T, typename... Args>
		void log(T format, Args... args)
		{
			if (!m_out && !m_file)
				throw std::runtime_error("No output destination specified");

			std::string text = std::vformat(format, std::make_format_args(args...));
			const char* textptr = text.data();

			std::string to_log;
			for (size_t i = 0; i < text.size(); ++i)
			{
				if (begins_with(textptr + i, m_config.attributes.suppressor))
				{
					size_t suppressor_size = m_config.attributes.suppressor.size();

					// double backslash
					if (begins_with(textptr + i + suppressor_size, m_config.attributes.suppressor))
					{
						i += suppressor_size * 2 - 1;
						to_log += m_config.attributes.suppressor;
						continue;
					}

					// suppressed prefix
					if (begins_with(textptr + i + suppressor_size, m_config.attributes.prefix))
					{
						i += suppressor_size + m_config.attributes.prefix.size() - 1;
						to_log += m_config.attributes.prefix;
						continue;
					}
				}

				// prefix
				if (begins_with(textptr + i, m_config.attributes.prefix))
				{
					i += m_config.attributes.prefix.size();
					size_t j = i;

					// find suffix (not checking for suppressor since everything in between is attributes)
					for (; j < text.size(); ++j)
					{
						if (begins_with(textptr + j, m_config.attributes.suffix))
							break;
					}

					std::string attributes = text.substr(i, j - i);
					std::lock_guard lock(m_mutex);
					if (m_out) *m_out << to_log;
					if (m_file) *m_file << to_log;
					apply_attributes(attributes);

					to_log = "";
					i = j + m_config.attributes.suffix.size() - 1;
					continue;
				}

				to_log += text[i];
			}

			std::lock_guard lock(m_mutex);
			if (m_out) *m_out << to_log << '\n';
			if (m_file) *m_file << to_log << '\n';
		}

		template <typename T>
		std::ofstream& operator<<(T& arg)
		{
			log(arg);
			if (m_out)
				return *m_out;

			if (m_file)
				return *m_file;

			throw;
		}

		template <typename T>
		std::ifstream& operator>>(T& rhs)
		{
			if (m_in)
				throw std::runtime_error("No input destination specified");

			std::lock_guard lock(m_mutex);
			return *m_in >> rhs;
		}

		void lock()
		{
			m_mutex.lock();
		}

		void unlock()
		{
			m_mutex.unlock();
		}

#ifdef EASY_CONSOLE_COLOR_EXTRAS
#define C1(a) std::string(m_config.attributes.prefix) + std::string(m_config.attributes.a) + std::string(m_config.attributes.suffix)
#define C2(a, b) std::string(m_config.attributes.prefix) + std::string(m_config.attributes.a) + std::string(m_config.attributes.seperator) + std::string(m_config.attributes.b) + std::string(m_config.attributes.suffix)
#define CE std::string(m_config.attributes.prefix) + std::string(m_config.attributes.RESET) + std::string(m_config.attributes.suffix)
		std::string red(const std::string& str) { return C1(FG_RED) + str + CE; }
		std::string green(const std::string& str) { return C1(FG_GREEN) + str + CE; }
		std::string blue(const std::string& str) { return C1(FG_BLUE) + str + CE; }
		std::string yellow(const std::string& str) { return C2(FG_RED, FG_GREEN) + str + CE; }
		std::string cyan(const std::string& str) { return C2(FG_BLUE, FG_GREEN) + str + CE; }
		std::string magenta(const std::string& str) { return C2(FG_RED, FG_BLUE) + str + CE; }
		std::string intensity(const std::string& str) { return C1(FG_INTENSITY) + str + CE; }

		std::string red_bg(const std::string& str) { return C1(BG_RED) + str + CE; }
		std::string green_bg(const std::string& str) { return C1(BG_GREEN) + str + CE; }
		std::string blue_bg(const std::string& str) { return C1(BG_BLUE) + str + CE; }
		std::string yellow_bg(const std::string& str) { return C2(BG_RED, BG_GREEN) + str + CE; }
		std::string cyan_bg(const std::string& str) { return C2(BG_BLUE, BG_GREEN) + str + CE; }
		std::string magenta_bg(const std::string& str) { return C2(BG_RED, BG_BLUE) + str + CE; }
		std::string intensity_bg(const std::string& str) { return C1(BG_INTENSITY) + str + CE; }
#undef C1
#undef C2
#undef CE
#endif

	private:
		static bool begins_with(const std::string_view& str1, const std::string_view& str2)
		{
			if (str2.size() > str1.size())
				return false;

			for (size_t i = 0; i < str2.size(); ++i)
			{
				if (str1.at(i) != str2.at(i))
					return false;
			}

			return true;
		}

		std::vector<std::string> split(std::string str)
		{
			std::vector<std::string> res;

			size_t pos;
			while ((pos = str.find(m_config.attributes.seperator)) != std::string::npos)
			{
				std::string token = str.substr(0, pos);
				res.emplace_back(token);
				str.erase(0, pos + m_config.attributes.seperator.size());
			}
			res.emplace_back(str);

			return res;
		}

		void apply_attributes(const std::string& attributes_str)
		{
			static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
			std::vector<std::string> attributes = !m_config.attributes.seperator.empty() ? split(attributes_str) : std::vector{ attributes_str };
			WORD flags = 0;

			for (auto& attribute : attributes)
			{
				if (attribute == m_config.attributes.FG_BLUE)
					flags |= FOREGROUND_BLUE;
				else if (attribute == m_config.attributes.FG_GREEN)
					flags |= FOREGROUND_GREEN;
				else if (attribute == m_config.attributes.FG_RED)
					flags |= FOREGROUND_RED;
				else if (attribute == m_config.attributes.FG_INTENSITY)
					flags |= FOREGROUND_INTENSITY;
				else if (attribute == m_config.attributes.BG_BLUE)
					flags |= BACKGROUND_BLUE;
				else if (attribute == m_config.attributes.BG_GREEN)
					flags |= BACKGROUND_GREEN;
				else if (attribute == m_config.attributes.BG_RED)
					flags |= BACKGROUND_RED;
				else if (attribute == m_config.attributes.BG_INTENSITY)
					flags |= BACKGROUND_INTENSITY;
				else if (attribute == m_config.attributes.RESET)
					flags = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
				else
					assert(false && "Invalid attribute");
			}

			SetConsoleTextAttribute(hConsole, flags);
		}
	};
}
