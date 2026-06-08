#include "PreviewPanel.h"
#include <wx/dcbuffer.h>
#include <wx/mstream.h>
#include <wx/animate.h>
#include <algorithm>

PreviewPanel::PreviewPanel(wxWindow *parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
               wxFULL_REPAINT_ON_RESIZE),
      m_timer(this)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    SetBackgroundColour(wxColour(18, 18, 28));

    Bind(wxEVT_PAINT, &PreviewPanel::OnPaint, this);
    Bind(wxEVT_SIZE,  &PreviewPanel::OnSize,  this);
    Bind(wxEVT_TIMER, &PreviewPanel::OnTimer, this);
}

bool PreviewPanel::LoadFrames(const t2g_frame_list_t *list)
{
    m_timer.Stop();
    m_originals.clear();
    m_delays.clear();
    m_frameIdx = 0;
    m_current  = wxNullBitmap;

    if (!list || list->count == 0) return false;

    m_originals.reserve(list->count);
    m_delays.reserve(list->count);

    for (int i = 0; i < list->count; i++) {
        const t2g_frame_t &f = list->frames[i];
        wxMemoryInputStream stream(f.data, f.size);
        wxImage img(stream, wxBITMAP_TYPE_PNG);
        if (!img.IsOk()) return false;
        m_originals.push_back(img);
        m_delays.push_back(f.delay_ms > 0 ? f.delay_ms : 40);
    }

    ScaleFrame(0);
    Refresh();

    if (m_originals.size() > 1)
        m_timer.StartOnce(m_delays[0]);

    return true;
}

bool PreviewPanel::LoadFile(const wxString &path)
{
    m_timer.Stop();
    m_originals.clear();
    m_delays.clear();
    m_frameIdx = 0;
    m_current  = wxNullBitmap;

    /* Use wxAnimation to iterate GIF frames */
    wxAnimation anim;
    if (!anim.LoadFile(path) || !anim.IsOk() || anim.GetFrameCount() == 0)
        return false;

    int n = anim.GetFrameCount();
    m_originals.reserve(n);
    m_delays.reserve(n);

    for (int i = 0; i < n; i++) {
        wxImage img = anim.GetFrame(i);
        if (!img.IsOk()) return false;
        m_originals.push_back(img);
        int d = anim.GetDelay(i);
        m_delays.push_back(d > 0 ? d : 40);
    }

    ScaleFrame(0);
    Refresh();

    if (n > 1)
        m_timer.StartOnce(m_delays[0]);

    return true;
}

void PreviewPanel::Clear()
{
    m_timer.Stop();
    m_originals.clear();
    m_delays.clear();
    m_current  = wxNullBitmap;
    m_frameIdx = 0;
    Refresh();
}

void PreviewPanel::ScaleFrame(int idx)
{
    if (idx < 0 || idx >= (int)m_originals.size()) return;
    const wxImage &img = m_originals[idx];

    wxSize panel = GetClientSize();
    if (panel.x < 4 || panel.y < 4) { m_current = wxNullBitmap; return; }

    double sx = (double)panel.x / img.GetWidth();
    double sy = (double)panel.y / img.GetHeight();
    double s  = std::min(sx, sy);
    int dw = std::max(1, (int)(img.GetWidth()  * s));
    int dh = std::max(1, (int)(img.GetHeight() * s));

    m_current = wxBitmap(img.Scale(dw, dh, wxIMAGE_QUALITY_BICUBIC));
}

void PreviewPanel::AdvanceFrame()
{
    if (m_originals.empty()) return;
    m_frameIdx = (m_frameIdx + 1) % (int)m_originals.size();
    ScaleFrame(m_frameIdx);
    Refresh();
    m_timer.StartOnce(m_delays[m_frameIdx]);
}

void PreviewPanel::OnPaint(wxPaintEvent &)
{
    wxAutoBufferedPaintDC dc(this);
    wxSize panel = GetClientSize();
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    if (!m_current.IsOk()) {
        dc.SetTextForeground(wxColour(100, 100, 120));
        dc.DrawLabel("No preview — edit the .tig file and click Generate",
                      wxRect(0, 0, panel.x, panel.y), wxALIGN_CENTER);
        return;
    }

    int x = (panel.x - m_current.GetWidth())  / 2;
    int y = (panel.y - m_current.GetHeight()) / 2;
    dc.DrawBitmap(m_current, x, y, true);
}

void PreviewPanel::OnSize(wxSizeEvent &e)
{
    ScaleFrame(m_frameIdx);
    Refresh();
    e.Skip();
}

void PreviewPanel::OnTimer(wxTimerEvent &)
{
    AdvanceFrame();
}
