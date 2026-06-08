#include "PlaybackFrame.h"

PlaybackFrame::PlaybackFrame(wxWindow *parent, const t2g_frame_list_t *frames)
    : wxFrame(parent, wxID_ANY, "timeline2gif — Playback",
              wxDefaultPosition, wxDefaultSize,
              wxFRAME_FLOAT_ON_PARENT | wxSTAY_ON_TOP |
              wxBORDER_NONE | wxFRAME_NO_TASKBAR)
{
    SetBackgroundColour(*wxBLACK);

    m_preview = new PreviewPanel(this);
    m_preview->LoadFrames(frames);

    auto *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(m_preview, 1, wxEXPAND);
    SetSizer(sizer);

    ShowFullScreen(true, wxFULLSCREEN_ALL);

    m_preview->Bind(wxEVT_KEY_DOWN,  &PlaybackFrame::OnKeyDown,  this);
    m_preview->Bind(wxEVT_LEFT_DOWN, &PlaybackFrame::OnLeftDown, this);
    Bind(wxEVT_KEY_DOWN,             &PlaybackFrame::OnKeyDown,  this);
}

void PlaybackFrame::OnKeyDown(wxKeyEvent &e)
{
    if (e.GetKeyCode() == WXK_ESCAPE || e.GetKeyCode() == WXK_RETURN ||
        e.GetKeyCode() == WXK_SPACE)
        Close();
    else
        e.Skip();
}

void PlaybackFrame::OnLeftDown(wxMouseEvent &)
{
    Close();
}
