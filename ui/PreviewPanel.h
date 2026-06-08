#pragma once
#include <wx/wx.h>
#include <wx/animate.h>
#include <wx/timer.h>
#include <vector>

extern "C" {
#include "output.h"
}

/* Full-color animated preview panel.
 *
 * Primary path: LoadFrames() from t2g_generate_frames() — Cairo PNG bytes,
 * full 32-bit color, correct per-frame timing.
 *
 * Fallback path: LoadFile() from a GIF path (used by fullscreen player).
 */
class PreviewPanel : public wxPanel {
public:
    explicit PreviewPanel(wxWindow *parent);

    /* Load full-color frames from the in-memory list returned by
     * t2g_generate_frames(). Caller retains ownership of the list. */
    bool LoadFrames(const t2g_frame_list_t *list);

    /* Load a GIF/APNG file (fullscreen player fallback). */
    bool LoadFile(const wxString &path);

    /* Stop playback and clear the display. */
    void Clear();

private:
    /* Full-resolution originals for quality rescaling on resize */
    std::vector<wxImage>  m_originals;
    std::vector<int>      m_delays;     /* per-frame delay in ms */
    wxBitmap              m_current;    /* scaled current frame */
    int                   m_frameIdx = 0;
    wxTimer               m_timer;

    void ScaleFrame(int idx);
    void AdvanceFrame();

    void OnPaint(wxPaintEvent &);
    void OnSize (wxSizeEvent  &);
    void OnTimer(wxTimerEvent &);
};
