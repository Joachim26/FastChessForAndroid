#include <matchmaking/match.hpp>

#include <helper.hpp>
#include <util/logger.hpp>

#include <types/tournament_options.hpp>

namespace fast_chess {

namespace atomic {
extern std::atomic_bool stop;
}  // namespace atomic

using namespace std::literals;

namespace chrono = std::chrono;
using clock      = chrono::high_resolution_clock;

using namespace chess;

void Match::addMoveData(Participant& player, int64_t measured_time) {
    const auto best_move = player.engine->bestmove();

    MoveData move_data = MoveData(best_move, "0.00", measured_time, 0, 0, 0, 0);

    if (player.engine->output().size() <= 1) {
        data_.moves.push_back(move_data);
        played_moves_.push_back(best_move);
        return;
    }

    // extract last info line
    const auto score_type = player.engine->lastScoreType();
    const auto info       = player.engine->lastInfo();

    move_data.nps      = str_utils::findElement<int>(info, "nps").value_or(0);
    move_data.depth    = str_utils::findElement<int>(info, "depth").value_or(0);
    move_data.seldepth = str_utils::findElement<int>(info, "seldepth").value_or(0);
    move_data.nodes    = str_utils::findElement<uint64_t>(info, "nodes").value_or(0);
    move_data.score    = player.engine->lastScore();

    // Missing elements default to 0
    std::stringstream ss;
    if (score_type == "cp") {
        ss << (move_data.score >= 0 ? '+' : '-');
        ss << std::fixed << std::setprecision(2) << (float(std::abs(move_data.score)) / 100);
    } else if (score_type == "mate") {
        ss << (move_data.score > 0 ? "+M" : "-M") << std::to_string(std::abs(move_data.score));
    } else {
        ss << "ERR";
    }

    move_data.score_string = ss.str();

    verifyPvLines(player);

    data_.moves.push_back(move_data);
    played_moves_.push_back(best_move);
}

void Match::start(UciEngine& engine1, UciEngine& engine2) {
    Participant* player_1 = new Participant(&engine1);
    Participant* player_2 = new Participant(&engine2);

    player_1->engine->startEngine();
    player_2->engine->startEngine();

    board_.set960(tournament_options_.variant == VariantType::FRC);

    if (!player_1->engine->sendUciNewGame()) {
        throw std::runtime_error(player_1->engine->getConfig().name + " failed to start.");
    }

    if (!player_2->engine->sendUciNewGame()) {
        throw std::runtime_error(player_2->engine->getConfig().name + " failed to start.");
    }

    board_.setFen(opening_.fen);

    start_position_ = board_.getFen() == constants::STARTPOS ? "startpos" : board_.getFen();

    std::vector<std::string> uci_moves = [&]() {
        std::vector<std::string> moves;

        for (const auto& move : opening_.moves) {
            moves.push_back(uci::moveToUci(move, board_.chess960()));
            board_.makeMove(move);
        }

        return moves;
    }();

    // Reset data
    played_moves_.clear();

    data_           = MatchData();
    draw_tracker_   = DrawTacker();
    resign_tracker_ = ResignTracker();

    // Add opening moves to played moves
    played_moves_.insert(played_moves_.end(), uci_moves.begin(), uci_moves.end());

    for (const auto& move : uci_moves) {
        data_.moves.push_back(MoveData(move, "0.00", 0, 0, 0, 0, 0));
    }

    player_1->info.color = board_.sideToMove();
    player_2->info.color = ~board_.sideToMove();

    data_.fen = opening_.fen;

    data_.start_time = Logger::getDateTime("%Y-%m-%dT%H:%M:%S %z");
    data_.date       = Logger::getDateTime("%Y-%m-%d");

    const auto start = clock::now();

    try {
        while (true) {
            if (atomic::stop.load()) {
                data_.termination = MatchTermination::INTERRUPT;
                break;
            }

            if (!playMove(*player_1, *player_2)) break;

            if (atomic::stop.load()) {
                data_.termination = MatchTermination::INTERRUPT;
                break;
            }

            if (!playMove(*player_2, *player_1)) break;
        }
    } catch (const std::exception& e) {
        if (tournament_options_.recover)
            data_.needs_restart = true;
        else {
            throw e;
        }
    }

    const auto end = clock::now();

    data_.end_time = Logger::getDateTime("%Y-%m-%dT%H:%M:%S %z");
    data_.duration = Logger::formatDuration(chrono::duration_cast<chrono::seconds>(end - start));

    data_.players = std::make_pair(player_1->info, player_2->info);

    delete player_1;
    delete player_2;
}

bool Match::playMove(Participant& us, Participant& opponent) {
    const auto gameover = board_.isGameOver();
    const auto name     = us.engine->getConfig().name;

    if (gameover.second == GameResult::DRAW) {
        setDraw(us, opponent);
    }

    if (gameover.second == GameResult::LOSE) {
        setLose(us, opponent);
    }

    if (gameover.first != GameResultReason::NONE) {
        data_.reason = convertChessReason(name, gameover.first);
        return false;
    }

    // disconnect
    if (!us.engine->isResponsive()) {
        setLose(us, opponent);

        data_.termination = MatchTermination::DISCONNECT;
        data_.reason      = name + Match::DISCONNECT_MSG;

        return false;
    }

    // write new uci position
    us.engine->writeEngine(Participant::buildPositionInput(played_moves_, start_position_));
    // write go command
    us.engine->writeEngine(us.buildGoInput(board_.sideToMove(), opponent.getTimeControl()));

    // wait for bestmove
    const auto t0 = clock::now();
    us.engine->readEngine("bestmove", us.getTimeoutThreshold());
    const auto t1 = clock::now();

    const auto elapsed_millis = chrono::duration_cast<chrono::milliseconds>(t1 - t0).count();

    if (atomic::stop) {
        data_.termination = MatchTermination::INTERRUPT;

        return false;
    }

    // Time forfeit
    if (!us.updateTime(elapsed_millis)) {
        setLose(us, opponent);

        data_.termination = MatchTermination::TIMEOUT;
        data_.reason      = name + Match::TIMEOUT_MSG;

        Logger::cout("Warning; Engine", name, "loses on time");

        return false;
    }

    updateDrawTracker(us);
    updateResignTracker(us);

    addMoveData(us, elapsed_millis);

    const auto best_move = us.engine->bestmove();
    const auto move      = uci::uciToMove(board_, best_move);

    Movelist moves;
    movegen::legalmoves(moves, board_);

    // Illegal move
    if (moves.find(move) == -1) {
        setLose(us, opponent);

        data_.termination = MatchTermination::ILLEGAL_MOVE;
        data_.reason      = name + Match::ILLEGAL_MSG;

        Logger::cout("Warning; Illegal move", best_move, "played by", name);

        return false;
    }

    board_.makeMove(move);

    return !adjudicate(us, opponent);
}

void Match::verifyPvLines(const Participant& us) {
    const auto verifyPv = [&](const std::vector<std::string>& tokens, const std::string& info) {
        auto tmp = board_;
        auto it  = std::find(tokens.begin(), tokens.end(), "pv");

        Movelist moves;

        // iterate over pv moves
        while (++it != tokens.end()) {
            movegen::legalmoves(moves, tmp);

            if (moves.find(uci::uciToMove(tmp, *it)) == -1) {
                Logger::cout("Warning; Illegal pv move ", *it, "pv:", info);
                break;
            }

            tmp.makeMove(uci::uciToMove(tmp, *it));
        }
    };

    for (const auto& info : us.engine->output()) {
        const auto tokens = str_utils::splitString(info, ' ');

        // skip lines without pv
        if (!str_utils::contains(tokens, "pv")) continue;

        verifyPv(tokens, info);
    }
}

void Match::setDraw(Participant& us, Participant& them) {
    us.info.result   = GameResult::DRAW;
    them.info.result = GameResult::DRAW;
}

void Match::setWin(Participant& us, Participant& them) {
    us.info.result   = GameResult::WIN;
    them.info.result = GameResult::LOSE;
}

void Match::setLose(Participant& us, Participant& them) {
    us.info.result   = GameResult::LOSE;
    them.info.result = GameResult::WIN;
}

void Match::updateDrawTracker(const Participant& player) {
    const auto score = player.engine->lastScore();

    if (played_moves_.size() >= tournament_options_.draw.move_number &&
        std::abs(score) <= tournament_options_.draw.score &&
        player.engine->lastScoreType() == "cp") {
        draw_tracker_.draw_moves++;
    } else {
        draw_tracker_.draw_moves = 0;
    }
}

void Match::updateResignTracker(const Participant& player) {
    const auto score = player.engine->lastScore();

    if (std::abs(score) >= tournament_options_.resign.score &&
        player.engine->lastScoreType() == "cp") {
        resign_tracker_.resign_moves++;
    } else {
        resign_tracker_.resign_moves = 0;
    }
}

bool Match::adjudicate(Participant& us, Participant& them) {
    if (tournament_options_.draw.enabled &&
        draw_tracker_.draw_moves >= tournament_options_.draw.move_count) {
        setDraw(us, them);

        data_.termination = MatchTermination::ADJUDICATION;
        data_.reason      = Match::ADJUDICATION_MSG;

        return true;
    }

    if (tournament_options_.resign.enabled &&
        resign_tracker_.resign_moves >= tournament_options_.resign.move_count) {
        data_.reason = us.engine->getConfig().name;

        if (us.engine->lastScore() < tournament_options_.resign.score) {
            setLose(us, them);

            data_.termination = MatchTermination::ADJUDICATION;
            data_.reason += Match::ADJUDICATION_LOSE_MSG;
        } else {
            setWin(us, them);

            data_.termination = MatchTermination::ADJUDICATION;
            data_.reason += Match::ADJUDICATION_WIN_MSG;
        }

        return true;
    }

    return false;
}

std::string Match::convertChessReason(const std::string& engine_name, GameResultReason reason) {
    if (reason == GameResultReason::CHECKMATE) {
        return engine_name + Match::CHECKMATE_MSG;
    }
    if (reason == GameResultReason::STALEMATE) {
        return Match::STALEMATE_MSG;
    }
    if (reason == GameResultReason::INSUFFICIENT_MATERIAL) {
        return Match::INSUFFICIENT_MSG;
    }
    if (reason == GameResultReason::THREEFOLD_REPETITION) {
        return Match::REPETITION_MSG;
    }
    if (reason == GameResultReason::FIFTY_MOVE_RULE) {
        return Match::FIFTY_MSG;
    }

    return "";
}

}  // namespace fast_chess