#pragma once

#include "AnnoInfo.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace MaxsuPoise
{
	class AnnotationCache
	{
	public:
		static constexpr std::size_t maxEntries = 512;
		static constexpr std::size_t maxTextBytes = 1024 * 1024;

		template <class Parse>
		std::optional<AnnoInfo> Get(std::string_view a_text, Parse&& a_parse)
		{
			if (a_text.size() > maxTextBytes) {
				return std::nullopt;
			}
			if (const auto it = _entries.find(a_text); it != _entries.end()) {
				return it->second;
			}
			auto result = a_parse(a_text);
			if (_entries.size() >= maxEntries || _textBytes + a_text.size() > maxTextBytes) {
				_entries.clear();
				_textBytes = 0;
			}
			_entries.emplace(a_text, result);
			_textBytes += a_text.size();
			return result;
		}

		std::size_t Size() const { return _entries.size(); }
		std::size_t TextBytes() const { return _textBytes; }

	private:
		struct TextHash
		{
			using is_transparent = void;
			std::size_t operator()(std::string_view a_text) const
			{
				return std::hash<std::string_view>{}(a_text);
			}
		};

		std::unordered_map<std::string, std::optional<AnnoInfo>, TextHash, std::equal_to<>> _entries;
		std::size_t _textBytes = 0;
	};
}
