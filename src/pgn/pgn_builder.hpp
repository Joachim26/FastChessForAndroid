#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <chess.hpp>

#include <matchmaking/match/match.hpp>

#include <types/tournament_options.hpp>

namespace fast_chess {

class PgnBuilder {
   public:
    PgnBuilder(const MatchData &match, const options::Tournament &tournament_options,
               std::size_t round_id);

    /// @brief Get the newly created pgn
    /// @return
    [[nodiscard]] std::string get() const noexcept { return pgn_.str() + "\n\n"; }

    static constexpr int LINE_LENGTH = 80;

   private:
    /// @brief Converts a UCI move to either SAN, LAN or keeps it as UCI
    /// @param board
    /// @param move
    /// @return
    [[nodiscard]] std::string moveNotation(chess::Board &board,
                                           const std::string &move) const noexcept;

    /// @brief Adds a header to the pgn
    /// @tparam T
    /// @param name
    /// @param value
    template <typename T>
    void addHeader(std::string_view name, const T &value) noexcept;

    std::string addMove(chess::Board &board, const MoveData &move, std::size_t move_number,
                        bool illegal) noexcept;

    /// @brief Adds a comment to the pgn. The comment is formatted as {first, args}
    /// @tparam First
    /// @tparam ...Args
    /// @param first
    /// @param ...args
    /// @return
    template <typename First, typename... Args>
    [[nodiscard]] static std::string addComment(First &&first, Args &&...args) {
        std::stringstream ss;

        ss << " {" << std::forward<First>(first);
        ((ss << (std::string(args).empty() ? "" : ", ") << std::forward<Args>(args)), ...);
        ss << "}";

        return ss.str();
    }

    /// @brief Formats a time in milliseconds to seconds with 3 decimals
    /// @param millis
    /// @return
    [[nodiscard]] static std::string formatTime(uint64_t millis) {
        std::stringstream ss;
        ss << std::setprecision(3) << std::fixed << millis / 1000.0 << "s";
        return ss.str();
    }

    [[nodiscard]] static std::string getResultFromMatch(const MatchData &match) noexcept;

    [[nodiscard]] static std::string convertMatchTermination(const MatchTermination &res) noexcept;

    MatchData match_;
    options::Tournament game_options_;

    std::stringstream pgn_;
};

}  // namespace fast_chess