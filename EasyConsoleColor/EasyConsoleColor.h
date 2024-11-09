#pragma once
#ifndef _WIN32
static_assert(false, "This header is only compatible with Windows");
#endif

#include <Windows.h>

#include <cassert>
#include <format>
#include <fstream>
#include <mutex>
#include <optional>
#include <queue>
#include <vector>

namespace ECC
{
	constexpr WORD RESET = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;

	struct Config
	{
		DWORD thread_delay_ms = 200;
		bool create_console = true;

		std::string_view suppressor = "\\";
		std::string_view prefix = "~";
		std::string_view suffix = "~";
		std::string_view seperator = ",";
		std::string_view group_begin = "(";
		std::string_view group_end = ")";

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

	struct ParsedGroup
	{
		std::string text;
		WORD prev_attributes;
	};

	class EasyConsoleColor
	{
		const Config m_config;
		std::ofstream m_output;
		std::ifstream m_input;
		HANDLE m_event;
		HANDLE m_thread;
		HANDLE m_console;
		std::mutex m_queue_mutex;
		std::queue<std::string> m_log_queue;
		std::mutex m_log_mutex;

	public:
		EasyConsoleColor(const Config& config) : m_config(config)
		{
			m_output.open("CONOUT$");
			m_input.open("CONIN$");

			m_event = CreateEvent(nullptr, true, false, nullptr);			assert(m_event != nullptr && "Failed to create event");
			m_thread = CreateThread(nullptr, 0, thread, this, 0, nullptr);	assert(m_thread != nullptr && "Failed to create thread");

			if (m_config.create_console)
			{
				if (!AttachConsole(ATTACH_PARENT_PROCESS))
					assert(AllocConsole() && "Failed to create console");
			}

			m_console = GetStdHandle(STD_OUTPUT_HANDLE);
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
			EasyConsoleColor* ecc = (EasyConsoleColor*)lpThreadParameter;

			while (WaitForSingleObject(ecc->m_event, ecc->m_config.thread_delay_ms) == WAIT_TIMEOUT)
				ecc->flush_queue();

			ecc->flush_queue();
			ecc->set_attributes(RESET);
			ecc->flush();
			ExitThread(0);
		}

		void flush_queue()
		{
			std::lock_guard queue_lock(m_queue_mutex);
			while (!m_log_queue.empty())
			{
				std::string str = m_log_queue.front(); m_log_queue.pop();
				const char* strptr = str.data();

				std::string to_log;
				to_log.reserve(str.size());
				for (size_t i = 0; i < str.size(); ++i)
				{
					// Avoid logging suppressors
					if (begins_with({ strptr + i, str.size() - i }, m_config.suppressor))
					{
						size_t suppressor_size = m_config.suppressor.size();
						if (begins_with({ strptr + i + suppressor_size, str.size() - i - suppressor_size }, m_config.suppressor))
						{
							to_log += m_config.suppressor;
							i += suppressor_size;
						}

						continue;
					}

					// Search for unsuppressed prefix (This check is useless here because its already in parse_attributes)
					if (begins_with({ strptr + i, str.size() - i }, m_config.prefix) && !is_suppressed(str, i))
					{
						std::optional<ParsedGroup> parsed_group;
						WORD parsed_attributes;
						if (parse_attributes(str, &i, &parsed_attributes, &parsed_group))
						{
							std::lock_guard lock(m_log_mutex);
							if (!to_log.empty())
							{
								m_output << to_log;
								flush();

								to_log.clear();
							}

							set_attributes(parsed_attributes);

							if (parsed_group.has_value())
							{
								m_output << parsed_group->text;
								flush();

								set_attributes(parsed_group->prev_attributes);
							}

							continue;
						}
					}

					to_log += strptr[i];
				}

				if (!to_log.empty())
				{
					std::lock_guard lock(m_log_mutex);
					m_output << to_log;
				}
				flush();

				// set_attributes(RESET); // This can be used so after the logged text the color will reset to default
			}
		}

		void flush()
		{
			m_output.flush();
		}

		// Could just remove insert_new_line and always manually add it
		template <bool insert_new_line = true, typename T, typename... Args>
		void log(T format, Args... args)
		{
			std::lock_guard lock(m_queue_mutex);
			std::string text = std::vformat(format, std::make_format_args(args...));
			if constexpr (insert_new_line)
				text += '\n';

			m_log_queue.push(text);
		}

		std::ifstream& get_input()
		{
			return m_input;
		}

		std::ofstream& get_output()
		{
			return m_output;
		}

		std::mutex& get_lock()
		{
			return m_log_mutex;
		}

		void set_attributes(WORD attributes)
		{
			SetConsoleTextAttribute(m_console, attributes);
		}

#define CHECK(attr) begins_with(sv, m_config.attr)
		std::string escape_str(const std::string& str)
		{
			const char* strptr = str.data();

			std::string res;
			res.reserve(str.size() + str.size() / 2);
			for (size_t i = 0; i < str.size(); ++i)
			{
				std::string_view sv = { strptr + i, str.size() - i };
				if (CHECK(suppressor) || CHECK(prefix) || CHECK(suffix) || CHECK(seperator) || CHECK(group_begin) || CHECK(group_end))
					res += m_config.suppressor;

				res += str[i];
			}

			return res;
		}
#undef CHECK

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
			while ((pos = str.find(m_config.seperator)) != std::string::npos)
			{
				std::string token = str.substr(0, pos);
				res.emplace_back(token);
				str.erase(0, pos + m_config.seperator.size());
			}
			res.emplace_back(str);

			return res;
		}

		bool is_suppressed(const std::string& str, size_t pos)
		{
			size_t suppressor_size = m_config.suppressor.size();

			bool is_suppressed = false;
			while (pos >= suppressor_size && begins_with({ str.data() + pos - suppressor_size, str.size() - pos + suppressor_size }, m_config.suppressor))
			{
				is_suppressed = !is_suppressed;
				pos -= suppressor_size;
			}

			return is_suppressed;
		}

		WORD attributes_str_to_num(const std::string& attributes)
		{
			std::vector<std::string> attributes_list = split(attributes);
			WORD flags = 0;

			for (const std::string& attribute : attributes_list)
			{
				if (attribute == m_config.FG_BLUE)
					flags |= FOREGROUND_BLUE;
				else if (attribute == m_config.FG_GREEN)
					flags |= FOREGROUND_GREEN;
				else if (attribute == m_config.FG_RED)
					flags |= FOREGROUND_RED;
				else if (attribute == m_config.FG_INTENSITY)
					flags |= FOREGROUND_INTENSITY;
				else if (attribute == m_config.BG_BLUE)
					flags |= BACKGROUND_BLUE;
				else if (attribute == m_config.BG_GREEN)
					flags |= BACKGROUND_GREEN;
				else if (attribute == m_config.BG_RED)
					flags |= BACKGROUND_RED;
				else if (attribute == m_config.BG_INTENSITY)
					flags |= BACKGROUND_INTENSITY;
				else if (attribute == m_config.RESET)
					flags |= FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
				else
					assert(false && "Invalid attribute");
			}

			return flags;
		}

		// Parses attributes and modifies position
		// out_group is only set when a group was found
		bool parse_attributes(const std::string& str, size_t* pos, WORD* out_attributes, std::optional<ParsedGroup>* out_group)
		{
			const char* strptr = str.data();

			// Check if the current position is not a prefix
			if (!begins_with({ strptr + *pos, str.size() - *pos }, m_config.prefix) || is_suppressed(str, *pos))
				return false;

			// Search for suffix
			size_t suffix_start = *pos + 1;
			while (++*pos < str.size())
			{
				// Check if current position is a suffix
				if (begins_with({ strptr + *pos, str.size() - *pos }, m_config.suffix))
					break;
			}

			// Check if the suffix was not found
			if (*pos == str.size())
			{
				*pos = suffix_start - 1;
				return false;
			}

			*out_attributes = attributes_str_to_num(str.substr(suffix_start, *pos - suffix_start));

			// Check if next position is group begin
			if (begins_with({ strptr + *pos + 1, str.size() - *pos - 1 }, m_config.group_begin) && !is_suppressed(str, *pos + 1))
			{
				++*pos;
				size_t group_start = *pos + 1;
				while (++*pos < str.size())
				{
					// Check if current position is a group end
					if (begins_with({ strptr + *pos, str.size() - *pos }, m_config.group_end) && !is_suppressed(str, *pos))
						break;
				}

				// Check if the group end was not found
				if (*pos == str.size())
				{
					*pos = group_start - 2;
					*out_group = std::nullopt;
				}
				else
				{
					WORD prev_attributes = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
					if (CONSOLE_SCREEN_BUFFER_INFO csbi; GetConsoleScreenBufferInfo(m_console, &csbi))
						prev_attributes = csbi.wAttributes;

					*out_group = { str.substr(group_start, *pos - group_start), prev_attributes };
				}
			}

			return true;
		}
	};
}
