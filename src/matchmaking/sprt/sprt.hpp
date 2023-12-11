#pragma once

#include <string>

namespace fast_chess {

enum SPRTResult { SPRT_H0, SPRT_H1, SPRT_CONTINUE };

class SPRT {
   public:
    SPRT() = default;

    SPRT(double alpha, double beta, double elo0, double elo1);

    [[nodiscard]] bool isValid() const noexcept;

    [[nodiscard]] static double getLL(double elo) noexcept;
    [[nodiscard]] double getLLR(int win, int draw, int loss) const noexcept;

    [[nodiscard]] SPRTResult getResult(double llr) const noexcept;
    [[nodiscard]] std::string getBounds() const noexcept;
    [[nodiscard]] std::string getElo() const noexcept;

   private:
    double lower_ = 0.0;
    double upper_ = 0.0;
    double s0_    = 0.0;
    double s1_    = 0.0;

    double elo0_ = 0.0;
    double elo1_ = 0.0;

    bool valid_ = false;
};

}  // namespace fast_chess
