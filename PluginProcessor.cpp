#include "PluginProcessor.h"
#include "PluginEditor.h"

using juce::Decibels;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AuroraProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using R = juce::NormalisableRange<float>;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    auto f = [&] (const char* id, const char* name, float lo, float hi, float def)
    {
        l.add (std::make_unique<F> (juce::ParameterID { id, 1 }, name, R (lo, hi, 0.01f), def));
    };

    f ("in",       "Input (dB)",       -24.f, 24.f, 0.f);
    f ("gate",     "Gate Threshold",   -90.f, -30.f, -75.f);
    l.add (std::make_unique<B> (juce::ParameterID { "pedalOn", 1 }, "Pedal On", false));
    f ("drive",    "Pedal Drive",       0.f, 10.f, 4.f);
    f ("pedLevel", "Pedal Level (dB)", -12.f, 12.f, 0.f);
    f ("gain",     "Amp Gain",          0.f, 10.f, 5.f);
    f ("bass",     "Bass",              0.f, 10.f, 5.f);
    f ("mid",      "Mid",               0.f, 10.f, 5.f);
    f ("treble",   "Treble",            0.f, 10.f, 5.f);
    f ("presence", "Presence",          0.f, 10.f, 4.f);
    f ("master",   "Master",            0.f, 10.f, 5.f);
    f ("sag",      "Power Sag",         0.f, 1.f, 0.3f);
    l.add (std::make_unique<B> (juce::ParameterID { "cabOn", 1 }, "Cabinet On", true));
    f ("delayTime","Delay Time (ms)",  20.f, 1000.f, 380.f);
    f ("delayFb",  "Delay Feedback",    0.f, 0.9f, 0.3f);
    f ("delayMix", "Delay Mix",         0.f, 1.f, 0.f);
    f ("revSize",  "Reverb Size",       0.f, 1.f, 0.4f);
    f ("revMix",   "Reverb Mix",        0.f, 1.f, 0.15f);
    f ("out",      "Output (dB)",      -24.f, 12.f, 0.f);
    return l;
}

AuroraProcessor::AuroraProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

bool AuroraProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo())
           && l.getMainInputChannelSet() == out;
}

void AuroraProcessor::prepareToPlay (double sr, int blockSize)
{
    fs = sr;
    const double osFs = sr * 4.0;

    os.initProcessing ((size_t) blockSize);
    os.reset();
    setLatencySamples ((int) std::round (os.getLatencyInSamples()));

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) blockSize, 2 };
    conv.prepare (spec);  conv.reset();
    rev.prepare (spec);   rev.reset();

    pedHP.highpass (osFs, 720.f, 0.7f);   pedLP.lowpass (osFs, 4000.f, 0.7f);
    couplingHP.highpass (osFs, 30.f, 0.7f);
    dcBlock.highpass (osFs, 18.f, 0.7f);
    millerLP.lowpass (osFs, 7500.f, 0.6f);

    // Fallback "generic 4x12" cab when no IR is loaded
    cabHP.highpass (sr, 75.f, 0.8f);
    cabLP1.lowpass (sr, 5200.f, 0.7f);
    cabLP2.lowpass (sr, 6500.f, 0.7f);
    cabRes.peak (sr, 110.f, 1.2f, 3.f);
    cabPresence.peak (sr, 2500.f, 1.0f, 3.f);

    for (auto* b : { &pedHP, &pedLP, &couplingHP, &dcBlock, &millerLP, &tBass, &tMid, &tTreble, &tPres,
                     &cabHP, &cabLP1, &cabLP2, &cabRes, &cabPresence })
        b->reset();

    const int sz = (int) (sr * 2.0) + 4;
    for (auto& v : dbuf) v.assign ((size_t) sz, 0.f);
    wpos = 0;
    fbLP[0] = fbLP[1] = 0.f;
    delaySm.reset (sr, 0.15);
    delaySm.setCurrentAndTargetValue ((float) (*apvts.getRawParameterValue ("delayTime") * 0.001 * sr));

    sagEnv[0] = sagEnv[1] = 0.f;
    gEnv = 0.f; gGain = 1.f;
    lastOut = Decibels::decibelsToGain (apvts.getRawParameterValue ("out")->load());
}

void AuroraProcessor::loadIR (const juce::File& file)
{
    conv.loadImpulseResponse (file, juce::dsp::Convolution::Stereo::yes,
                              juce::dsp::Convolution::Trim::yes, 0,
                              juce::dsp::Convolution::Normalise::yes);
    irPath = file.getFullPathName();
    irLoaded = true;
}

//==============================================================================
void AuroraProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int nCh = juce::jmin (getTotalNumInputChannels(), getTotalNumOutputChannels(), 2);
    const int n = buffer.getNumSamples();
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, n);
    if (nCh <= 0 || n == 0) return;

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    const float inLin   = Decibels::decibelsToGain (P ("in"));
    const float thr     = Decibels::decibelsToGain (P ("gate"));
    const bool  pedalOn = P ("pedalOn") > 0.5f;
    const float drive   = P ("drive");
    const float pedLvl  = Decibels::decibelsToGain (P ("pedLevel"));
    const float gain    = P ("gain");
    const float master  = P ("master");
    const float sag     = P ("sag");
    const bool  cabOn   = P ("cabOn") > 0.5f;

    // ---- 1. Input gain + noise gate (base rate) ----------------------------------
    {
        const float aEnv = std::exp (-1.f / (0.001f * (float) fs));
        const float rEnv = std::exp (-1.f / (0.080f * (float) fs));
        const float aG   = std::exp (-1.f / (0.003f * (float) fs));
        const float rG   = std::exp (-1.f / (0.120f * (float) fs));

        for (int i = 0; i < n; ++i)
        {
            float det = 0.f;
            for (int c = 0; c < nCh; ++c)
                det = juce::jmax (det, std::abs (buffer.getSample (c, i) * inLin));

            gEnv = det > gEnv ? aEnv * gEnv + (1 - aEnv) * det : rEnv * gEnv + (1 - rEnv) * det;
            const float target = gEnv < thr ? 0.f : 1.f;
            gGain = target > gGain ? aG * gGain + (1 - aG) * target   // open fast
                                   : rG * gGain + (1 - rG) * target;  // close slow

            for (int c = 0; c < nCh; ++c)
                buffer.getWritePointer (c)[i] *= inLin * gGain;
        }
    }

    // ---- 2. Nonlinear section at 4x sample rate -----------------------------------
    {
        const double osFs = fs * 4.0;
        const float bass = (P ("bass")     - 5.f) * 2.4f;
        const float mid  = (P ("mid")      - 5.f) * 2.0f;
        const float tre  = (P ("treble")   - 5.f) * 2.4f;
        const float pres = (P ("presence") - 5.f) * 2.0f;
        tBass.lowShelf  (osFs, 100.f,  0.7f, bass);
        tMid.peak       (osFs, 650.f,  0.7f, mid);
        tTreble.highShelf (osFs, 3200.f, 0.7f, tre);
        tPres.highShelf (osFs, 5500.f, 0.7f, pres);

        const float pedGain = 1.f + drive * 6.f;
        const float g1 = 1.f + gain * gain * 0.6f;      // 1 .. 61
        const float g2 = 1.f + gain * 1.5f;             // 1 .. 16
        const float masterLin = 0.2f + master * 0.16f;
        const float bias = 0.2f;
        const float biasOut = std::tanh (bias);
        const float envCoef = std::exp (-1.f / (0.05f * (float) osFs));

        auto block = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) nCh);
        auto up = os.processSamplesUp (block);
        const int m = (int) up.getNumSamples();

        for (int c = 0; c < nCh; ++c)
        {
            float* d = up.getChannelPointer ((size_t) c);
            for (int i = 0; i < m; ++i)
            {
                float x = d[i];

                if (pedalOn)   // Tube-Screamer-style: HP -> soft clip -> LP, blended with dry
                {
                    float h = pedHP.process (c, x);
                    float s = std::tanh (h * pedGain);
                    s = pedLP.process (c, s);
                    x = (s * 0.8f + x * 0.2f) * pedLvl;
                }

                // Preamp stage 1: asymmetric clipping (adds even harmonics, "tube" flavour)
                x = couplingHP.process (c, x);
                x = std::tanh (x * g1 + bias) - biasOut;
                x = dcBlock.process (c, x);
                x = millerLP.process (c, x);

                // Preamp stage 2
                x = std::tanh (x * g2);

                // Tone stack
                x = tBass.process (c, x);
                x = tMid.process (c, x);
                x = tTreble.process (c, x);
                x = tPres.process (c, x);

                // Power amp with sag (gain drops as the supply is loaded)
                sagEnv[c] = envCoef * sagEnv[c] + (1 - envCoef) * std::abs (x);
                x *= masterLin / (1.f + sag * 2.f * sagEnv[c]);
                x = std::tanh (x * 1.2f) * 0.8f;

                d[i] = x;
            }
        }
        os.processSamplesDown (block);
    }

    // ---- 3. Cabinet ----------------------------------------------------------------
    if (cabOn)
    {
        if (irLoaded.load())
        {
            auto block = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) nCh);
            juce::dsp::ProcessContextReplacing<float> ctx (block);
            conv.process (ctx);
        }
        else
        {
            for (int c = 0; c < nCh; ++c)
            {
                float* d = buffer.getWritePointer (c);
                for (int i = 0; i < n; ++i)
                {
                    float x = cabHP.process (c, d[i]);
                    x = cabRes.process (c, x);
                    x = cabPresence.process (c, x);
                    x = cabLP1.process (c, x);
                    d[i] = cabLP2.process (c, x);
                }
            }
        }
    }

    // ---- 4. Delay ------------------------------------------------------------------
    {
        const float dMix = P ("delayMix");
        const float fb   = juce::jmin (P ("delayFb"), 0.92f);
        delaySm.setTargetValue (P ("delayTime") * 0.001f * (float) fs);
        const int sz = (int) dbuf[0].size();

        for (int i = 0; i < n; ++i)
        {
            const float dSamp = delaySm.getNextValue();
            float rp = (float) wpos - dSamp;
            while (rp < 0.f) rp += (float) sz;
            int i0 = (int) rp;
            const float fr = rp - (float) i0;
            i0 %= sz;
            const int i1 = (i0 + 1) % sz;

            for (int c = 0; c < nCh; ++c)
            {
                auto& b = dbuf[(size_t) c];
                const float dl = b[(size_t) i0] * (1.f - fr) + b[(size_t) i1] * fr;
                fbLP[c] += 0.4f * (dl - fbLP[c]);            // darker repeats, tape-like
                float* d = buffer.getWritePointer (c);
                b[(size_t) wpos] = d[i] + fbLP[c] * fb;
                d[i] = d[i] + dl * dMix;
            }
            wpos = (wpos + 1) % sz;
        }
    }

    // ---- 5. Reverb -----------------------------------------------------------------
    {
        juce::dsp::Reverb::Parameters rp;
        rp.roomSize = P ("revSize");
        rp.damping = 0.5f;
        rp.wetLevel = P ("revMix") * 0.5f;
        rp.dryLevel = 1.f;
        rp.width = 1.f;
        rev.setParameters (rp);

        auto block = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) nCh);
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        rev.process (ctx);
    }

    // ---- 6. Output gain (smoothed) ---------------------------------------------------
    const float outLin = Decibels::decibelsToGain (P ("out"));
    buffer.applyGainRamp (0, n, lastOut, outLin);
    lastOut = outLin;

    // Feed the GUI meters
    for (int c = 0; c < 2; ++c)
        meterPeak[c].store (buffer.getMagnitude (juce::jmin (c, nCh - 1), 0, n));
    gateOpen.store (gGain);
}

//==============================================================================
juce::AudioProcessorEditor* AuroraProcessor::createEditor() { return new AuroraEditor (*this); }

void AuroraProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    state.setProperty ("irPath", irPath, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, dest);
}

void AuroraProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
    {
        auto state = juce::ValueTree::fromXml (*xml);
        if (state.isValid())
        {
            apvts.replaceState (state);
            const juce::File f (state.getProperty ("irPath").toString());
            if (f.existsAsFile())
                loadIR (f);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AuroraProcessor(); }
