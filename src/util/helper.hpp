#pragma once

#include <algorithm>
#include <array>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <json.hpp>

namespace fast_chess {
/*
Modified version of the NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE macro in nlohmann's json lib.
ordered_json type conversion is not yet supported, though we only have to change the type.
*/
#define NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ORDERED_JSON(Type, ...)                                \
    inline void to_json(nlohmann::ordered_json &nlohmann_json_j, const Type &nlohmann_json_t) {   \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_TO, __VA_ARGS__))                  \
    }                                                                                             \
    inline void from_json(const nlohmann::ordered_json &nlohmann_json_j, Type &nlohmann_json_t) { \
        NLOHMANN_JSON_EXPAND(NLOHMANN_JSON_PASTE(NLOHMANN_JSON_FROM, __VA_ARGS__))                \
    }
}  // namespace fast_chess

namespace fast_chess::str_utils {

/// @brief @todo unnecessary with c++20
/// @param haystack
/// @param needle
/// @return
[[nodiscard]] inline bool startsWith(std::string_view haystack, std::string_view needle) noexcept {
    if (needle.empty()) return false;
    return (haystack.rfind(needle, 0) != std::string::npos);
}

/// @brief @todo unnecessary with c++20
/// @param value
/// @param ending
/// @return
[[nodiscard]] inline bool endsWith(std::string_view value, std::string_view ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/// @brief Check if a string contains a substring. @todo unnecessary with c++20
/// @param haystack
/// @param needle
/// @return
[[nodiscard]] inline bool contains(std::string_view haystack, std::string_view needle) noexcept {
    return haystack.find(needle) != std::string::npos;
}

/// @brief Check if a vector of strings contains a string.
/// @param haystack
/// @param needle
/// @return
[[nodiscard]] inline bool contains(const std::vector<std::string> &haystack,
                                   std::string_view needle) noexcept {
    return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

/// @brief Split a string into a vector of strings based on a delimiter.
/// @param string
/// @param delimiter
/// @return
[[nodiscard]] inline std::vector<std::string> splitString(const std::string &string,
                                                          const char &delimiter) {
    std::stringstream string_stream(string);
    std::string segment;
    std::vector<std::string> seglist;

    while (std::getline(string_stream, segment, delimiter)) seglist.emplace_back(segment);

    return seglist;
}

/// @brief Find an element in a vector of strings and return the next element as a specified type.
/// @tparam T
/// @param haystack
/// @param needle
/// @return
template <typename T>
[[nodiscard]] std::optional<T> findElement(const std::vector<std::string> &haystack,
                                           std::string_view needle) {
    auto position = std::find(haystack.begin(), haystack.end(), needle);
    auto index    = position - haystack.begin();
    if (position == haystack.end()) return std::nullopt;
    if constexpr (std::is_same_v<T, int>)
        return std::stoi(haystack[index + 1]);
    else if constexpr (std::is_same_v<T, float>)
        return std::stof(haystack[index + 1]);
    else if constexpr (std::is_same_v<T, uint64_t>)
        return std::stoull(haystack[index + 1]);
    else
        return haystack[index + 1];
}

}  // namespace fast_chess::str_utils
