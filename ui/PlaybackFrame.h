#pragma once
#include <wx/wx.h>
#include "PreviewPanel.h"

extern "C" {
#include "output.h"
}

/* Full-screen animation playback window. Press Escape / Space / click to close. */
class PlaybackFrame : public wxFrame {
public:
    PlaybackFrame(wxWindow *parent, const t2g_frame_list_t *frames);

private:
    PreviewPanel *m_preview;
    void OnKeyDown (wxKeyEvent  &);
    void OnLeftDown(wxMouseEvent &);
};
