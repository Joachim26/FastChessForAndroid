#pragma once

#include <matchmaking/roundrobin.hpp>

#include <types/tournament_options.hpp>

namespace fast_chess {

/// @brief Manages the tournament, currenlty wraps round robin but can be extended to support
/// different tournament types
class Tournament {
   public:
    explicit Tournament(const cmd::TournamentOptions &game_config) noexcept;

    ~Tournament() { saveJson(); }

    void start(std::vector<EngineConfiguration> engine_configs);
    void stop() { round_robin_.stop(); }

    [[nodiscard]] stats_map getResults() noexcept { return round_robin_.getResults(); }

    [[nodiscard]] RoundRobin *roundRobin() { return &round_robin_; }

    void saveJson() {
        nlohmann::ordered_json jsonfile = tournament_options_;
        jsonfile["engines"]             = engine_configs_;
        jsonfile["stats"]               = round_robin_.getResults();

        Logger::log<Logger::Level::TRACE>("Saving results...");

        std::ofstream file("config.json");
        file << std::setw(4) << jsonfile << std::endl;

        Logger::log("Saved results.");
    }

   private:
    void loadConfig(const cmd::TournamentOptions &game_config);

    static void validateEngines(std::vector<EngineConfiguration> &configs);

    RoundRobin round_robin_;

    cmd::TournamentOptions tournament_options_;
    std::vector<EngineConfiguration> engine_configs_;
};

}  // namespace fast_chess