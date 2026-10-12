#pragma once
#include "PluginProcessor.h"
#include <functional>
#include <vector>

//==============================================================================
// Palette
//==============================================================================
namespace pal
{
    inline const juce::Colour bgTop   { 0xff060d12 };
    inline const juce::Colour bgBot   { 0xff0e1b23 };
    inline const juce::Colour teal    { 0xff3ee6b5 };
    inline const juce::Colour violet  { 0xff8b7cff };
    inline const juce::Colour sky     { 0xff2fb8ff };
    inline const juce::Colour amber   { 0xffffb454 };
    inline const juce::Colour coral   { 0xffff5d6c };
    inline const juce::Colour text    { 0xffe6eef0 };
    inline const juce::Colour dim     { 0xff8fa4ad };
}

inline juce::Font uiFont (float h, int style = juce::Font::plain)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultSansSerifFontName(), h, style));
}

//==============================================================================
// Look and feel: glowing knobs, pill switches, outlined buttons
//==============================================================================
class AuroraLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AuroraLookAndFeel()
    {
        setColour (juce::TextButton::textColourOffId, pal::text);
        setColour (juce::TextButton::textColourOnId, pal::text);
        setColour (juce::Label::textColourId, pal::text);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return uiFont (14.f, juce::Font::bold); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider& s) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.f);
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float arcW = juce::jmax (3.f, r * 0.09f);
        const float arcR = r - arcW;
        const float toA = startA + pos * (endA - startA);
        const bool hot = s.isMouseOverOrDragging();

        // Track
        juce::Path track;
        track.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startA, endA, true);
        g.setColour (juce::Colours::white.withAlpha (0.09f));
        g.strokePath (track, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // Value arc with glow
        if (pos > 0.002f)
        {
            juce::Path v;
            v.addCentredArc (c.x, c.y, arcR, arcR, 0.f, startA, toA, true);

            juce::ColourGradient glow (pal::teal.withAlpha (hot ? 0.34f : 0.22f), c.x - arcR, c.y,
                                       pal::violet.withAlpha (hot ? 0.34f : 0.22f), c.x + arcR, c.y, false);
            g.setGradientFill (glow);
            g.strokePath (v, juce::PathStrokeType (arcW * 2.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            juce::ColourGradient grad (pal::teal, c.x - arcR, c.y, pal::violet, c.x + arcR, c.y, false);
            g.setGradientFill (grad);
            g.strokePath (v, juce::PathStrokeType (arcW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }

        // Knob body
        const float bodyR = arcR - arcW * 2.2f;
        juce::Rectangle<float> body (c.x - bodyR, c.y - bodyR, bodyR * 2.f, bodyR * 2.f);

        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillEllipse (body.translated (0.f, 3.f));

        juce::ColourGradient bg (juce::Colour (0xff35464f), c.x - bodyR * 0.3f, c.y - bodyR * 0.4f,
                                 juce::Colour (0xff0b141a), c.x - bodyR * 0.3f + bodyR * 1.5f, c.y - bodyR * 0.4f, true);
        g.setGradientFill (bg);
        g.fillEllipse (body);
        g.setColour (hot ? pal::teal.withAlpha (0.55f) : juce::Colours::white.withAlpha (0.14f));
        g.drawEllipse (body, 1.f);

        // Inner cap
        const float capR = bodyR * 0.62f;
        juce::Rectangle<float> cap (c.x - capR, c.y - capR, capR * 2.f, capR * 2.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c2a33), c.x, c.y - capR,
                                                 juce::Colour (0xff0d161c), c.x, c.y + capR, false));
        g.fillEllipse (cap);
        g.setColour (juce::Colours::white.withAlpha (0.07f));
        g.drawEllipse (cap, 1.f);

        // Pointer
        juce::Path p;
        p.startNewSubPath (c.x + std::sin (toA) * bodyR * 0.42f, c.y - std::cos (toA) * bodyR * 0.42f);
        p.lineTo          (c.x + std::sin (toA) * bodyR * 0.92f, c.y - std::cos (toA) * bodyR * 0.92f);
        g.setColour (pal::text);
        g.strokePath (p, juce::PathStrokeType (juce::jmax (2.f, r * 0.06f),
                                               juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hl, bool) override
    {
        auto area = b.getLocalBounds().toFloat();
        const bool on = b.getToggleState();
        const float ph = 22.f, pw = 40.f;
        juce::Rectangle<float> pill (area.getX() + 2.f, area.getCentreY() - ph * 0.5f, pw, ph);

        if (on)
        {
            g.setColour (pal::teal.withAlpha (0.22f));
            g.fillRoundedRectangle (pill.expanded (3.f), ph * 0.5f + 3.f);
            g.setGradientFill (juce::ColourGradient (pal::teal, pill.getX(), 0.f, pal::violet, pill.getRight(), 0.f, false));
        }
        else
            g.setColour (juce::Colours::white.withAlpha (hl ? 0.18f : 0.12f));

        g.fillRoundedRectangle (pill, ph * 0.5f);

        const float d = ph - 6.f;
        const float tx = on ? pill.getRight() - d - 3.f : pill.getX() + 3.f;
        g.setColour (on ? juce::Colours::white : pal::dim);
        g.fillEllipse (tx, pill.getY() + 3.f, d, d);

        g.setColour (on ? pal::text : pal::dim);
        g.setFont (uiFont (13.f));
        g.drawText (b.getButtonText(), (int) pill.getRight() + 8, 0, b.getWidth() - (int) pill.getRight() - 8,
                    b.getHeight(), juce::Justification::centredLeft);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.f);
        g.setColour (juce::Colours::white.withAlpha (down ? 0.20f : hl ? 0.13f : 0.06f));
        g.fillRoundedRectangle (r, 10.f);
        g.setGradientFill (juce::ColourGradient (pal::teal.withAlpha (0.9f), r.getX(), 0.f,
                                                 pal::violet.withAlpha (0.9f), r.getRight(), 0.f, false));
        g.drawRoundedRectangle (r, 10.f, 1.4f);
    }
};

//==============================================================================
// Animated aurora background + glass panels + title
//==============================================================================
class AuroraBackground : public juce::Component, private juce::Timer
{
public:
    struct Panel { juce::Rectangle<int> r; juce::String title; };

    AuroraBackground() { setOpaque (true); setInterceptsMouseClicks (false, false); startTimerHz (24); }

    void setPanels (std::vector<Panel> p) { panels = std::move (p); repaint(); }

    void paint (juce::Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (pal::bgTop, 0.f, 0.f, pal::bgBot, 0.f, b.getHeight(), false));
        g.fillAll();

        // Stars
        juce::Random rnd (7);
        for (int i = 0; i < 70; ++i)
        {
            const float sx = rnd.nextFloat() * b.getWidth(), sy = rnd.nextFloat() * 240.f;
            const float sz = 0.7f + rnd.nextFloat() * 1.1f;
            g.setColour (juce::Colours::white.withAlpha (0.12f + rnd.nextFloat() * 0.4f));
            g.fillEllipse (sx, sy, sz, sz);
        }

        // Aurora curtains
        curtain (g, b.getWidth(), 78.f, 22.f, 0.0060f, phase * 0.6f,       240.f, pal::teal,   0.30f);
        curtain (g, b.getWidth(), 48.f, 30.f, 0.0045f, phase * 0.4f + 2.f, 260.f, pal::violet, 0.26f);
        curtain (g, b.getWidth(), 108.f, 16.f, 0.0090f, -phase * 0.5f,     200.f, pal::sky,    0.13f);

        // Title
        g.setColour (pal::text);
        g.setFont (uiFont (36.f).withExtraKerningFactor (0.06f));
        g.drawText ("Aurora", 30, 14, 300, 44, juce::Justification::centredLeft);
        g.setColour (pal::dim);
        g.setFont (uiFont (13.f).withExtraKerningFactor (0.04f));
        g.drawText ("Guitar studio", 33, 56, 300, 18, juce::Justification::centredLeft);

        // Glass panels
        for (auto& p : panels)
        {
            auto r = p.r.toFloat();
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.075f), 0.f, r.getY(),
                                                     juce::Colours::white.withAlpha (0.025f), 0.f, r.getBottom(), false));
            g.fillRoundedRectangle (r, 16.f);
            g.setColour (juce::Colours::white.withAlpha (0.10f));
            g.drawRoundedRectangle (r.reduced (0.5f), 16.f, 1.f);

            g.setColour (pal::dim);
            g.setFont (uiFont (14.f, juce::Font::bold));
            g.drawText (p.title, p.r.getX() + 18, p.r.getY() + 10, 160, 22, juce::Justification::centredLeft);
        }
    }

private:
    void curtain (juce::Graphics& g, float w, float y0, float amp, float freq, float ph,
                  float height, juce::Colour c, float alpha)
    {
        auto edge = [&] (float x) { return y0 + amp * std::sin (x * freq + ph) + amp * 0.4f * std::sin (x * freq * 2.3f - ph * 1.3f); };

        juce::Path p;
        p.startNewSubPath (0.f, edge (0.f));
        for (float x = 8.f; x <= w; x += 8.f)
            p.lineTo (x, edge (x));
        p.lineTo (w, edge (w) + height);
        p.lineTo (0.f, edge (0.f) + height);
        p.closeSubPath();

        g.setGradientFill (juce::ColourGradient (c.withAlpha (alpha), 0.f, y0 - amp,
                                                 c.withAlpha (0.f), 0.f, y0 + height, false));
        g.fillPath (p);
    }

    void timerCallback() override
    {
        phase += 0.045f;
        if (isShowing()) repaint();
    }

    std::vector<Panel> panels;
    float phase = 0.f;
};

//==============================================================================
// Knob = name + rotary slider + value read-out
//==============================================================================
class Knob : public juce::Component
{
public:
    Knob (juce::AudioProcessorValueTreeState& apvts, const juce::String& id, const juce::String& name,
          const juce::String& suffix = {}, int decimals = 1, double scale = 1.0)
        : attach (apvts, id, slider), nameText (name), suffixText (suffix), dec (decimals), scl (scale)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f, true);
        slider.onValueChange = [this] { repaint(); };
        slider.addMouseListener (this, false);
        addAndMakeVisible (slider);
    }

    void resized() override
    {
        slider.setBounds (getLocalBounds().withTrimmedTop (18).withTrimmedBottom (20));
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (pal::dim);
        g.setFont (uiFont (13.f));
        g.drawText (nameText, 0, 0, getWidth(), 18, juce::Justification::centred);

        g.setColour (slider.isMouseOverOrDragging() ? pal::text : pal::dim.brighter (0.2f));
        g.setFont (uiFont (13.f, juce::Font::bold));
        g.drawText (juce::String (slider.getValue() * scl, dec) + suffixText,
                    0, getHeight() - 20, getWidth(), 20, juce::Justification::centred);
    }

    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { repaint(); }

private:
    juce::Slider slider;
    juce::AudioProcessorValueTreeState::SliderAttachment attach;
    juce::String nameText, suffixText;
    int dec;
    double scl;
};

//==============================================================================
// Output level meter (stereo) and gate LED
//==============================================================================
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter (AuroraProcessor& p) : proc (p) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        const float barW = (b.getWidth() - 8.f) * 0.5f;

        for (int ch = 0; ch < 2; ++ch)
        {
            juce::Rectangle<float> bar (b.getX() + (float) ch * (barW + 8.f), b.getY(), barW, b.getHeight());
            g.setColour (juce::Colours::white.withAlpha (0.07f));
            g.fillRoundedRectangle (bar, 4.f);

            const float db = juce::jlimit (-60.f, 0.f, juce::Decibels::gainToDecibels (lvl[ch], -60.f));
            const float frac = (db + 60.f) / 60.f;
            if (frac < 0.01f) continue;

            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip;
            clip.addRoundedRectangle (bar, 4.f);
            g.reduceClipRegion (clip);
            g.reduceClipRegion (bar.withTrimmedTop (bar.getHeight() * (1.f - frac)).toNearestInt());

            juce::ColourGradient grad (pal::coral, 0.f, bar.getY(), pal::teal, 0.f, bar.getBottom(), false);
            grad.addColour (0.08, pal::coral);
            grad.addColour (0.22, pal::amber);
            g.setGradientFill (grad);
            g.fillRect (bar);
        }
    }

private:
    void timerCallback() override
    {
        for (int ch = 0; ch < 2; ++ch)
            lvl[ch] = juce::jmax (proc.meterPeak[ch].load(), lvl[ch] * 0.86f);
        repaint();
    }

    AuroraProcessor& proc;
    float lvl[2] { 0.f, 0.f };
};

class Led : public juce::Component, private juce::Timer
{
public:
    explicit Led (std::function<bool()> source) : src (std::move (source)) { startTimerHz (30); }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat().reduced (3.f);
        if (on)
        {
            g.setColour (pal::teal.withAlpha (0.28f));
            g.fillEllipse (b.expanded (3.f));
        }
        g.setColour (on ? pal::teal : juce::Colours::white.withAlpha (0.14f));
        g.fillEllipse (b);
    }

private:
    void timerCallback() override
    {
        const bool now = src();
        if (now != on) { on = now; repaint(); }
    }

    std::function<bool()> src;
    bool on = false;
};

//==============================================================================
// Content: fixed 1100x620 layout (the editor scales it when resized)
//==============================================================================
class AuroraContent : public juce::Component
{
public:
    static constexpr int W = 1100, H = 620;

    explicit AuroraContent (AuroraProcessor& p)
        : proc (p),
          kIn (p.apvts, "in", "Input", " dB"),
          kGate (p.apvts, "gate", "Threshold", " dB", 0),
          kDrive (p.apvts, "drive", "Drive"),
          kPedLevel (p.apvts, "pedLevel", "Level", " dB"),
          kGain (p.apvts, "gain", "Gain"),
          kMaster (p.apvts, "master", "Master"),
          kSag (p.apvts, "sag", "Sag", "%", 0, 100.0),
          kBass (p.apvts, "bass", "Bass"),
          kMid (p.apvts, "mid", "Mid"),
          kTreble (p.apvts, "treble", "Treble"),
          kPres (p.apvts, "presence", "Presence"),
          kDelayTime (p.apvts, "delayTime", "Time", " ms", 0),
          kDelayFb (p.apvts, "delayFb", "Feedback", "%", 0, 100.0),
          kDelayMix (p.apvts, "delayMix", "Mix", "%", 0, 100.0),
          kRevSize (p.apvts, "revSize", "Size", "%", 0, 100.0),
          kRevMix (p.apvts, "revMix", "Mix", "%", 0, 100.0),
          kOut (p.apvts, "out", "Output", " dB"),
          meter (p),
          gateLed ([&p] { return p.gateOpen.load() > 0.5f; }),
          pedalAtt (p.apvts, "pedalOn", pedalToggle),
          cabAtt (p.apvts, "cabOn", cabToggle)
    {
        addAndMakeVisible (bg);
        for (auto* k : { &kIn, &kGate, &kDrive, &kPedLevel, &kGain, &kMaster, &kSag, &kBass, &kMid, &kTreble,
                         &kPres, &kDelayTime, &kDelayFb, &kDelayMix, &kRevSize, &kRevMix, &kOut })
            addAndMakeVisible (*k);

        addAndMakeVisible (meter);
        addAndMakeVisible (gateLed);

        pedalToggle.setButtonText ("On");
        cabToggle.setButtonText ("On");
        addAndMakeVisible (pedalToggle);
        addAndMakeVisible (cabToggle);

        irName.setFont (uiFont (22.f));
        irName.setColour (juce::Label::textColourId, pal::text);
        irHint.setFont (uiFont (13.f));
        irHint.setColour (juce::Label::textColourId, pal::dim);
        irName.setInterceptsMouseClicks (false, false);
        irHint.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (irName);
        addAndMakeVisible (irHint);

        irButton.setButtonText ("Load impulse response");
        irButton.onClick = [this]
        {
            chooser = std::make_unique<juce::FileChooser> ("Choose a cabinet impulse response",
                                                           juce::File {}, "*.wav;*.aif;*.aiff;*.flac");
            chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                                  [this] (const juce::FileChooser& fc)
                                  {
                                      auto f = fc.getResult();
                                      if (f.existsAsFile())
                                      {
                                          proc.loadIR (f);
                                          updateIR();
                                      }
                                  });
        };
        addAndMakeVisible (irButton);
        updateIR();

        setSize (W, H);
    }

    void resized() override
    {
        bg.setBounds (getLocalBounds());

        const int M = 20, G = 14, yA = 92, hA = 176, hB = 320;
        const int yB = yA + hA + G;

        juce::Rectangle<int> gateR (M, yA, 210, hA);
        juce::Rectangle<int> pedR  (gateR.getRight() + G, yA, 210, hA);
        juce::Rectangle<int> cabR  (pedR.getRight() + G, yA, 370, hA);
        juce::Rectangle<int> outR  (cabR.getRight() + G, yA, 228, hA);
        juce::Rectangle<int> ampR  (M, yB, 580, hB);
        juce::Rectangle<int> delR  (ampR.getRight() + G, yB, 240, hB);
        juce::Rectangle<int> revR  (delR.getRight() + G, yB, 212, hB);

        bg.setPanels ({ { gateR, "Gate" }, { pedR, "Pedal" }, { cabR, "Cabinet" }, { outR, "Output" },
                        { ampR, "Amp" }, { delR, "Delay" }, { revR, "Reverb" } });

        // Row A
        auto two = [] (juce::Rectangle<int> r, Knob& a, Knob& b)
        {
            auto body = r.withTrimmedTop (38).reduced (10, 6);
            a.setBounds (body.removeFromLeft (body.getWidth() / 2));
            b.setBounds (body);
        };
        two (gateR, kIn, kGate);
        two (pedR, kDrive, kPedLevel);
        gateLed.setBounds (gateR.getRight() - 34, gateR.getY() + 12, 18, 18);
        pedalToggle.setBounds (pedR.getRight() - 84, pedR.getY() + 9, 72, 24);
        cabToggle.setBounds (cabR.getRight() - 84, cabR.getY() + 9, 72, 24);

        auto cab = cabR.withTrimmedTop (50).reduced (22, 0);
        irName.setBounds (cab.getX(), cab.getY(), cab.getWidth(), 30);
        irHint.setBounds (cab.getX(), cab.getY() + 32, cab.getWidth(), 20);
        irButton.setBounds (cab.getX(), cab.getY() + 70, 190, 36);

        meter.setBounds (outR.getX() + 30, outR.getY() + 46, 46, 118);
        kOut.setBounds (outR.getX() + 92, outR.getY() + 34, 128, 136);

        // Row B
        const int topH = 152, botH = 126;

        auto a = ampR.withTrimmedTop (36).reduced (14, 0);
        auto aTop = a.removeFromTop (topH);
        const int w3 = aTop.getWidth() / 3;
        kGain.setBounds (aTop.removeFromLeft (w3));
        kMaster.setBounds (aTop.removeFromLeft (w3));
        kSag.setBounds (aTop);
        auto aBot = a.removeFromTop (botH);
        const int w4 = aBot.getWidth() / 4;
        kBass.setBounds (aBot.removeFromLeft (w4));
        kMid.setBounds (aBot.removeFromLeft (w4));
        kTreble.setBounds (aBot.removeFromLeft (w4));
        kPres.setBounds (aBot);

        auto d = delR.withTrimmedTop (36).reduced (14, 0);
        kDelayTime.setBounds (d.removeFromTop (topH));
        auto dBot = d.removeFromTop (botH);
        kDelayFb.setBounds (dBot.removeFromLeft (dBot.getWidth() / 2));
        kDelayMix.setBounds (dBot);

        auto r = revR.withTrimmedTop (36).reduced (14, 0);
        kRevSize.setBounds (r.removeFromTop (topH));
        kRevMix.setBounds (r.removeFromTop (botH));
    }

private:
    void updateIR()
    {
        irName.setText (proc.getIRName(), juce::dontSendNotification);
        irHint.setText (proc.hasCustomIR() ? "Custom impulse response"
                                           : "Load a .wav impulse response for a real cabinet",
                        juce::dontSendNotification);
    }

    AuroraProcessor& proc;
    AuroraBackground bg;
    Knob kIn, kGate, kDrive, kPedLevel, kGain, kMaster, kSag, kBass, kMid, kTreble, kPres,
         kDelayTime, kDelayFb, kDelayMix, kRevSize, kRevMix, kOut;
    LevelMeter meter;
    Led gateLed;
    juce::ToggleButton pedalToggle, cabToggle;
    juce::AudioProcessorValueTreeState::ButtonAttachment pedalAtt, cabAtt;
    juce::Label irName, irHint;
    juce::TextButton irButton;
    std::unique_ptr<juce::FileChooser> chooser;
};

//==============================================================================
class AuroraEditor : public juce::AudioProcessorEditor
{
public:
    explicit AuroraEditor (AuroraProcessor& p) : AudioProcessorEditor (p), content (p)
    {
        setLookAndFeel (&laf);
        addAndMakeVisible (content);
        setResizable (true, true);
        setResizeLimits (715, 403, 1650, 930);
        getConstrainer()->setFixedAspectRatio ((double) AuroraContent::W / AuroraContent::H);
        setSize (AuroraContent::W, AuroraContent::H);
    }

    ~AuroraEditor() override { setLookAndFeel (nullptr); }

    void resized() override
    {
        content.setBounds (0, 0, AuroraContent::W, AuroraContent::H);
        content.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) AuroraContent::W));
    }

private:
    AuroraLookAndFeel laf;   // must outlive `content`
    AuroraContent content;
};
