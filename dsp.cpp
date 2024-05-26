#include "dsp.h"
#include <thread>
#include <chrono>

void delay(finyl_buffer& buffer, Delay& d) {
  for (int i = 0; i<buffer.size(); i=i+2) {
    auto left = d.buf[d.index];
    auto right = d.buf[d.index+1];
    
    buffer[i] = clip_sample(buffer[i] * d.drymix + left * d.wetmix);
    buffer[i+1] = clip_sample(buffer[i+1] * d.drymix + right * d.wetmix);

    if (d.on) {
      d.buf[d.index] = d.feedback * (left + buffer[i]);
      d.buf[d.index+1] = d.feedback * (right + buffer[i+1]);
    } else {
      d.buf[d.index] = d.feedback * left;
      d.buf[d.index+1] = d.feedback * right;
    }
    
    d.index=d.index+2;
    if (d.index >= d.ssize) {
      d.index = 0;
    }
  }
}

constexpr double kMinimumFrequency = 10.0;
constexpr double kMaximumFrequency = static_cast<double>(44100) / 2.0;
constexpr double kStartupLoFreq = 246.0;
constexpr double kStartupHiFreq = 2484;
constexpr double kStartupMidFreq = 1100.0;
constexpr double kQBoost = 0.3;
constexpr double kQKill = 0.9;
constexpr double kQLowKillShelve = 0.4;
constexpr double kQHighKillShelve = 0.4;
constexpr double kKillGain = -23;
constexpr double kBesselStartRatio = 0.25;

/////////////////////////MATH
#if defined(__cpp_lib_constexpr_cmath) && __cpp_lib_constexpr_cmath >= 202202L
#define CMATH_CONSTEXPR constexpr
#else
#define CMATH_CONSTEXPR inline
#endif
template<typename T>
requires std::is_floating_point_v<T>
        CMATH_CONSTEXPR T ratio2db(T a) {
    return log10(a) * 20;
}
/////////////////////////

double getCenterFrequency(double low, double high) {
    double scaleLow = log10(low);
    double scaleHigh = log10(high);

    double scaleCenter = (scaleHigh - scaleLow) / 2 + scaleLow;
    return pow(10, scaleCenter);
}

double knobValueToBiquadGainDb(double value, bool kill) {
    if (kill) {
        return kKillGain;
    }
    if (value > kBesselStartRatio) {
        return ratio2db(value);
    }
    double startDB = ratio2db(kBesselStartRatio);
    value = 1 - (value / kBesselStartRatio);
    return (kKillGain - startDB) * value + startDB;
}

EngineFilterBiquad1LowShelving::EngineFilterBiquad1LowShelving(double centerFreq, double Q) {
    m_startFromDry = true;
    // printf("[INFO] EngineFilterBiquad1LowShelving center q: %lf %lf\n", centerFreq, Q);
    setFrequencyCorners(centerFreq, Q, 0);
}

void EngineFilterBiquad1LowShelving::setFrequencyCorners(double centerFreq,
                                                         double Q,
                                                         double dBgain) {
  //MYNOTE: the core
  //MYNOTE: dBgain is bqGainLow (0 when no effect)
    snprintf(m_spec, sizeof(m_spec), "LsBq/%.10f/%.10f", Q, dBgain);
    // printf("LOWKILL spec %s (center: %lf)\n", m_spec, centerFreq);
    setCoefs(m_spec, sizeof(m_spec), centerFreq);
}


BiquadFullKillEQEffectGroupState::BiquadFullKillEQEffectGroupState():
  m_oldLowKill(0),
  m_oldLowBoost(0),
  m_loFreqCorner(250.004443){

  m_lowBoost = std::make_unique<EngineFilterBiquad1Peaking>(kStartupLoFreq, kQBoost);

  m_lowKill = std::make_unique<EngineFilterBiquad1LowShelving>(kStartupLoFreq * 2, kQLowKillShelve);

  setFilters(kStartupLoFreq,
             kStartupHiFreq);
}

void BiquadFullKillEQEffectGroupState::setFilters(double lowFreqCorner, double highFreqCorner) {
  double lowCenter = getCenterFrequency(kMinimumFrequency, lowFreqCorner);
  double midCenter = getCenterFrequency(lowFreqCorner, highFreqCorner);
  double highCenter = getCenterFrequency(highFreqCorner, kMaximumFrequency);

  m_lowBoost->setFrequencyCorners(lowCenter, kQBoost, m_oldLowBoost);
  m_lowKill->setFrequencyCorners(lowCenter * 2, kQLowKillShelve, m_oldLowKill);

}

BiquadFullKillEQEffect::BiquadFullKillEQEffect() {
  
}




void BiquadFullKillEQEffect::process(BiquadFullKillEQEffectGroupState *pState,
                                const CSAMPLE *pInput, CSAMPLE *pOutput) {
  double bqGainLow = pState->bqGainLow;
  double bqGainMid = pState->bqGainMid;
  double bqGainHigh = pState->bqGainHigh;

  //bqgainlow: max 12.041200 min -23.0
  
  if (bqGainLow > 0.0 || pState->m_oldLowBoost > 0.0) {
    if (bqGainLow != pState->m_oldLowBoost) {
      double lowCenter = getCenterFrequency(kMinimumFrequency, pState->m_loFreqCorner);
      pState->m_lowBoost->setFrequencyCorners(lowCenter, kQBoost, bqGainLow);
      pState->m_oldLowBoost = bqGainLow;
    }
    if (bqGainLow > 0.0) {
      pState->m_lowBoost->process(pInput, pOutput, period_size_2);
    } else {
      pState->m_lowBoost->processAndPauseFilter(pInput, pOutput, period_size_2);
    }
  } else {
    pState->m_lowBoost->pauseFilter();
  }
  
  if (bqGainLow < 0.0 || pState->m_oldLowKill < 0.0) {
    if (bqGainLow != pState->m_oldLowKill) {
      double lowCenter = getCenterFrequency(kMinimumFrequency, pState->m_loFreqCorner);
      pState->m_lowKill->setFrequencyCorners(lowCenter*2, kQLowKillShelve, bqGainLow);
      pState->m_oldLowKill = bqGainLow;
    }
    if (bqGainLow < 0.0) {
      pState->m_lowKill->process(pInput, pOutput, period_size_2);
    } else {
      pState->m_lowKill->processAndPauseFilter(pInput, pOutput, period_size_2);
    }
  } else {
    pState->m_lowKill->pauseFilter();
  }

  // if (bqGainMid > 0.0 || pState->m_oldMidBoost > 0.0) {
  //   if (bqGainMid != pState->m_oldMidBoost) {
  //     double midCenter = getCenterFrequency(
  //                                           pState->m_loFreqCorner, pState->m_highFreqCorner);
  //     pState->m_midBoost->setFrequencyCorners(
  //                                             engineParameters.sampleRate(), midCenter, kQBoost, bqGainMid);
  //     pState->m_oldMidBoost = bqGainMid;
  //   }
  //   if (bqGainMid > 0.0) {
  //     pState->m_midBoost->process(
  //                                 inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   } else {
  //     pState->m_midBoost->processAndPauseFilter(
  //                                               inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   }
  //   ++bufIndex;
  // } else {
  //   pState->m_midBoost->pauseFilter();
  // }

  // if (bqGainMid < 0.0 || pState->m_oldMidKill < 0.0) {
  //   if (bqGainMid != pState->m_oldMidKill) {
  //     double midCenter = getCenterFrequency(
  //                                           pState->m_loFreqCorner, pState->m_highFreqCorner);
  //     pState->m_midKill->setFrequencyCorners(
  //                                            engineParameters.sampleRate(), midCenter, kQKill, bqGainMid);
  //     pState->m_oldMidKill = bqGainMid;
  //   }
  //   if (bqGainMid < 0.0) {
  //     pState->m_midKill->process(
  //                                inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   } else {
  //     pState->m_midKill->processAndPauseFilter(
  //                                              inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   }
  //   ++bufIndex;
  // } else {
  //   pState->m_midKill->pauseFilter();
  // }

  // if (bqGainHigh > 0.0 || pState->m_oldHighBoost > 0.0) {
  //   if (bqGainHigh != pState->m_oldHighBoost) {
  //     double highCenter = getCenterFrequency(
  //                                            pState->m_highFreqCorner, kMaximumFrequency);
  //     pState->m_highBoost->setFrequencyCorners(
  //                                              engineParameters.sampleRate(), highCenter, kQBoost, bqGainHigh);
  //     pState->m_oldHighBoost = bqGainHigh;
  //   }
  //   if (bqGainHigh > 0.0) {
  //     pState->m_highBoost->process(
  //                                  inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   } else {
  //     pState->m_highBoost->processAndPauseFilter(
  //                                                inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   }
  //   ++bufIndex;
  // } else {
  //   pState->m_highBoost->pauseFilter();
  // }

  // if (bqGainHigh < 0.0 || pState->m_oldHighKill < 0.0) {
  //   if (bqGainHigh != pState->m_oldHighKill) {
  //     double highCenter = getCenterFrequency(
  //                                            pState->m_highFreqCorner, kMaximumFrequency);
  //     pState->m_highKill->setFrequencyCorners(
  //                                             engineParameters.sampleRate(), highCenter / 2, kQHighKillShelve, bqGainHigh);
  //     pState->m_oldHighKill = bqGainHigh;
  //   }
  //   if (bqGainHigh < 0.0) {
  //     pState->m_highKill->process(
  //                                 inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   } else {
  //     pState->m_highKill->processAndPauseFilter(
  //                                               inBuffer[bufIndex], outBuffer[bufIndex], engineParameters.samplesPerBuffer());
  //   }
  //   ++bufIndex;
  // } else {
  //   pState->m_highKill->pauseFilter();
  // }
  
}

EngineFilterBiquad1Peaking::EngineFilterBiquad1Peaking(double centerFreq, double Q) {
  m_startFromDry = true;
  setFrequencyCorners(centerFreq, Q, 0);
}

void EngineFilterBiquad1Peaking::setFrequencyCorners(double centerFreq, double Q, double dBgain) {
  snprintf(m_spec, sizeof(m_spec), "PkBq/%.10f/%.10f", Q, dBgain);

  setCoefs(m_spec, sizeof(m_spec), centerFreq);
}
