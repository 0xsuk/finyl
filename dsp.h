#ifndef DSP_H
#define DSP_H

#include <cstring>
#include <cmath>
#include "finyl.h"
#include "fidlib.h"
#include "Reverb.h"
#include <fftw3.h>

#define MAX_DELAY_SEC 10
#define MAX_DELAY_SAMPLES 44100*MAX_DELAY_SEC*2
#define FIDSPEC_LENGTH 40
typedef float CSAMPLE;

struct DelayState {
  finyl_sample buf[MAX_DELAY_SAMPLES] = {0};
  bool on = false;
  int ssize = 0;
  int index = 0;
  double wetmix;
  double drymix;
  double feedback;

  void setMsize(int _msize) {
    int _ssize = _msize*2;
    if (_ssize>MAX_DELAY_SAMPLES) {
      ssize = MAX_DELAY_SAMPLES;
    } else {
      ssize = _ssize;
    }
  }
};
void delay(finyl_buffer& buffer, DelayState& d);

class EffectState {
 public:
  EffectState() {};
  virtual ~EffectState(){};
};

template<typename EffectSpecificState>
class EffectProcessor {
 public:
  EffectProcessor() {};
  virtual ~EffectProcessor() {};
  virtual void process(EffectSpecificState* pState,
                       const CSAMPLE* pInput,
                       CSAMPLE* pOutput
                       ) = 0;
};

enum IIRPass {
    IIR_LP,
    IIR_BP,
    IIR_HP,
    IIR_LPMO,
    IIR_HPMO,
    IIR_LP2,
    IIR_HP2,
};

template<unsigned int SIZE, enum IIRPass PASS>
class EngineFilterIIR {
  public:
    EngineFilterIIR()
            : m_doRamping(false),
              m_doStart(false),
              m_startFromDry(false) {
        memset(m_coef, 0, sizeof(m_coef));
        pauseFilter();
    }

    virtual ~EngineFilterIIR() {};

    // this can be called continuously for Filters that have own ramping
    // or need no fade when disabling
    void pauseFilter() {
        if (!m_doStart) {
            pauseFilterInner();
        }
    }

    void setStartFromDry(bool val) {
        m_startFromDry = val;
    }

    // this is can be used instead off a final process() call before pause
    // It fades to dry or 0 according to the m_startFromDry parameter
    // it is an alternative for using pauseFillter() calls
    void processAndPauseFilter(
            const CSAMPLE* pIn,
            CSAMPLE* pOutput,
            int iBufferSize) {
        process(pIn, pOutput, iBufferSize);
        //MYNOTE: SampleUtil?
        // if (m_startFromDry) {
        //     SampleUtil::linearCrossfadeBuffersOut(
        //             pOutput, // fade out filtered
        //             pIn,     // fade in dry
        //             iBufferSize);
        // } else {
        //     SampleUtil::applyRampingGain(
        //             pOutput, 1.0, 0, // fade out filtered
        //             iBufferSize);
        // }
        pauseFilterInner();
    }

    void initBuffers() {
        // Copy the current buffers into the old buffers
        memcpy(m_oldBuf1, m_buf1, sizeof(m_buf1));
        memcpy(m_oldBuf2, m_buf2, sizeof(m_buf2));
        // Set the current buffers to 0
        memset(m_buf1, 0, sizeof(m_buf1));
        memset(m_buf2, 0, sizeof(m_buf2));
        m_doRamping = true;
    }

    void setCoefs(const char* spec,
            size_t bufsize,
            double freq0,
            double freq1 = 0,
            int adj = 0) {
        char spec_d[FIDSPEC_LENGTH];
        // VERIFY_OR_DEBUG_ASSERT(bufsize <= sizeof(spec_d)) {
            // return;
        // }
        // Copy to dynamic-ish memory to prevent fidlib API breakage.
        std::strncpy(spec_d, spec, bufsize);

        // Copy the old coefficients into m_oldCoef
        memcpy(m_oldCoef, m_coef, sizeof(m_coef));

        m_coef[0] = fid_design_coef(m_coef + 1, SIZE, spec_d, gApp.audio->get_sample_rate(), freq0, freq1, adj);

        initBuffers();

// #if(IIR_ANALYSIS)
//         char* desc;
//         FidFilter* filt = fid_design(spec_d, sampleRate, freq0, freq1, adj, &desc);
//         int delay = fid_calc_delay(filt);
//         qDebug() << QString().fromLatin1(desc);
//         qDebug() << spec_d << "delay:" << delay;
//         double resp0, phase0;
//         resp0 = fid_response_pha(filt, freq0 / sampleRate, &phase0);
//         qDebug() << "freq0:" << freq0 << resp0 << phase0;
//         if (freq1) {
//             double resp1, phase1;
//             resp1 = fid_response_pha(filt, freq1 / sampleRate, &phase1);
//             qDebug() << "freq1:" << freq1 << resp1 << phase1;
//         }
//         double resp2, phase2;
//         resp2 = fid_response_pha(filt, freq0 / sampleRate / 2, &phase2);
//         qDebug() << "freq2:" << freq0 / 2 << resp2 << phase2;
//         double resp3, phase3;
//         resp3 = fid_response_pha(filt, freq0 / sampleRate * 2, &phase3);
//         qDebug() << "freq3:" << freq0 * 2 << resp3 << phase3;
//         double resp4, phase4;
//         resp4 = fid_response_pha(filt, freq0 / sampleRate / 2.2, &phase4);
//         qDebug() << "freq4:" << freq0 / 2.2 << resp4 << phase4;
//         double resp5, phase5;
//         resp5 = fid_response_pha(filt, freq0 / sampleRate * 2.2, &phase5);
//         qDebug() << "freq5:" << freq0 * 2.2 << resp5 << phase5;
//         free(filt);
// #endif
    }

//     void setCoefs2(double sampleRate,
//             int n_coef1,
//             const char* spec1,
//             size_t spec1size,
//             double freq01,
//             double freq11,
//             int adj1,
//             const char* spec2,
//             size_t spec2size,
//             double freq02,
//             double freq12,
//             int adj2) {
//         char spec1_d[FIDSPEC_LENGTH];
//         char spec2_d[FIDSPEC_LENGTH];
//         // VERIFY_OR_DEBUG_ASSERT(spec1size <= sizeof(spec1_d) && spec2size <= sizeof(spec2_d)) {
//             // return;
//         // }

//         // Copy to dynamic-ish memory to prevent fidlib API breakage.
//         std::strncpy(spec1_d, spec1, spec1size);
//         std::strncpy(spec2_d, spec2, spec2size);
//         spec1_d[FIDSPEC_LENGTH - 1] = '\0';
//         spec2_d[FIDSPEC_LENGTH - 1] = '\0';

//         // Copy the old coefficients into m_oldCoef
//         memcpy(m_oldCoef, m_coef, sizeof(m_coef));
//         m_coef[0] = fid_design_coef(m_coef + 1,
//                             n_coef1,
//                             spec1,
//                             sampleRate,
//                             freq01,
//                             freq11,
//                             adj1) *
//                 fid_design_coef(m_coef + 1 + n_coef1,
//                         SIZE - n_coef1,
//                         spec2,
//                         sampleRate,
//                         freq02,
//                         freq12,
//                         adj2);

//         initBuffers();

// #if(IIR_ANALYSIS)
//         char* desc1;
//         char* desc2;
//         FidFilter* filt1 = fid_design(spec1, sampleRate, freq01, freq11, adj1, &desc1);
//         FidFilter* filt2 = fid_design(spec2, sampleRate, freq02, freq12, adj2, &desc2);
//         FidFilter* filt = fid_cat(1, filt1, filt2, NULL);
//         int delay = fid_calc_delay(filt);
//         qDebug() << QString().fromLatin1(desc1) << "X"
//                  << QString().fromLatin1(desc2);
//         qDebug() << spec1 << "X" << spec2 << "delay:" << delay;
//         double resp0, phase0;
//         resp0 = fid_response_pha(filt, freq01 / sampleRate, &phase0);
//         qDebug() << "freq01:" << freq01 << resp0 << phase0;
//         resp0 = fid_response_pha(filt, freq01 / sampleRate, &phase0);
//         qDebug() << "freq02:" << freq02 << resp0 << phase0;
//         if (freq11) {
//             double resp1, phase1;
//             resp1 = fid_response_pha(filt, freq11 / sampleRate, &phase1);
//             qDebug() << "freq1:" << freq11 << resp1 << phase1;
//         }
//         if (freq12) {
//             double resp1, phase1;
//             resp1 = fid_response_pha(filt, freq12 / sampleRate, &phase1);
//             qDebug() << "freq1:" << freq12 << resp1 << phase1;
//         }
//         double resp2, phase2;
//         resp2 = fid_response_pha(filt, freq01 / sampleRate / 2, &phase2);
//         qDebug() << "freq2:" << freq01 / 2 << resp2 << phase2;
//         double resp3, phase3;
//         resp3 = fid_response_pha(filt, freq01 / sampleRate * 2, &phase3);
//         qDebug() << "freq3:" << freq01 * 2 << resp3 << phase3;
//         double resp4, phase4;
//         resp4 = fid_response_pha(filt, freq01 / sampleRate / 2.2, &phase4);
//         qDebug() << "freq4:" << freq01 / 2.2 << resp4 << phase4;
//         double resp5, phase5;
//         resp5 = fid_response_pha(filt, freq01 / sampleRate * 2.2, &phase5);
//         qDebug() << "freq5:" << freq01 * 2.2 << resp5 << phase5;
//         free(filt);
// #endif
//     }

    virtual void assumeSettled() {
        m_doRamping = false;
        m_doStart = false;
    }

    virtual void process(const CSAMPLE* pIn, CSAMPLE* pOutput,
                         const int iBufferSize) {
        if (!m_doRamping) {
            for (int i = 0; i < iBufferSize; i += 2) {
                pOutput[i] = static_cast<CSAMPLE>(processSample(m_coef, m_buf1, pIn[i]));
                pOutput[i + 1] = static_cast<CSAMPLE>(processSample(m_coef, m_buf2, pIn[i + 1]));
            }
        } else {
            double cross_mix = 0.0;
            double cross_inc = 4.0 / static_cast<double>(iBufferSize);
            for (int i = 0; i < iBufferSize; i += 2) {
                // Do a linear cross fade between the output of the old
                // Filter and the new filter.
                // The new filter is settled for Input = 0 and it sees
                // all frequencies of the rectangular start impulse.
                // Since the group delay, after which the start impulse
                // has passed is unknown here, we just what the half
                // iBufferSize until we use the samples of the new filter.
                // In one of the previous version we have faded the Input
                // of the new filter but it turns out that this produces
                // a gain drop due to the filter delay which is more
                // conspicuous than the settling noise.
                double old1;
                double old2;
                if (!m_doStart) {
                    // Process old filter, but only if we do not do a fresh start
                    old1 = static_cast<CSAMPLE>(processSample(m_oldCoef, m_oldBuf1, pIn[i]));
                    old2 = static_cast<CSAMPLE>(processSample(m_oldCoef, m_oldBuf2, pIn[i + 1]));
                } else {
                    if (m_startFromDry) {
                        old1 = pIn[i];
                        old2 = pIn[i + 1];
                    } else {
                        old1 = 0;
                        old2 = 0;
                    }
                }
                double new1 = static_cast<CSAMPLE>(processSample(m_coef, m_buf1, pIn[i]));
                double new2 = static_cast<CSAMPLE>(processSample(m_coef, m_buf2, pIn[i + 1]));

                if (i < iBufferSize / 2) {
                    pOutput[i] = static_cast<CSAMPLE>(old1);
                    pOutput[i + 1] = static_cast<CSAMPLE>(old2);
                } else {
                    pOutput[i] = static_cast<CSAMPLE>(new1 * cross_mix + old1 * (1.0 - cross_mix));
                    pOutput[i + 1] = static_cast<CSAMPLE>(
                            new2 * cross_mix + old2 * (1.0 - cross_mix));
                    cross_mix += cross_inc;
                }
            }
            m_doRamping = false;
            m_doStart = false;
        }
    }

  protected:
    inline double processSample(double* coef, double* buf, double val);
    inline void pauseFilterInner() {
        // Set the current buffers to 0
        memset(m_buf1, 0, sizeof(m_buf1));
        memset(m_buf2, 0, sizeof(m_buf2));
        m_doRamping = true;
        m_doStart = true;
    }

    double m_coef[SIZE + 1];
    // Old coefficients needed for ramping
    double m_oldCoef[SIZE + 1];

    // Channel 1 state
    double m_buf1[SIZE];
    // Old channel 1 buffer needed for ramping
    double m_oldBuf1[SIZE];

    // Channel 2 state
    double m_buf2[SIZE];
    // Old channel 2 buffer needed for ramping
    double m_oldBuf2[SIZE];

    // Flag set to true if ramping needs to be done
    bool m_doRamping;
    // Flag set to true if old filter is invalid
    bool m_doStart;
    // Flag set to true if this is a chained filter
    bool m_startFromDry;
};

template<>
inline double EngineFilterIIR<5, IIR_BP>::processSample(double* coef,
                                                        double* buf,
                                                        double val) {
  // printf("IIR_BP 5\n");
    double tmp, fir, iir;
    tmp = buf[0]; buf[0] = buf[1];
    iir = val * coef[0];
    iir -= coef[1] * tmp; fir = coef[2] * tmp;
    iir -= coef[3] * buf[0]; fir += coef[4] * buf[0];
    fir += coef[5] * iir;
    buf[1] = iir; val = fir;
    return val;
}

class EngineFilterBiquad1LowShelving : public EngineFilterIIR<5, IIR_BP> {
public:
  EngineFilterBiquad1LowShelving(double centerFreq, double Q);
  void setFrequencyCorners(double centerFreq, double Q, double dBgain);
private:
  char m_spec[FIDSPEC_LENGTH];
};

class EngineFilterBiquad1Peaking : public EngineFilterIIR<5, IIR_BP> {
public:
  EngineFilterBiquad1Peaking(double centerFreq, double Q);
  void setFrequencyCorners(double centerFreq,
                           double Q,
                           double dBgain);

private:
  char m_spec[FIDSPEC_LENGTH];
};

class EngineFilterBiquad1HighShelving : public EngineFilterIIR<5, IIR_BP> {
public:
  EngineFilterBiquad1HighShelving(double centerFreq, double Q);
  void setFrequencyCorners(double centerFreq,
                           double Q,
                           double dBgain);

private:
  char m_spec[FIDSPEC_LENGTH];

};

class BiquadFullKillEQEffectGroupState : public EffectState {
public:
  BiquadFullKillEQEffectGroupState();
  void setFilters(double lowFreqCorner, double highFreqCorner); 
  std::unique_ptr<EngineFilterBiquad1Peaking> m_lowBoost;
  std::unique_ptr<EngineFilterBiquad1Peaking> m_midBoost;
  std::unique_ptr<EngineFilterBiquad1Peaking> m_highBoost;
  std::unique_ptr<EngineFilterBiquad1LowShelving> m_lowKill;
  std::unique_ptr<EngineFilterBiquad1Peaking> m_midKill;
  std::unique_ptr<EngineFilterBiquad1HighShelving> m_highKill;
  
  double m_oldLowBoost;
  double m_oldLowKill;
  double m_oldMidBoost;
  double m_oldMidKill;
  double m_oldHighBoost;
  double m_oldHighKill;
  double m_loFreqCorner;
  double m_highFreqCorner;

  double bqGainLow;
  double bqGainMid;
  double bqGainHigh;
};

class BiquadFullKillEQEffect : public EffectProcessor<BiquadFullKillEQEffectGroupState> {
public:
  BiquadFullKillEQEffect();
  ~BiquadFullKillEQEffect() override = default;

  void process(BiquadFullKillEQEffectGroupState* pState,
               const CSAMPLE* pInput,
               CSAMPLE* pOutput) override;
private:
  //dont know the use
  // double m_pLoFreqCorner;
};


class ReverbGroupState : public EffectState {
public:
  ReverbGroupState() : sendPrevious(0) {
  }
  ~ReverbGroupState() override = default;

  // void engineParametersChanged(const mixxx::EngineParameters& engineParameters) {
  //   sampleRate = engineParameters.sampleRate();
  //   sendPrevious = 0;
  // }

  float sampleRate;
  float sendPrevious;
  MixxxPlateX2 reverb;
};


class ReverbEffect : public EffectProcessor<ReverbGroupState> {
public:
  ReverbEffect();
  ~ReverbEffect() override = default;

  void process(
               ReverbGroupState* pState,
               const CSAMPLE* pInput,
               CSAMPLE* pOutput
               // const mixxx::EngineParameters& engineParameters,
               // const EffectEnableState enableState,
               // const GroupFeatureState& groupFeatures
               ) override;

                      
};


struct FFTState {
public:
  FFTState();
  
  int out_size;
  float* left_in;
  float* right_in;
  fftwf_complex* left_out;
  fftwf_complex* right_out;
  fftwf_plan left_fplan;
  fftwf_plan right_fplan;
  fftwf_plan left_iplan;
  fftwf_plan right_iplan;

  float* window;
  
  finyl_buffer* buffer;

  void set_target(finyl_buffer& _buffer);
  void forward();
  void inverse();
  void init_window();
};

struct SpectralGateState {
  float gateThreshold = 0.0f;
  
  SpectralGateState() = default;
  
  void setThreshold(float threshold) {
    gateThreshold = threshold;
  }
};

class SpectralGate {
public:
  static void process(FFTState* fftState, SpectralGateState* state) {
    const int size = fftState->out_size;
    processChannel(fftState->left_out, size, state->gateThreshold); 
    processChannel(fftState->right_out, size, state->gateThreshold);
  }
private:
  static void processChannel(fftwf_complex* channel, int size, float threshold) {
    for (int i = 0; i < size; i++) {
      float re = channel[i][0];
      float im = channel[i][1];
      
      float magnitude = sqrt(re * re + im * im);
      float phase = atan2(im, re);

      if (magnitude < threshold) {
        magnitude = 0.0f;
      }

      channel[i][0] = magnitude * cos(phase);
      channel[i][1] = magnitude * sin(phase);
    }
  }
};

#endif
