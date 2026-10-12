#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>
#include <vector>
#include "Dsp.h"

class AuroraProcessor : public juce::AudioProcessor
{
public:
    AuroraProcessor();
    ~AuroraProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Aurora Guitar Studio"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // Loads a cabinet impulse response (WAV/AIFF/FLAC). Call from the message thread.
    void loadIR (const juce::File& file);

    juce::AudioProcessorValueTreeState apvts;

    // Read by the GUI (meters, gate LED, IR name)
    std::atomic<float> meterPeak[2] { 0.f, 0.f };
    std::atomic<float> gateOpen { 1.f };
    bool hasCustomIR() const { return irLoaded.load(); }
    juce::String getIRName() const
    {
        return irPath.isEmpty() ? juce::String ("Built-in 4x12")
                                : juce::File (irPath).getFileNameWithoutExtension();
    }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double fs = 44100.0;

    // 4x oversampling around all nonlinear stages (reduces aliasing = cleaner distortion)
    juce::dsp::Oversampling<float> os { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true };

    // Noise gate state
    float gEnv = 0.f, gGain = 1.f;

    // Pedal (overdrive) filters
    Biquad pedHP, pedLP;
    // Preamp
    Biquad couplingHP, dcBlock, millerLP;
    // Tone stack
    Biquad tBass, tMid, tTreble, tPres;
    // Power amp sag envelope
    float sagEnv[2] { 0.f, 0.f };

    // Cabinet
    juce::dsp::Convolution conv;
    std::atomic<bool> irLoaded { false };
    juce::String irPath;
    Biquad cabHP, cabLP1, cabLP2, cabRes, cabPresence;

    // Delay
    std::array<std::vector<float>, 2> dbuf;
    int wpos = 0;
    float fbLP[2] { 0.f, 0.f };
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> delaySm;

    // Reverb
    juce::dsp::Reverb rev;

    float lastOut = 1.f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AuroraProcessor)
};
