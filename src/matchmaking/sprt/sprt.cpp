#include <matchmaking/sprt/sprt.hpp>

#include <cmath>
#include <iomanip>
#include <sstream>

#include <util/logger/logger.hpp>

namespace fast_chess {

SPRT::SPRT(double alpha, double beta, double elo0, double elo1) {
    valid_ = alpha != 0.0 && beta != 0.0 && elo0 < elo1;
    if (isValid()) {
        lower_ = std::log(beta / (1 - alpha));
        upper_ = std::log((1 - beta) / alpha);
        s0_    = getLL(elo0);
        s1_    = getLL(elo1);

        elo0_ = elo0;
        elo1_ = elo1;

        Logger::log<Logger::Level::INFO>("Initialized valid SPRT configuration.");
    } else if (!(alpha == 0.0 && beta == 0.0 && elo0 == 0.0 && elo1 == 0.0)) {
        Logger::log<Logger::Level::INFO>("No valid SPRT configuration was found!");
    }
}

double SPRT::getLL(double elo) noexcept { return 1.0 / (1.0 + std::pow(10.0, -elo / 400.0)); }

double SPRT::getLLR(int win, int draw, int loss) const noexcept {
    if (win == 0 || draw == 0 || loss == 0 || !valid_) return 0.0;

    const double games = win + draw + loss;
    const double W = double(win) / games, D = double(draw) / games;
    const double a     = W + D / 2;
    const double b     = W + D / 4;
    const double var   = b - std::pow(a, 2);
    const double var_s = var / games;
    return (s1_ - s0_) * (2 * a - s0_ - s1_) / var_s / 2.0;
}

SPRTResult SPRT::getResult(double llr) const noexcept {
    if (!valid_) return SPRT_CONTINUE;

    if (llr > upper_)
        return SPRT_H0;
    else if (llr < lower_)
        return SPRT_H1;
    else
        return SPRT_CONTINUE;
}

std::string SPRT::getBounds() const noexcept {
    std::stringstream ss;
    ss << "(" << std::fixed << std::setprecision(2) << lower_ << ", " << std::fixed
       << std::setprecision(2) << upper_ << ")";
    return ss.str();
}

std::string SPRT::getElo() const noexcept {
    std::stringstream ss;
    ss << "[" << std::fixed << std::setprecision(2) << elo0_ << ", " << std::fixed
       << std::setprecision(2) << elo1_ << "]";

    return ss.str();
}

bool SPRT::isValid() const noexcept { return valid_; }

}  // namespace fast_chess
