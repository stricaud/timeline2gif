#pragma once
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/timer.h>
#include <wx/filename.h>
#include "PreviewPanel.h"
#include "TigEditor.h"

extern "C" {
#include "output.h"
}

enum {
    ID_INSERT_EVENT    = wxID_HIGHEST + 100,
    ID_GENERATE,
    ID_AUTO_PREVIEW,
    ID_THEME,
    ID_PLAY_FULLSCREEN,
    ID_EXPORT_ANIM,

    /* Insert-menu snippet IDs */
    ID_SNIPPET_CANVAS,
    ID_SNIPPET_THEME,
    ID_SNIPPET_CALLOUT,
    ID_SNIPPET_PROGRESS,
    ID_SNIPPET_SPLIT,
    ID_SNIPPET_TRANSITION,
};

class MainFrame : public wxFrame {
public:
    MainFrame();
    ~MainFrame();

    /* Load a .tig file from the command line. */
    void OpenFile(const wxString &path);

private:
    TigEditor    *m_editor;
    PreviewPanel *m_preview;
    wxStaticText *m_status;
    wxTimer       m_debounce;

    wxString         m_currentFile;
    t2g_frame_list_t m_frames      = {nullptr, 0};
    bool             m_modified    = false;
    bool             m_autoPreview = true;

    void Generate();
    void InsertAtCursor(const wxString &text);
    void SetModified(bool v);
    void UpdateTitle();
    void SetStatus(const wxString &msg);

    /* File */
    void OnNew        (wxCommandEvent &);
    void OnOpen       (wxCommandEvent &);
    void OnSave       (wxCommandEvent &);
    void OnSaveAs     (wxCommandEvent &);
    void OnExportAnim (wxCommandEvent &);
    void OnExit       (wxCommandEvent &);
    /* Edit / Generate */
    void OnGenerate       (wxCommandEvent &);
    void OnAutoPreview    (wxCommandEvent &);
    void OnEditorChanged  (wxCommandEvent &);
    void OnDebounce       (wxTimerEvent   &);
    void OnTheme          (wxCommandEvent &);
    void OnPlayFullscreen (wxCommandEvent &);
    /* Insert */
    void OnInsertEvent   (wxCommandEvent &);
    void OnInsertSnippet (wxCommandEvent &);
    /* Window */
    void OnClose(wxCloseEvent &);

    bool ConfirmDiscard();
};
