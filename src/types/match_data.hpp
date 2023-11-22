#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <types/player_info.hpp>
#include <util/logger.hpp>

namespace fast_chess {

struct MoveData {
    MoveData(std::string _move, std::string _score_string, int64_t _elapsed_millis, int _depth,
             int _seldepth, int _score, int _nodes)
        : move(std::move(_move)),
          score_string(std::move(_score_string)),
          elapsed_millis(_elapsed_millis),
          nodes(_nodes),
          seldepth(_seldepth),
          depth(_depth),
          score(_score) {}

    std::string move;
    std::string score_string;
    int64_t elapsed_millis = 0;
    uint64_t nodes         = 0;
    int seldepth           = 0;
    int depth              = 0;
    int score              = 0;
    int nps                = 0;
};

enum class MatchTermination {
    ADJUDICATION,
    TIMEOUT,
    DISCONNECT,
    ILLEGAL_MOVE,
    INTERRUPT,
    None,
};

struct MatchData {
    MatchData() = default;

    explicit MatchData(std::string fen) : fen(std::move(fen)) {
        start_time = Logger::getDateTime("%Y-%m-%dT%H:%M:%S %z");
        date       = Logger::getDateTime("%Y-%m-%d");
    }

    std::pair<PlayerInfo, PlayerInfo> players;

    std::string start_time;
    std::string end_time;
    std::string duration;
    std::string date;
    std::string fen;

    // This reason will be printed on the console and will be written to the
    // pgn file as a comment at the end of the game.
    std::string reason;

    std::vector<MoveData> moves;

    // This is the reason why the game ended.
    // It will be used for the PGN Header.
    MatchTermination termination = MatchTermination::None;

    bool needs_restart = false;
};

}  // namespace fast_chess