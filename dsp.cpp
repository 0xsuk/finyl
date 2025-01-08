#include "dsp.h"
#include <thread>
#include <chrono>
#include <cmath>

void delay(finyl_buffer& buffer, DelayState& d) {
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

EngineFilterBiquad1HighShelving::EngineFilterBiquad1HighShelving(double centerFreq, double Q) {
  m_startFromDry = true;
  setFrequencyCorners(centerFreq, Q, 0);
}

void EngineFilterBiquad1HighShelving::setFrequencyCorners(double centerFreq,
                                                          double Q,
                                                          double dBgain) {
  snprintf(m_spec, sizeof(m_spec), "HsBq/%.10f/%.10f", Q, dBgain);
  setCoefs(m_spec, sizeof(m_spec), centerFreq);
}


BiquadFullKillEQEffectGroupState::BiquadFullKillEQEffectGroupState():
  m_oldLowBoost(0),
  m_oldLowKill(0),
  m_oldMidBoost(0),
  m_oldMidKill(0),
  m_oldHighBoost(0),
  m_oldHighKill(0),
  m_loFreqCorner(250.004443),
  m_highFreqCorner(2499.925102){ //TODO

  m_lowBoost = std::make_unique<EngineFilterBiquad1Peaking>(kStartupLoFreq, kQBoost);
  m_lowKill = std::make_unique<EngineFilterBiquad1LowShelving>(kStartupLoFreq * 2, kQLowKillShelve);

  m_midBoost = std::make_unique<EngineFilterBiquad1Peaking>(kStartupMidFreq, kQBoost);
  m_midKill = std::make_unique<EngineFilterBiquad1Peaking>(kStartupMidFreq, kQKill);

  m_highBoost = std::make_unique<EngineFilterBiquad1Peaking>(kStartupHiFreq, kQBoost);
  m_highKill = std::make_unique<EngineFilterBiquad1HighShelving>(kStartupHiFreq / 2, kQHighKillShelve);
  
  setFilters(kStartupLoFreq, kStartupHiFreq);
}

void BiquadFullKillEQEffectGroupState::setFilters(double lowFreqCorner, double highFreqCorner) {
  double lowCenter = getCenterFrequency(kMinimumFrequency, lowFreqCorner);
  double midCenter = getCenterFrequency(lowFreqCorner, highFreqCorner);
  double highCenter = getCenterFrequency(highFreqCorner, kMaximumFrequency);

  m_lowBoost->setFrequencyCorners(lowCenter, kQBoost, m_oldLowBoost);
  m_lowKill->setFrequencyCorners(lowCenter * 2, kQLowKillShelve, m_oldLowKill);

  m_midBoost->setFrequencyCorners(midCenter, kQBoost, m_oldMidBoost);
  m_midKill->setFrequencyCorners(midCenter, kQKill, m_oldMidKill);

  m_highBoost->setFrequencyCorners(highCenter, kQBoost, m_oldHighBoost);
  m_highKill->setFrequencyCorners(highCenter / 2, kQHighKillShelve, m_oldHighKill);
}

BiquadFullKillEQEffect::BiquadFullKillEQEffect() {
  
}




void BiquadFullKillEQEffect::process(BiquadFullKillEQEffectGroupState *pState,
                                const CSAMPLE *pInput, CSAMPLE *pOutput) {
  double bqGainLow = pState->bqGainLow;
  double bqGainMid = pState->bqGainMid;
  double bqGainHigh = pState->bqGainHigh;

  //bqgainlow: max 12.041200 min -23.0
  
  //NOTE: changed > to >=
  if (bqGainLow >= 0.0 || pState->m_oldLowBoost > 0.0) {
    if (bqGainLow != pState->m_oldLowBoost) {
      double lowCenter = getCenterFrequency(kMinimumFrequency, pState->m_loFreqCorner);
      pState->m_lowBoost->setFrequencyCorners(lowCenter, kQBoost, bqGainLow);
      pState->m_oldLowBoost = bqGainLow;
    }

    if (bqGainLow > 0.0) {
      pState->m_lowBoost->process(pInput, pOutput, gApp.audio->get_period_size_2());
    } else {
      pState->m_lowBoost->processAndPauseFilter(pInput, pOutput, gApp.audio->get_period_size_2());
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
      pState->m_lowKill->process(pInput, pOutput, gApp.audio->get_period_size_2());
    } else {
      pState->m_lowKill->processAndPauseFilter(pInput, pOutput, gApp.audio->get_period_size_2());
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

void ReverbEffect::process(ReverbGroupState *pState, const CSAMPLE *pInput, CSAMPLE *pOutput) {
  // Q_UNUSED(groupFeatures);

  // const auto decay = static_cast<sample_t>(m_pDecayParameter->value());
  // const auto bandwidth = static_cast<sample_t>(m_pBandWidthParameter->value());
  // const auto damping = static_cast<sample_t>(m_pDampingParameter->value());
  // const auto sendCurrent = static_cast<sample_t>(m_pSendParameter->value());

  // // Reinitialize the effect when turning it on to prevent replaying the old buffer
  // // from the last time the effect was enabled.
  // // Also, update the sample rate if it has changed.
  // if (enableState == EffectEnableState::Enabling ||
  //     pState->sampleRate != engineParameters.sampleRate()) {
  //   pState->reverb.init(engineParameters.sampleRate());
  //   pState->sampleRate = engineParameters.sampleRate();
  // }

  // pState->reverb.processBuffer(pInput,
  //                              pOutput,
  //                              engineParameters.samplesPerBuffer(),
  //                              bandwidth,
  //                              decay,
  //                              damping,
  //                              sendCurrent,
  //                              pState->sendPrevious);

  // // The ramping of the send parameter handles ramping when enabling, so
  // // this effect must handle ramping to dry when disabling itself (instead
  // // of being handled by EngineEffect::process).
  // if (enableState == EffectEnableState::Disabling) {
  //   SampleUtil::applyRampingGain(pOutput, 1.0, 0.0, engineParameters.samplesPerBuffer());
  //   pState->sendPrevious = 0;
  // } else {
  //   pState->sendPrevious = sendCurrent;
  // }

}


void deinterleave_to_float(finyl_sample* input, float *left, float *right, int period_size) {
    for (size_t i = 0; i < period_size; ++i) {
        left[i] = input[2 * i] / 32768.0;
        right[i] = input[2 * i + 1] / 32768.0;
    }
}
void float_to_interleave(float *left, float *right, finyl_sample *output, int period_size) {
  for (size_t i = 0; i < period_size; ++i) {
    output[2 * i] = (finyl_sample)(left[i] * 32768.0);
    output[2 * i + 1] = (finyl_sample)(right[i] * 32768.0);
  }
}

FFTState::FFTState() {
  out_size = gApp.audio->get_period_size() / 2 + 1;
  left_in = (float*)malloc(gApp.audio->get_period_size()*sizeof(float));
  right_in = (float*)malloc(gApp.audio->get_period_size()*sizeof(float));
  left_out = (fftwf_complex*)fftwf_malloc(sizeof(fftw_complex) * out_size);
  right_out = (fftwf_complex*)fftwf_malloc(sizeof(fftw_complex) * out_size);
  memset(left_out, 0, sizeof(fftwf_complex) * out_size);
  memset(right_out, 0, sizeof(fftwf_complex) * out_size);


  //FFTW_PRESERVE_INPUT is needed
  left_fplan = fftwf_plan_dft_r2c_1d(gApp.audio->get_period_size(), left_in, left_out, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
  right_fplan = fftwf_plan_dft_r2c_1d(gApp.audio->get_period_size(), right_in, right_out, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);

  left_iplan = fftwf_plan_dft_c2r_1d(gApp.audio->get_period_size(), left_out, left_in, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
  right_iplan = fftwf_plan_dft_c2r_1d(gApp.audio->get_period_size(), right_out, right_in, FFTW_ESTIMATE | FFTW_PRESERVE_INPUT);
}

void FFTState::set_target(finyl_buffer &_buffer) {
  buffer = &_buffer;
}
void FFTState::forward() {
  deinterleave_to_float(buffer->data(), left_in, right_in, gApp.audio->get_period_size());
  fftwf_execute(left_fplan);
  fftwf_execute(right_fplan);
}
void fft_normalize(float* in, int period_size) {
  for (int i = 0; i<period_size; i++) {
    in[i] /= period_size;
  }
}
void FFTState::inverse() {
  fftwf_execute(left_iplan);
  fftwf_execute(right_iplan);

  fft_normalize(left_in, gApp.audio->get_period_size());
  fft_normalize(right_in, gApp.audio->get_period_size());
  float_to_interleave(left_in, right_in, buffer->data(), gApp.audio->get_period_size());
}

