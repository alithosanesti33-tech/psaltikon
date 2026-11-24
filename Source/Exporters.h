// Psaltikon — offline audio rendering (WAV) and printable score export (PDF / PNG).
#pragma once
#include "Performer.h"
#include "ScoreRenderer.h"

namespace byz
{
// Renders the composition offline. what: 0 melody + ison, 1 melody only, 2 ison only.
inline bool renderWav (const Composition& comp, const TimelineOptions& topt, PerformParams prm, int what,
                       const juce::File& file, double sampleRate = 48000.0)
{
    auto tl = buildTimeline (comp, topt);
    prm.renderMelody = what != 2;
    prm.renderIson = what != 1;
    if (prm.isonMode == IsonOff && what == 2) prm.isonMode = IsonScore;
    if (prm.isonMode == IsonLive) prm.isonMode = IsonScore;
    prm.droneOn = prm.isonMode == IsonManual;
    Performer perf;
    perf.prepare (sampleRate, 512);
    perf.setParams (prm);
    perf.play (tl, 0.0);
    const double secs = tl->total * 60.0 / std::max (20.0, comp.bpm) + 4.0;
    const int total = (int) (secs * sampleRate);
    juce::AudioBuffer<float> buf (2, total);
    buf.clear();
    for (int pos = 0; pos < total; pos += 512)
    {
        const int n = std::min (512, total - pos);
        perf.process (buf.getWritePointer (0, pos), buf.getWritePointer (1, pos), n);
        if (! perf.isPlaying() && prm.isonMode == IsonManual && pos > (int) ((secs - 4.0) * sampleRate)) { /* drone keeps going to the end */ }
    }
    // fade the last second
    const int fade = (int) sampleRate;
    for (int ch = 0; ch < 2; ++ch)
        buf.applyGainRamp (ch, total - fade, fade, 1.0f, 0.0f);
    const float pk = buf.getMagnitude (0, total);
    if (pk > 0.95f) buf.applyGain (0.95f / pk);

    file.deleteFile();
    std::unique_ptr<juce::FileOutputStream> out (file.createOutputStream());
    if (! out) return false;
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (out.get(), sampleRate, 2, 24, {}, 0));
    if (! w) return false;
    out.release();
    return w->writeFromAudioSampleBuffer (buf, 0, total);
}

// Renders score pages (A4 proportion) as images.
inline std::vector<juce::Image> renderPages (const Composition& comp, bool showParallage)
{
    ScoreStyle st;
    st.scale = 1.6f;
    st.showParallage = showParallage;
    const float pageW = 1240.0f, pageH = pageW * 842.0f / 595.0f, top = 60.0f, bottom = 70.0f;
    auto L = layoutScore (comp, pageW, st);
    std::vector<juce::Image> pages;
    int row = 0;
    while (row < L.rows || pages.empty())
    {
        juce::Image img (juce::Image::RGB, (int) pageW, (int) pageH, true);
        juce::Graphics g (img);
        g.fillAll (juce::Colours::white);
        const bool first = pages.empty();
        const float startY = first ? top : top;
        const float avail = pageH - startY - bottom - (first ? L.headerH : 0);
        const int fit = std::max (1, (int) (avail / L.rowH));
        const int last = std::min (L.rows - 1, row + fit - 1);
        const float yOff = first ? startY : startY - L.headerH;
        paintScore (g, comp, L, st, -1, -1, -1, row, last, yOff);
        g.setColour (st.faint);
        g.setFont (serifFont (18));
        g.drawText (juce::String ((int) pages.size() + 1), juce::Rectangle<float> (0, pageH - 50, pageW, 24), juce::Justification::centred, false);
        pages.push_back (img);
        row = last + 1;
        if (L.rows == 0) break;
    }
    return pages;
}

// Minimal PDF writer: one JPEG image per A4 page.
inline bool writePdf (const std::vector<juce::Image>& pages, const juce::File& file, const juce::String& title)
{
    juce::MemoryOutputStream pdf;
    std::vector<juce::int64> offsets;
    auto obj = [&] (const juce::String& body)
    {
        offsets.push_back (pdf.getPosition());
        pdf << juce::String ((int) offsets.size()) << " 0 obj\n" << body << "\nendobj\n";
    };
    {
        const unsigned char head[] = { '%', 'P', 'D', 'F', '-', '1', '.', '4', '\n', '%', 0xE2, 0xE3, 0xCF, 0xD3, '\n' };
        pdf.write (head, sizeof (head));
    }
    const int n = (int) pages.size();
    // object numbering: 1 catalog, 2 pages, 3 info, then per page: page, content, image
    juce::String kids;
    for (int p = 0; p < n; ++p) kids << juce::String (4 + p * 3) << " 0 R ";
    obj ("<< /Type /Catalog /Pages 2 0 R >>");
    obj ("<< /Type /Pages /Kids [" + kids + "] /Count " + juce::String (n) + " >>");
    juce::String safeTitle;
    for (auto ch : title) if (ch >= 32 && ch < 127 && ch != '(' && ch != ')' && ch != '\\') safeTitle += juce::String::charToString (ch);
    obj ("<< /Title (" + (safeTitle.isEmpty() ? juce::String ("Psaltikon score") : safeTitle) + ") /Creator (Psaltikon) >>");
    for (int p = 0; p < n; ++p)
    {
        const auto& img = pages[(size_t) p];
        juce::MemoryOutputStream jpg;
        juce::JPEGImageFormat jf; jf.setQuality (0.92f);
        jf.writeImageToStream (img, jpg);
        const int pageObj = 4 + p * 3, contentObj = pageObj + 1, imgObj = pageObj + 2;
        obj ("<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Resources << /XObject << /Im" + juce::String (p) + " "
             + juce::String (imgObj) + " 0 R >> >> /Contents " + juce::String (contentObj) + " 0 R >>");
        juce::String content = "q 595 0 0 842 0 0 cm /Im" + juce::String (p) + " Do Q";
        obj ("<< /Length " + juce::String (content.length()) + " >>\nstream\n" + content + "\nendstream");
        offsets.push_back (pdf.getPosition());
        pdf << juce::String (imgObj) << " 0 obj\n<< /Type /XObject /Subtype /Image /Width " << juce::String (img.getWidth())
            << " /Height " << juce::String (img.getHeight()) << " /ColorSpace /DeviceRGB /BitsPerComponent 8 /Filter /DCTDecode /Length "
            << juce::String ((juce::int64) jpg.getDataSize()) << " >>\nstream\n";
        pdf.write (jpg.getData(), jpg.getDataSize());
        pdf << "\nendstream\nendobj\n";
    }
    const auto xref = pdf.getPosition();
    pdf << "xref\n0 " << juce::String ((int) offsets.size() + 1) << "\n0000000000 65535 f \n";
    for (auto o : offsets) pdf << juce::String (o).paddedLeft ('0', 10) << " 00000 n \n";
    pdf << "trailer\n<< /Size " << juce::String ((int) offsets.size() + 1) << " /Root 1 0 R /Info 3 0 R >>\nstartxref\n"
        << juce::String (xref) << "\n%%EOF\n";
    return file.replaceWithData (pdf.getData(), pdf.getDataSize());
}
} // namespace byz
