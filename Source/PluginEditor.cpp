#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

// ============================================================================
//  Fonts
// ============================================================================
static Typeface::Ptr titleFace()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::ShrikhandRegular_ttf, BinaryData::ShrikhandRegular_ttfSize);
    return t;
}
static Typeface::Ptr labelFace()
{
    static Typeface::Ptr t = Typeface::createSystemTypefaceFor (BinaryData::RighteousRegular_ttf, BinaryData::RighteousRegular_ttfSize);
    return t;
}
Font LunchboxLook::title (float h) { return Font (FontOptions (titleFace()).withHeight (h)); }
Font LunchboxLook::label (float h) { return Font (FontOptions (labelFace()).withHeight (h)); }

static Path textPath (const String& s, const Font& f, Rectangle<float> area, Justification j = Justification::centred)
{
    GlyphArrangement ga;
    ga.addLineOfText (f, s, 0, 0);
    Path p; ga.createPath (p);
    auto b = p.getBounds();
    float x = area.getX(), y = area.getCentreY() - b.getCentreY();
    if (j.testFlags (Justification::horizontallyCentred)) x = area.getCentreX() - b.getCentreX();
    else if (j.testFlags (Justification::right)) x = area.getRight() - b.getRight();
    else x = area.getX() - b.getX();
    p.applyTransform (AffineTransform::translation (x, y));
    return p;
}

// ============================================================================
//  Look and feel
// ============================================================================
LunchboxLook::LunchboxLook()
{
    setDefaultSansSerifTypeface (labelFace());
    setColour (PopupMenu::backgroundColourId, pal::paper);
    setColour (PopupMenu::textColourId, pal::brown);
    setColour (PopupMenu::highlightedBackgroundColourId, pal::orange);
    setColour (PopupMenu::highlightedTextColourId, pal::paper);
    setColour (ComboBox::textColourId, pal::brown);
    setColour (Label::textColourId, pal::brown);
    setColour (TextEditor::backgroundColourId, pal::paper);
    setColour (TextEditor::textColourId, pal::brown);
    setColour (TextEditor::highlightColourId, pal::gold.withAlpha (0.5f));
    setColour (TextEditor::outlineColourId, pal::brown);
    setColour (TextEditor::focusedOutlineColourId, pal::orange);
    setColour (CaretComponent::caretColourId, pal::brown);
}

Label* LunchboxLook::createSliderTextBox (Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (label (14.0f));
    return l;
}

void LunchboxLook::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                     float a0, float a1, Slider& s)
{
    const bool big = (bool) s.getProperties()["big"];
    const Colour accent = Colour ((uint32) (int64) s.getProperties()["accent"]);
    const Colour textInk = s.getProperties().contains ("ink") ? Colour ((uint32) (int64) s.getProperties()["ink"]) : pal::brown;
    const Colour ink = pal::brown;
    const float textH = big ? 40.0f : 32.0f;

    auto area = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    auto textArea = area.removeFromBottom (textH);
    const float rad = jmin (area.getWidth(), area.getHeight()) * 0.5f - 4.0f;
    const auto c = area.getCentre();
    const float ang = a0 + pos * (a1 - a0);

    // value arc track
    {
        Path track; track.addCentredArc (c.x, c.y, rad, rad, 0, a0, a1, true);
        g.setColour (textInk.withAlpha (0.18f));
        g.strokePath (track, PathStrokeType (big ? 7.0f : 5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        Path val; val.addCentredArc (c.x, c.y, rad, rad, 0, a0, ang, true);
        g.setColour (accent);
        g.strokePath (val, PathStrokeType (big ? 7.0f : 5.0f, PathStrokeType::curved, PathStrokeType::rounded));
    }

    const float kr = rad * (big ? 0.80f : 0.74f);

    // drop shadow
    g.setColour (Colours::black.withAlpha (0.28f));
    g.fillEllipse (c.x - kr + 2, c.y - kr + 4, kr * 2, kr * 2);

    if (big)
    {
        // flower-power knob: petals rotate with the value
        Path petals;
        const int np = 10;
        for (int i = 0; i < np; ++i)
        {
            float a = ang + MathConstants<float>::twoPi * (float) i / (float) np;
            Path p; p.addEllipse (-kr * 0.30f, -kr * 1.0f, kr * 0.60f, kr * 0.95f);
            p.applyTransform (AffineTransform::rotation (a).translated (c.x, c.y));
            petals.addPath (p);
        }
        g.setColour (pal::paper); g.fillPath (petals);
        g.setColour (pal::brown); g.strokePath (petals, PathStrokeType (2.0f));
        float cr = kr * 0.52f;
        ColourGradient cg (pal::gold.brighter (0.3f), c.x - cr * 0.4f, c.y - cr * 0.5f, pal::orange, c.x + cr, c.y + cr, true);
        g.setGradientFill (cg); g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        g.setColour (pal::brown); g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 2.5f);
        // pointer
        Point<float> tip (c.x + std::sin (ang) * cr * 0.85f, c.y - std::cos (ang) * cr * 0.85f);
        g.setColour (pal::brown);
        g.drawLine ({ c, tip }, 5.0f);
        g.fillEllipse (tip.x - 4, tip.y - 4, 8, 8);
    }
    else
    {
        // chunky 70s plastic knob: cream skirt + coloured cap
        g.setColour (pal::paper); g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        g.setColour (ink); g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 2.2f);
        // skirt ticks
        for (int i = 0; i <= 10; ++i)
        {
            float a = a0 + (a1 - a0) * (float) i / 10.0f;
            Point<float> p1 (c.x + std::sin (a) * kr * 0.80f, c.y - std::cos (a) * kr * 0.80f);
            Point<float> p2 (c.x + std::sin (a) * kr * 0.94f, c.y - std::cos (a) * kr * 0.94f);
            g.setColour (ink.withAlpha (0.55f)); g.drawLine ({ p1, p2 }, 1.4f);
        }
        float cr = kr * 0.66f;
        ColourGradient cg (accent.brighter (0.35f), c.x - cr * 0.5f, c.y - cr * 0.6f, accent.darker (0.25f), c.x + cr, c.y + cr, true);
        g.setGradientFill (cg); g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        g.setColour (ink); g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 2.0f);
        g.setColour (Colours::white.withAlpha (0.35f));
        g.fillEllipse (c.x - cr * 0.55f, c.y - cr * 0.7f, cr * 0.7f, cr * 0.4f);
        Point<float> tip (c.x + std::sin (ang) * kr * 0.92f, c.y - std::cos (ang) * kr * 0.92f);
        Point<float> base (c.x + std::sin (ang) * cr * 0.25f, c.y - std::cos (ang) * cr * 0.25f);
        g.setColour (ink); g.drawLine ({ base, tip }, 3.6f);
    }

    // name + value
    g.setColour (textInk);
    g.setFont (label (big ? 22.0f : 15.0f));
    g.drawText (s.getName().toUpperCase(), textArea.removeFromTop (textH * 0.52f), Justification::centred, false);
    g.setColour (textInk.withAlpha (0.75f));
    g.setFont (label (big ? 15.0f : 12.5f));
    g.drawText (s.getTextFromValue (s.getValue()), textArea, Justification::centredTop, false);
}

void LunchboxLook::drawToggleButton (Graphics& g, ToggleButton& b, bool hi, bool)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();

    if (b.getProperties().contains ("leftLabel"))
    {
        // slide switch: SERIES | PARALLEL, TEMPO | PICK
        g.setColour (pal::darkBrown); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        auto half = r.reduced (4);
        auto thumb = on ? half.withTrimmedLeft (half.getWidth() * 0.5f) : half.withTrimmedRight (half.getWidth() * 0.5f);
        g.setColour (pal::gold); g.fillRoundedRectangle (thumb, thumb.getHeight() * 0.5f);
        g.setColour (pal::brown); g.drawRoundedRectangle (thumb, thumb.getHeight() * 0.5f, 1.5f);
        g.setFont (label (r.getHeight() < 30 ? 12.5f : 14.0f));
        auto left = half.withTrimmedRight (half.getWidth() * 0.5f), right = half.withTrimmedLeft (half.getWidth() * 0.5f);
        g.setColour (on ? pal::cream.withAlpha (0.6f) : pal::brown); g.drawText (b.getProperties()["leftLabel"].toString(), left, Justification::centred);
        g.setColour (on ? pal::brown : pal::cream.withAlpha (0.6f)); g.drawText (b.getProperties()["rightLabel"].toString(), right, Justification::centred);
        return;
    }

    // pill power switch
    const Colour accent = Colour ((uint32) (int64) b.getProperties()["accent"]);
    g.setColour (Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (r.translated (0, 2), r.getHeight() * 0.5f);
    g.setColour (on ? pal::paper : pal::darkBrown.withAlpha (0.55f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (pal::brown); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 2.0f);
    auto dot = r.withWidth (r.getHeight()).reduced (4).translated (on ? r.getWidth() - r.getHeight() : 0.0f, 0.0f);
    g.setColour (on ? accent.brighter (0.2f) : pal::cream.withAlpha (0.35f));
    g.fillEllipse (dot);
    g.setColour (pal::brown); g.drawEllipse (dot, 1.5f);
    g.setFont (label (13.0f));
    g.setColour (on ? pal::brown : pal::cream.withAlpha (hi ? 0.95f : 0.75f));
    auto txt = on ? r.withTrimmedRight (r.getHeight() - 2) : r.withTrimmedLeft (r.getHeight() - 2);
    g.drawText (on ? "ON" : "OFF", txt, Justification::centred);
}

void LunchboxLook::drawComboBox (Graphics& g, int w, int h, bool, int, int, int, int, ComboBox&)
{
    auto r = Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.5f);
    g.setColour (Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (r.translated (0, 2), 9.0f);
    g.setColour (pal::paper); g.fillRoundedRectangle (r, 9.0f);
    g.setColour (pal::brown); g.drawRoundedRectangle (r, 9.0f, 2.0f);
    Path tri; float ax = r.getRight() - 20, ay = r.getCentreY();
    tri.addTriangle (ax - 6, ay - 3, ax + 6, ay - 3, ax, ay + 5);
    g.setColour (pal::orange); g.fillPath (tri);
}

void LunchboxLook::positionComboBoxText (ComboBox& box, Label& l)
{
    l.setBounds (12, 1, box.getWidth() - 40, box.getHeight() - 2);
    l.setFont (label (17.0f));
}

// ============================================================================
//  Editor
// ============================================================================
static constexpr int W = 980, H = 690;

ScoobyGroovesEditor::Knob& ScoobyGroovesEditor::addKnob (const char* id, const String& name, Colour accent, bool big)
{
    auto k = std::make_unique<Knob>();
    k->s.setName (name);
    k->s.setComponentID (id);
    k->s.setSliderStyle (Slider::RotaryVerticalDrag);
    k->s.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
    k->s.setRotaryParameters (MathConstants<float>::pi * 1.22f, MathConstants<float>::pi * 2.78f, true);
    k->s.getProperties().set ("accent", (int64) accent.getARGB());
    k->s.getProperties().set ("big", big);
    k->s.setMouseDragSensitivity (big ? 260 : 200);
    k->s.setVelocityBasedMode (false);
    k->a = std::make_unique<SA> (proc.apvts, id, k->s);
    if (auto* prm = proc.apvts.getParameter (id))
        k->s.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    k->s.textFromValueFunction = [prm = proc.apvts.getParameter (id)] (double v)
    { return prm != nullptr ? prm->getText (prm->convertTo0to1 ((float) v), 16) : String (v); };
    addAndMakeVisible (k->s);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

ScoobyGroovesEditor::ScoobyGroovesEditor (ScoobyGroovesProcessor& p)
    : AudioProcessorEditor (p), proc (p)
{
    setLookAndFeel (&look);

    addKnob ("lofiBits", "Bits", pal::orange);
    addKnob ("lofiRate", "Rate", pal::orange);
    addKnob ("lofiTone", "Tone", pal::orange);
    addKnob ("lofiCrackle", "Dust", pal::orange);
    addKnob ("lofiLevel", "Blend", pal::orange);

    addKnob ("tapeWow", "Wow", pal::teal);
    addKnob ("tapeFlutter", "Flutter", pal::teal);
    addKnob ("tapeHiss", "Hiss", pal::teal);
    addKnob ("tapeAge", "Age", pal::teal);
    addKnob ("tapeLevel", "Blend", pal::teal);

    addKnob ("tubeDrive", "Drive", pal::avocado);
    addKnob ("tubeBias", "Bias", pal::avocado);
    addKnob ("tubeWarmth", "Warmth", pal::avocado);
    addKnob ("tubeSag", "Sag", pal::avocado);
    addKnob ("tubeLevel", "Blend", pal::avocado);

    addKnob ("glitch", "Glitch", pal::red, true).s.getProperties().set ("ink", (int64) pal::cream.getARGB());
    addKnob ("mix", "Mix", pal::gold).s.getProperties().set ("ink", (int64) pal::cream.getARGB());
    addKnob ("output", "Output", pal::gold).s.getProperties().set ("ink", (int64) pal::cream.getARGB());

    auto setupToggle = [&] (ToggleButton& b, const char* id, Colour accent)
    {
        b.setComponentID (id);
        b.getProperties().set ("accent", (int64) accent.getARGB());
        addAndMakeVisible (b);
        buttonAtts.push_back (std::make_unique<BA> (proc.apvts, id, b));
    };
    setupToggle (lofiOn, "lofiOn", pal::orange);
    setupToggle (tapeOn, "tapeOn", pal::teal);
    setupToggle (tubeOn, "tubeOn", pal::avocado);
    setupToggle (routing, "routing", pal::gold);
    setupToggle (glitchSync, "glitchSync", pal::gold);
    routing.getProperties().set ("leftLabel", "SERIES");    routing.getProperties().set ("rightLabel", "PARALLEL");
    glitchSync.getProperties().set ("leftLabel", "TEMPO");  glitchSync.getProperties().set ("rightLabel", "PICK");

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);

    setSize (W, H);
    startTimerHz (30);
}

ScoobyGroovesEditor::~ScoobyGroovesEditor() { setLookAndFeel (nullptr); }

void ScoobyGroovesEditor::timerCallback()
{
    float target = proc.glitchLamp.load();
    float nl = target > lamp ? target : lamp * 0.8f;
    if (std::abs (nl - lamp) > 0.01f) { lamp = nl; repaint (glitchLampArea.expanded (12)); }
}

void ScoobyGroovesEditor::resized()
{
    cardLofi = { 46, 214, 286, 254 };
    cardTape = { 347, 214, 286, 254 };
    cardTube = { 648, 214, 286, 254 };
    deck     = { 46, 484, 888, 176 };

    auto find = [&] (const char* id) -> Slider& { for (auto& k : knobs) if (k->s.getComponentID() == id) return k->s; return knobs.front()->s; };
    auto layoutCard = [&] (Rectangle<int> c, const char* a, const char* b, const char* cc, const char* d, const char* lvl, ToggleButton& on)
    {
        on.setBounds (c.getRight() - 74, c.getY() + 9, 62, 26);
        auto body = c.withTrimmedTop (48).reduced (6, 2);
        auto left = body.removeFromLeft (178);
        auto row1 = left.removeFromTop (left.getHeight() / 2);
        find (a).setBounds (row1.removeFromLeft (89));
        find (b).setBounds (row1);
        find (cc).setBounds (left.removeFromLeft (89));
        find (d).setBounds (left);
        find (lvl).setBounds (body.withTrimmedTop (24).withTrimmedBottom (24));
    };
    layoutCard (cardLofi, "lofiBits", "lofiRate", "lofiTone", "lofiCrackle", "lofiLevel", lofiOn);
    layoutCard (cardTape, "tapeWow", "tapeFlutter", "tapeHiss", "tapeAge", "tapeLevel", tapeOn);
    layoutCard (cardTube, "tubeDrive", "tubeBias", "tubeWarmth", "tubeSag", "tubeLevel", tubeOn);

    // bottom deck
    routing.setBounds (deck.getX() + 26, deck.getY() + 46, 210, 34);
    presetBox.setBounds (deck.getX() + 26, deck.getY() + 118, 236, 34);
    find ("glitch").setBounds (deck.getCentreX() - 92, deck.getY() + 2, 184, 172);
    glitchLampArea = { deck.getCentreX() + 118, deck.getY() + 30, 22, 22 };
    glitchSync.setBounds (deck.getCentreX() + 80, deck.getY() + 112, 108, 28);
    find ("mix").setBounds (deck.getRight() - 236, deck.getY() + 26, 104, 136);
    find ("output").setBounds (deck.getRight() - 124, deck.getY() + 26, 104, 136);
}

// ----------------------------------------------------------------------------
//  Painting helpers
// ----------------------------------------------------------------------------
static void daisy (Graphics& g, Point<float> c, float r, Colour petal, Colour centre, float rot = 0)
{
    Path p;
    for (int i = 0; i < 8; ++i)
    {
        Path e; e.addEllipse (-r * 0.28f, -r, r * 0.56f, r * 0.95f);
        e.applyTransform (AffineTransform::rotation (rot + MathConstants<float>::twoPi * i / 8.0f).translated (c));
        p.addPath (e);
    }
    g.setColour (petal); g.fillPath (p);
    g.setColour (pal::brown); g.strokePath (p, PathStrokeType (1.6f));
    g.setColour (centre); g.fillEllipse (c.x - r * 0.33f, c.y - r * 0.33f, r * 0.66f, r * 0.66f);
    g.setColour (pal::brown); g.drawEllipse (c.x - r * 0.33f, c.y - r * 0.33f, r * 0.66f, r * 0.66f, 1.6f);
}

static void rivet (Graphics& g, Point<float> c, float r)
{
    ColourGradient cg (Colour (0xfff2efe6), c.x - r * 0.5f, c.y - r * 0.5f, Colour (0xff8a8578), c.x + r, c.y + r, true);
    g.setGradientFill (cg); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (Colour (0xff4a463e)); g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 1.0f);
}

static void icon (Graphics& g, int which, Rectangle<float> r, Colour ink)
{
    g.setColour (ink);
    if (which == 0)
    {
        // pixel staircase
        float s = r.getHeight() / 4.0f;
        for (int i = 0; i < 4; ++i)
            g.fillRect (r.getX() + i * s, r.getBottom() - (i + 1) * s, s - 1.0f, (i + 1) * s);
    }
    else if (which == 1)
    {
        // little cassette
        auto b = r.withSizeKeepingCentre (r.getHeight() * 1.5f, r.getHeight());
        g.drawRoundedRectangle (b, 3.0f, 2.0f);
        auto win = b.reduced (b.getWidth() * 0.18f, b.getHeight() * 0.28f).translated (0, -b.getHeight() * 0.06f);
        g.drawRoundedRectangle (win, 3.0f, 1.6f);
        float rr = win.getHeight() * 0.28f;
        g.fillEllipse (win.getX() + win.getWidth() * 0.25f - rr, win.getCentreY() - rr, rr * 2, rr * 2);
        g.fillEllipse (win.getX() + win.getWidth() * 0.75f - rr, win.getCentreY() - rr, rr * 2, rr * 2);
    }
    else
    {
        // vacuum tube
        auto b = r.withSizeKeepingCentre (r.getHeight() * 0.62f, r.getHeight());
        Path t; t.addRoundedRectangle (b.getX(), b.getY(), b.getWidth(), b.getHeight() * 0.78f, b.getWidth() * 0.5f, b.getWidth() * 0.5f, true, true, false, false);
        g.strokePath (t, PathStrokeType (2.0f));
        g.fillRect (b.getX() - 1, b.getY() + b.getHeight() * 0.78f, b.getWidth() + 2, b.getHeight() * 0.22f);
        g.drawLine (b.getCentreX(), b.getY() + b.getHeight() * 0.25f, b.getCentreX(), b.getY() + b.getHeight() * 0.7f, 1.6f);
        g.drawLine (b.getX() + 4, b.getY() + b.getHeight() * 0.45f, b.getRight() - 4, b.getY() + b.getHeight() * 0.45f, 1.6f);
    }
}

void ScoobyGroovesEditor::drawCard (Graphics& g, Rectangle<int> ri, const String& title, Colour accent, int ic)
{
    auto r = ri.toFloat();
    g.setColour (Colours::black.withAlpha (0.30f)); g.fillRoundedRectangle (r.translated (3, 5), 16.0f);
    g.setColour (pal::paper); g.fillRoundedRectangle (r, 16.0f);

    // header band
    auto head = r.withHeight (44.0f);
    Path hp; hp.addRoundedRectangle (head.getX(), head.getY(), head.getWidth(), head.getHeight(), 16.0f, 16.0f, true, true, false, false);
    g.setColour (accent); g.fillPath (hp);
    // stripe under header
    g.setColour (pal::gold); g.fillRect (r.getX(), head.getBottom(), r.getWidth(), 4.0f);
    g.setColour (pal::brown); g.drawLine (r.getX(), head.getBottom() + 4.0f, r.getRight(), head.getBottom() + 4.0f, 1.5f);

    icon (g, ic, Rectangle<float> (head.getX() + 14, head.getY() + 11, 24, 22), pal::paper);
    auto tp = textPath (title, title.length() > 6 ? LunchboxLook::title (24.0f) : LunchboxLook::title (26.0f),
                        head.withTrimmedLeft (52).withTrimmedRight (80), Justification::centredLeft);
    g.setColour (pal::brown); g.strokePath (tp, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (pal::paper); g.fillPath (tp);

    g.setColour (pal::brown); g.drawRoundedRectangle (r, 16.0f, 3.0f);

    // little divider between the four knobs and the blend knob
    g.setColour (accent.withAlpha (0.35f));
    float dx = r.getX() + 6 + 178 + 2;
    for (float yy = r.getY() + 60; yy < r.getBottom() - 12; yy += 9) g.fillEllipse (dx - 1.5f, yy, 3, 3);
}

void ScoobyGroovesEditor::paint (Graphics& g)
{
    auto full = getLocalBounds().toFloat();

    // tabletop behind the lunchbox
    g.setGradientFill (ColourGradient (Colour (0xff3a2414), 0, 0, Colour (0xff1c0f07), 0, full.getBottom(), false));
    g.fillAll();
    {
        Random rr (11);
        for (int i = 0; i < 70; ++i)
        {
            float y = rr.nextFloat() * full.getHeight();
            g.setColour (Colour (0xff000000).withAlpha (0.12f + 0.1f * rr.nextFloat()));
            g.drawLine (0, y, full.getWidth(), y + rr.nextFloat() * 6 - 3, 0.6f + rr.nextFloat() * 1.4f);
        }
    }

    // ---- handle
    {
        auto hx = full.getCentreX();
        Path handle;
        handle.startNewSubPath (hx - 150, 66);
        handle.cubicTo (hx - 150, 6, hx + 150, 6, hx + 150, 66);
        g.setColour (Colours::black.withAlpha (0.35f));
        g.strokePath (handle, PathStrokeType (22.0f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation (0, 5));
        g.setColour (pal::darkBrown);
        g.strokePath (handle, PathStrokeType (22.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (pal::orange);
        g.strokePath (handle, PathStrokeType (16.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white.withAlpha (0.25f));
        g.strokePath (handle, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation (0, -4));
        for (float sx : { hx - 150.0f, hx + 150.0f })
        {
            Rectangle<float> br (sx - 18, 52, 36, 18);
            g.setGradientFill (ColourGradient (Colour (0xffe8e4da), br.getX(), br.getY(), Colour (0xff8d887c), br.getX(), br.getBottom(), false));
            g.fillRoundedRectangle (br, 5.0f);
            g.setColour (Colour (0xff4a463e)); g.drawRoundedRectangle (br, 5.0f, 1.2f);
            rivet (g, br.getCentre(), 3.5f);
        }
    }

    // ---- lunchbox body (metal with rolled rim)
    auto body = Rectangle<float> (16, 62, full.getWidth() - 32, full.getHeight() - 72);
    g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (body.translated (0, 8), 30.0f);
    g.setGradientFill (ColourGradient (Colour (0xffeeeae0), body.getX(), body.getY(), Colour (0xff9a9488), body.getX(), body.getBottom(), false));
    g.fillRoundedRectangle (body, 30.0f);
    g.setColour (Colour (0xff57534a)); g.drawRoundedRectangle (body, 30.0f, 2.0f);

    auto face = body.reduced (12.0f);
    g.setColour (pal::orange); g.fillRoundedRectangle (face, 22.0f);

    // sunburst behind everything on the front
    {
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (face, 22.0f);
        g.reduceClipRegion (clip);
        Point<float> sc (face.getCentreX(), face.getY() + 112);
        const int rays = 28;
        for (int i = 0; i < rays; ++i)
        {
            float a0 = MathConstants<float>::twoPi * i / rays, a1 = MathConstants<float>::twoPi * (i + 0.5f) / rays;
            Path w; w.startNewSubPath (sc);
            w.lineTo (sc.x + std::sin (a0) * 1400, sc.y - std::cos (a0) * 1400);
            w.lineTo (sc.x + std::sin (a1) * 1400, sc.y - std::cos (a1) * 1400);
            w.closeSubPath();
            g.setColour (pal::gold.withAlpha (0.55f)); g.fillPath (w);
        }

        // rainbow stripes sweeping across behind the title
        const Colour stripes[] = { pal::brown, pal::red, pal::orange, pal::gold, pal::avocado };
        for (int i = 0; i < 5; ++i)
        {
            float yy = face.getY() + 108 + i * 13.0f;
            Path s; s.startNewSubPath (face.getX() - 10, yy + 30);
            s.cubicTo (face.getX() + 250, yy - 30, face.getRight() - 250, yy + 70, face.getRight() + 10, yy);
            g.setColour (stripes[i]);
            g.strokePath (s, PathStrokeType (13.0f));
        }

        // wear: faint scratches & scuffs on the paint
        Random rr (7);
        for (int i = 0; i < 90; ++i)
        {
            float x = face.getX() + rr.nextFloat() * face.getWidth();
            float y = face.getY() + rr.nextFloat() * face.getHeight();
            float len = 6 + rr.nextFloat() * 28;
            float a = -0.4f + rr.nextFloat() * 0.8f;
            g.setColour (pal::paper.withAlpha (0.10f + 0.12f * rr.nextFloat()));
            g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.8f);
        }
        for (int i = 0; i < 14; ++i)
        {
            bool top = rr.nextBool();
            float x = face.getX() + rr.nextFloat() * face.getWidth();
            float y = top ? face.getY() + rr.nextFloat() * 6 : face.getBottom() - rr.nextFloat() * 6;
            g.setColour (Colour (0xffc9c3b6).withAlpha (0.5f));
            g.fillEllipse (x, y - 2, 6 + rr.nextFloat() * 18, 3 + rr.nextFloat() * 3);
        }
    }
    g.setColour (pal::brown); g.drawRoundedRectangle (face, 22.0f, 3.0f);

    // latch on the lid
    {
        Rectangle<float> latch (face.getCentreX() - 34, face.getY() - 6, 68, 22);
        g.setGradientFill (ColourGradient (Colour (0xfff2efe6), latch.getX(), latch.getY(), Colour (0xff8a8578), latch.getX(), latch.getBottom(), false));
        g.fillRoundedRectangle (latch, 6.0f);
        g.setColour (Colour (0xff4a463e)); g.drawRoundedRectangle (latch, 6.0f, 1.2f);
        rivet (g, { latch.getX() + 10, latch.getCentreY() }, 3.5f);
        rivet (g, { latch.getRight() - 10, latch.getCentreY() }, 3.5f);
    }

    // corner rivets
    for (auto pt : { Point<float> (body.getX() + 22, body.getY() + 22), Point<float> (body.getRight() - 22, body.getY() + 22),
                     Point<float> (body.getX() + 22, body.getBottom() - 22), Point<float> (body.getRight() - 22, body.getBottom() - 22) })
        rivet (g, pt, 5.0f);

    // ---- sun + daisies decoration
    {
        Point<float> sun (128, 140);
        for (int i = 0; i < 12; ++i)
        {
            float a = MathConstants<float>::twoPi * i / 12.0f;
            Path ray; ray.addTriangle (-7, -34, 7, -34, 0, -54);
            ray.applyTransform (AffineTransform::rotation (a).translated (sun));
            g.setColour (pal::gold); g.fillPath (ray);
            g.setColour (pal::brown); g.strokePath (ray, PathStrokeType (1.6f));
        }
        g.setColour (pal::gold); g.fillEllipse (sun.x - 32, sun.y - 32, 64, 64);
        g.setColour (pal::brown); g.drawEllipse (sun.x - 32, sun.y - 32, 64, 64, 2.5f);
        // smile
        Path smile; smile.addCentredArc (sun.x, sun.y + 2, 16, 12, 0, MathConstants<float>::pi * 0.65f, MathConstants<float>::pi * 1.35f, true);
        g.strokePath (smile, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.fillEllipse (sun.x - 11, sun.y - 10, 6, 8);
        g.fillEllipse (sun.x + 5, sun.y - 10, 6, 8);
    }
    daisy (g, { 860, 128 }, 26, pal::paper, pal::gold, 0.2f);
    daisy (g, { 905, 175 }, 15, pal::paper, pal::orange, 0.5f);
    daisy (g, { 816, 178 }, 12, pal::pink, pal::gold, 0.1f);

    // ---- title
    {
        auto titleArea = Rectangle<float> (200, 84, 580, 96);
        auto tp = textPath ("Scooby Grooves", LunchboxLook::title (70.0f), titleArea);
        g.setColour (Colours::black.withAlpha (0.3f));
        g.strokePath (tp, PathStrokeType (16.0f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation (3, 6));
        g.setColour (pal::brown);
        g.strokePath (tp, PathStrokeType (16.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (pal::gold);
        g.strokePath (tp, PathStrokeType (9.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (pal::brown);
        g.strokePath (tp, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill (ColourGradient (pal::paper, 0, titleArea.getY() + 20, pal::cream.darker (0.05f), 0, titleArea.getBottom() - 10, false));
        g.fillPath (tp);

        // "2" starburst badge
        {
            Point<float> bc (titleArea.getRight() - 34, titleArea.getY() + 26);
            Path star;
            for (int i = 0; i < 24; ++i)
            {
                float a = MathConstants<float>::twoPi * i / 24.0f, rr = (i % 2 == 0) ? 34.0f : 26.0f;
                Point<float> pt (bc.x + std::sin (a) * rr, bc.y - std::cos (a) * rr);
                if (i == 0) star.startNewSubPath (pt); else star.lineTo (pt);
            }
            star.closeSubPath();
            g.setColour (Colours::black.withAlpha (0.3f)); g.fillPath (star, AffineTransform::translation (2, 4));
            g.setColour (pal::red); g.fillPath (star);
            g.setColour (pal::brown); g.strokePath (star, PathStrokeType (3.0f));
            auto two = textPath ("2", LunchboxLook::title (40.0f), Rectangle<float> (bc.x - 30, bc.y - 30, 60, 60));
            g.setColour (pal::brown); g.strokePath (two, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (pal::paper); g.fillPath (two);
        }

        // subtitle tag
        auto tag = Rectangle<float> (titleArea.getCentreX() - 150, 178, 300, 26);
        g.setColour (pal::brown); g.fillRoundedRectangle (tag, 13.0f);
        g.setColour (pal::cream); g.setFont (LunchboxLook::label (16.0f));
        g.drawText (String::fromUTF8 ("LO-FI \xe2\x80\xa2 CASSETTE \xe2\x80\xa2 TUBE \xe2\x80\xa2 GLITCH"), tag, Justification::centred);
    }

    // ---- section cards
    drawCard (g, cardLofi, "Lo-Fi", pal::orange, 0);
    drawCard (g, cardTape, "Cassette", pal::teal, 1);
    drawCard (g, cardTube, "Tube", pal::avocado, 2);

    // ---- bottom deck
    {
        auto d = deck.toFloat();
        g.setColour (Colours::black.withAlpha (0.3f)); g.fillRoundedRectangle (d.translated (3, 5), 18.0f);
        g.setColour (pal::brown); g.fillRoundedRectangle (d, 18.0f);
        // wood-ish grain stripes
        {
            Graphics::ScopedSaveState ss (g);
            Path clip; clip.addRoundedRectangle (d, 18.0f); g.reduceClipRegion (clip);
            Random rr (3);
            for (int i = 0; i < 26; ++i)
            {
                float y = d.getY() + rr.nextFloat() * d.getHeight();
                g.setColour (pal::darkBrown.withAlpha (0.35f));
                Path s; s.startNewSubPath (d.getX(), y);
                s.cubicTo (d.getX() + 300, y + rr.nextFloat() * 10 - 5, d.getRight() - 300, y + rr.nextFloat() * 10 - 5, d.getRight(), y);
                g.strokePath (s, PathStrokeType (0.8f + rr.nextFloat() * 1.4f));
            }
        }
        g.setColour (pal::gold); g.drawRoundedRectangle (d.reduced (5), 14.0f, 2.0f);
        g.setColour (pal::darkBrown); g.drawRoundedRectangle (d, 18.0f, 3.0f);

        g.setFont (LunchboxLook::label (15.0f));
        g.setColour (pal::cream);
        g.drawText ("ROUTING", Rectangle<float> (d.getX() + 26, d.getY() + 20, 210, 22), Justification::centredLeft);
        g.drawText ("PRESET", Rectangle<float> (d.getX() + 26, d.getY() + 92, 210, 22), Justification::centredLeft);

        // glitch lamp label
        g.setFont (LunchboxLook::label (11.0f));
        g.setFont (LunchboxLook::label (12.5f));
        g.drawText ("GLITCH SYNC", Rectangle<float> ((float) deck.getCentreX() + 80, d.getY() + 90, 108, 18), Justification::centred);
        g.setFont (LunchboxLook::label (11.0f));
        g.drawText ("ACTIVE", glitchLampArea.toFloat().translated (0, 22).withWidth (44).translated (-11, 0), Justification::centred);
    }
}

void ScoobyGroovesEditor::paintOverChildren (Graphics& g)
{
    auto r = glitchLampArea.toFloat();
    if (lamp > 0.02f)
    {
        g.setColour (pal::red.brighter (0.4f).withAlpha (0.35f * lamp));
        g.fillEllipse (r.expanded (8));
    }
    g.setColour (pal::red.darker (0.6f).interpolatedWith (Colour (0xffff4a2a), lamp));
    g.fillEllipse (r);
    g.setColour (Colours::white.withAlpha (0.35f + 0.4f * lamp));
    g.fillEllipse (r.getX() + 5, r.getY() + 4, 7, 5);
    g.setColour (pal::darkBrown); g.drawEllipse (r, 2.0f);
}
