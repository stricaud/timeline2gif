#include "MainFrame.h"
#include "TigEditor.h"
#include "AddEventDialog.h"
#include "ThemeDialog.h"
#include "PlaybackFrame.h"
#include <wx/file.h>
#include <wx/statline.h>

extern "C" {
#include "timeline2gif.h"
}

static const int DEBOUNCE_MS = 800;

/* ── Default starter template ─────────────────────────────────────── */
static const char *DEFAULT_TIG =
"# timeline2gif — starter template\n"
"# Run:  timeline2gif  this.tig  output.gif\n\n"
"image.width  900\n"
"image.height 420\n\n"
"theme.background  argb(255,16,21,62)\n"
"theme.background2 argb(255,10,15,40)\n"
"theme.accent      argb(255,0,180,216)\n"
"theme.text        argb(255,224,224,224)\n\n"
"timeline.position 210\n"
"item.spacing      180\n"
"camera.scroll     yes\n\n"
"speed.frames    4\n"
"speed.nextitem  60\n\n"
"callout.shape   rounded\n"
"callout.effect  zoom\n"
"callout.pause   150\n\n"
"progress.show   yes\n\n"
"# ─── Events ───────────────────────────────────────────────────\n"
"\"2020\"  \"First event\"\n"
"\"2022\"  \"Second event\"\n"
"\"2024\"  \"Third event\"\n";

/* ── Snippet templates inserted from the Insert menu ──────────────── */
static const char *SNIPPET_CANVAS =
"\n# ── Canvas ────────────────────────────────────────────────────\n"
"image.width  1000\n"
"image.height 460\n"
"timeline.position 230\n";

static const char *SNIPPET_THEME =
"\n# ── Dark theme ─────────────────────────────────────────────────\n"
"theme.background  argb(255,6,12,28)\n"
"theme.background2 argb(255,3,7,18)\n"
"theme.accent      argb(255,0,210,190)\n"
"theme.text        argb(255,210,240,235)\n";

static const char *SNIPPET_CALLOUT =
"\n# ── Callout spotlight ──────────────────────────────────────────\n"
"callout.shape   rounded   # rectangle | rounded | cloud\n"
"callout.effect  zoom      # none | funnel | zoom | float | fan\n"
"callout.pause   150       # hold time in centiseconds\n"
"callout.color   argb(255,5,18,38)\n"
"callout.border  argb(255,0,210,190)\n";

static const char *SNIPPET_PROGRESS =
"\n# ── Progress bar ────────────────────────────────────────────────\n"
"progress.show   yes\n"
"progress.color  argb(255,0,210,190)\n";

static const char *SNIPPET_SPLIT =
"\n# ── Split-screen panel (event list on the left) ─────────────────\n"
"split.show   yes\n"
"split.width  260\n";

static const char *SNIPPET_TRANSITION =
"\n# ── Transition between events ───────────────────────────────────\n"
"transition.style  fade    # none | fade | wipe | dissolve\n"
"transition.frames 8\n";

/* ─────────────────────────────────────────────────────────────────── */

MainFrame::MainFrame()
    : wxFrame(nullptr, wxID_ANY, "timeline2gif Studio",
              wxDefaultPosition, wxSize(1200, 740)),
      m_debounce(this)
{
    /* ── Menu bar ─────────────────────────────────────────────────── */
    auto *mb = new wxMenuBar;

    auto *fileMenu = new wxMenu;
    fileMenu->Append(wxID_NEW);
    fileMenu->Append(wxID_OPEN);
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_SAVE);
    fileMenu->Append(wxID_SAVEAS);
    fileMenu->AppendSeparator();
    fileMenu->Append(ID_EXPORT_ANIM, "Export &Animation…\tCtrl+Shift+S",
                     "Save the rendered animation as GIF, APNG, or WebP");
    fileMenu->AppendSeparator();
    fileMenu->Append(wxID_EXIT);
    mb->Append(fileMenu, "&File");

    auto *insertMenu = new wxMenu;
    insertMenu->Append(ID_INSERT_EVENT,    "&Add Event…\tCtrl+E",
                       "Insert a new timeline event");
    insertMenu->Append(ID_THEME,           "&Theme Colours…\tCtrl+T",
                       "Pick global theme colours and insert them");
    insertMenu->AppendSeparator();
    insertMenu->Append(ID_SNIPPET_CANVAS,     "Insert Canvas settings");
    insertMenu->Append(ID_SNIPPET_THEME,      "Insert Dark theme snippet");
    insertMenu->Append(ID_SNIPPET_CALLOUT,    "Insert Callout settings");
    insertMenu->Append(ID_SNIPPET_PROGRESS,   "Insert Progress bar");
    insertMenu->Append(ID_SNIPPET_SPLIT,      "Insert Split-screen panel");
    insertMenu->Append(ID_SNIPPET_TRANSITION, "Insert Transition settings");
    mb->Append(insertMenu, "&Insert");

    auto *viewMenu = new wxMenu;
    viewMenu->Append(ID_PLAY_FULLSCREEN, "&Play Fullscreen\tF5",
                     "Open the generated animation in a fullscreen player");
    mb->Append(viewMenu, "&View");

    SetMenuBar(mb);

    /* ── Status bar ───────────────────────────────────────────────── */
    CreateStatusBar(1);
    SetStatusText("Ready");

    /* ── Main panel ───────────────────────────────────────────────── */
    auto *panel = new wxPanel(this);
    auto *vbox  = new wxBoxSizer(wxVERTICAL);

    /* ── Top toolbar row ──────────────────────────────────────────── */
    auto *tb    = new wxPanel(panel);
    auto *tbbox = new wxBoxSizer(wxHORIZONTAL);

    auto mkBtn = [&](const wxString &label, int id, const wxString &tip) {
        auto *b = new wxButton(tb, id, label, wxDefaultPosition,
                               wxDefaultSize, wxBU_EXACTFIT);
        b->SetToolTip(tip);
        return b;
    };

    tbbox->Add(mkBtn("New",    wxID_NEW,        "New file"),        0, wxALL, 3);
    tbbox->Add(mkBtn("Open",   wxID_OPEN,       "Open .tig file"),  0, wxALL, 3);
    tbbox->Add(mkBtn("Save",   wxID_SAVE,       "Save .tig"),       0, wxALL, 3);
    tbbox->Add(mkBtn("Export", ID_EXPORT_ANIM,  "Export animation (GIF/APNG/WebP)…"),
               0, wxALL, 3);
    tbbox->Add(new wxStaticLine(tb, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLI_VERTICAL),
               0, wxEXPAND | wxTOP | wxBOTTOM, 6);

    auto *evtBtn = mkBtn("✦ Add Event", ID_INSERT_EVENT,
                          "Insert a new event (Ctrl+E)");
    auto *themeBtn = mkBtn("🎨 Theme", ID_THEME,
                            "Pick global theme colours (Ctrl+T)");
    tbbox->Add(evtBtn,   0, wxALL, 3);
    tbbox->Add(themeBtn, 0, wxALL, 3);

    tbbox->Add(new wxStaticLine(tb, wxID_ANY, wxDefaultPosition,
                                 wxDefaultSize, wxLI_VERTICAL),
               0, wxEXPAND | wxTOP | wxBOTTOM, 6);

    auto *genBtn  = mkBtn("▶ Generate", ID_GENERATE,
                           "Generate preview now");
    auto *playBtn = mkBtn("⛶ Fullscreen", ID_PLAY_FULLSCREEN,
                           "Play animation fullscreen (F5)");
    auto *autoChk = new wxCheckBox(tb, ID_AUTO_PREVIEW, " Auto-preview");
    autoChk->SetValue(m_autoPreview);
    autoChk->SetToolTip("Regenerate preview automatically after editing");
    tbbox->Add(genBtn,  0, wxALL, 3);
    tbbox->Add(playBtn, 0, wxALL, 3);
    tbbox->Add(autoChk, 0, wxALL | wxALIGN_CENTER_VERTICAL, 3);

    tb->SetSizer(tbbox);
    vbox->Add(tb, 0, wxEXPAND | wxALL, 2);
    vbox->Add(new wxStaticLine(panel), 0, wxEXPAND);

    /* ── Splitter: editor (left) | preview (right) ────────────────── */
    auto *splitter = new wxSplitterWindow(panel, wxID_ANY,
                                           wxDefaultPosition, wxDefaultSize,
                                           wxSP_LIVE_UPDATE | wxSP_3D);
    splitter->SetMinimumPaneSize(240);

    m_editor = new TigEditor(splitter);
    m_editor->SetText(DEFAULT_TIG);

    m_preview = new PreviewPanel(splitter);

    splitter->SplitVertically(m_editor, m_preview, 440);
    vbox->Add(splitter, 1, wxEXPAND);

    panel->SetSizer(vbox);

    UpdateTitle();
    CentreOnScreen();

    /* ── Event bindings ───────────────────────────────────────────── */
    Bind(wxEVT_MENU,     &MainFrame::OnNew,     this, wxID_NEW);
    Bind(wxEVT_MENU,     &MainFrame::OnOpen,    this, wxID_OPEN);
    Bind(wxEVT_MENU,     &MainFrame::OnSave,    this, wxID_SAVE);
    Bind(wxEVT_MENU,     &MainFrame::OnSaveAs,  this, wxID_SAVEAS);
    Bind(wxEVT_MENU,     &MainFrame::OnExit,    this, wxID_EXIT);
    Bind(wxEVT_BUTTON,   &MainFrame::OnNew,        this, wxID_NEW);
    Bind(wxEVT_BUTTON,   &MainFrame::OnOpen,       this, wxID_OPEN);
    Bind(wxEVT_BUTTON,   &MainFrame::OnSave,       this, wxID_SAVE);
    Bind(wxEVT_MENU,     &MainFrame::OnExportAnim, this, ID_EXPORT_ANIM);
    Bind(wxEVT_BUTTON,   &MainFrame::OnExportAnim, this, ID_EXPORT_ANIM);

    Bind(wxEVT_MENU,   &MainFrame::OnInsertEvent,   this, ID_INSERT_EVENT);
    Bind(wxEVT_BUTTON, &MainFrame::OnInsertEvent,   this, ID_INSERT_EVENT);

    for (int id : {ID_SNIPPET_CANVAS, ID_SNIPPET_THEME, ID_SNIPPET_CALLOUT,
                   ID_SNIPPET_PROGRESS, ID_SNIPPET_SPLIT, ID_SNIPPET_TRANSITION})
        Bind(wxEVT_MENU, &MainFrame::OnInsertSnippet, this, id);

    Bind(wxEVT_BUTTON,   &MainFrame::OnGenerate,   this, ID_GENERATE);
    Bind(wxEVT_CHECKBOX, &MainFrame::OnAutoPreview, this, ID_AUTO_PREVIEW);
    m_editor->Bind(wxEVT_STC_CHANGE, &MainFrame::OnEditorChanged, this);
    Bind(wxEVT_TIMER,    &MainFrame::OnDebounce, this);
    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

    Bind(wxEVT_MENU,   &MainFrame::OnTheme,          this, ID_THEME);
    Bind(wxEVT_BUTTON, &MainFrame::OnTheme,           this, ID_THEME);
    Bind(wxEVT_MENU,   &MainFrame::OnPlayFullscreen,  this, ID_PLAY_FULLSCREEN);
    Bind(wxEVT_BUTTON, &MainFrame::OnPlayFullscreen,  this, ID_PLAY_FULLSCREEN);
}

MainFrame::~MainFrame()
{
    t2g_frame_list_free(&m_frames);
}


/* ── Helpers ──────────────────────────────────────────────────────── */

void MainFrame::UpdateTitle()
{
    wxString t = "timeline2gif Studio";
    if (!m_currentFile.empty())
        t += " — " + wxFileName(m_currentFile).GetFullName();
    if (m_modified) t += " *";
    SetTitle(t);
}

void MainFrame::SetStatus(const wxString &msg)
{
    SetStatusText(msg);
}

void MainFrame::SetModified(bool v)
{
    m_modified = v;
    UpdateTitle();
}

void MainFrame::InsertAtCursor(const wxString &text)
{
    m_editor->InsertText(m_editor->GetCurrentPos(), text);
    m_editor->GotoPos(m_editor->GetCurrentPos() + (int)text.utf8_str().length());
}

bool MainFrame::ConfirmDiscard()
{
    if (!m_modified) return true;
    int r = wxMessageBox("The file has unsaved changes. Continue anyway?",
                          "Unsaved changes",
                          wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
    return (r == wxYES);
}

/* ── Generate ─────────────────────────────────────────────────────── */

void MainFrame::Generate()
{
    /* Write editor content to a temp .tig so t2g_generate_frames() can
     * resolve relative image paths from the file's own directory. */
    wxString tempTig = wxFileName::CreateTempFileName("t2g_input") + ".tig";
    {
        wxFile f(tempTig, wxFile::write);
        if (!f.IsOpened()) { SetStatus("Cannot write temp file"); return; }
        f.Write(m_editor->GetText(), wxConvUTF8);
    }

    SetStatus("Generating…");
    wxYield();

    /* Resolve relative image paths against the opened file's folder, not the
     * temp file's location. Empty (unsaved) → current directory. */
    wxString baseDir = m_currentFile.empty()
        ? wxString(".") : wxFileName(m_currentFile).GetPath();

    t2g_frame_list_free(&m_frames);
    int rc = t2g_generate_frames_in(tempTig.mb_str(), &m_frames,
                                    baseDir.mb_str());
    wxRemoveFile(tempTig);

    if (rc == 0 && m_frames.count > 0) {
        if (m_preview->LoadFrames(&m_frames))
            SetStatus(wxString::Format("OK  (%d frames)", m_frames.count));
        else
            SetStatus("Frames generated but display failed");
    } else {
        /* Keep the last good preview visible; just flag the error. */
        SetStatus("Syntax error in .tig file — preview not updated");
    }
}

/* ── Event handlers ───────────────────────────────────────────────── */

void MainFrame::OnNew(wxCommandEvent &)
{
    if (!ConfirmDiscard()) return;
    m_editor->SetText(DEFAULT_TIG);
    m_currentFile.clear();
    SetModified(false);
    m_preview->Clear();
    SetStatus("New file");
}

void MainFrame::OpenFile(const wxString &path)
{
    wxFile f(path);
    wxString content;
    if (!f.ReadAll(&content)) { wxLogError("Cannot read '%s'", path); return; }
    m_editor->SetText(content);
    m_currentFile = path;
    SetModified(false);
    SetStatus("Opened: " + wxFileName(path).GetFullName());
    if (m_autoPreview) Generate();
}

void MainFrame::OnOpen(wxCommandEvent &)
{
    if (!ConfirmDiscard()) return;
    wxFileDialog dlg(this, "Open timeline file", "", "",
                     "Timeline files (*.tig)|*.tig|All files (*)|*",
                     wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (dlg.ShowModal() != wxID_OK) return;
    OpenFile(dlg.GetPath());
}

void MainFrame::OnSave(wxCommandEvent &)
{
    if (m_currentFile.empty()) { wxCommandEvent e; OnSaveAs(e); return; }
    wxFile f(m_currentFile, wxFile::write);
    if (!f.IsOpened()) { wxLogError("Cannot save '%s'", m_currentFile); return; }
    f.Write(m_editor->GetText(), wxConvUTF8);
    m_editor->SetSavePoint();  /* tells Scintilla "this is the saved state" */
    SetModified(false);
    SetStatus("Saved");
}

void MainFrame::OnSaveAs(wxCommandEvent &)
{
    wxFileDialog dlg(this, "Save timeline file", "", "",
                     "Timeline files (*.tig)|*.tig|All files (*)|*",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;
    m_currentFile = dlg.GetPath();
    wxCommandEvent e; OnSave(e);
}

void MainFrame::OnExit(wxCommandEvent &)     { Close(); }
void MainFrame::OnGenerate(wxCommandEvent &) { Generate(); }

void MainFrame::OnExportAnim(wxCommandEvent &)
{
    if (m_frames.count == 0) {
        wxMessageBox("Nothing to export yet — click Generate first.",
                     "Export Animation", wxOK | wxICON_INFORMATION, this);
        return;
    }

    /* Default filename: same stem as the .tig, or "animation" */
    wxString defaultName = "animation";
    if (!m_currentFile.empty())
        defaultName = wxFileName(m_currentFile).GetName();

    wxFileDialog dlg(this, "Export Animation", "",
                     defaultName,
                     "GIF animation (*.gif)|*.gif|"
                     "Animated PNG (*.apng;*.png)|*.apng;*.png|"
                     "WebP animation (*.webp)|*.webp",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (dlg.ShowModal() != wxID_OK) return;

    wxString outPath = dlg.GetPath();

    /* Ensure the chosen filter's extension is appended when the user didn't
     * type one.  wxFileDialog on macOS often omits it. */
    wxFileName fn(outPath);
    if (fn.GetExt().IsEmpty()) {
        static const wxString exts[] = { "gif", "apng", "webp" };
        int fi = dlg.GetFilterIndex();
        if (fi >= 0 && fi < 3) fn.SetExt(exts[fi]);
        outPath = fn.GetFullPath();
    }

    /* Write the current editor content to a temp .tig so images referenced
     * by relative paths resolve correctly. */
    wxString tempTig = wxFileName::CreateTempFileName("t2g_export") + ".tig";
    {
        wxFile f(tempTig, wxFile::write);
        if (!f.IsOpened()) {
            SetStatus("Cannot write temp file for export");
            return;
        }
        f.Write(m_editor->GetText(), wxConvUTF8);
    }

    SetStatus("Exporting…");
    wxYield();

    wxString baseDir = m_currentFile.empty()
        ? wxString(".") : wxFileName(m_currentFile).GetPath();

    int rc = t2g_generate_in(tempTig.mb_str(), outPath.mb_str(),
                             baseDir.mb_str());
    wxRemoveFile(tempTig);

    if (rc == 0)
        SetStatus("Exported: " + wxFileName(outPath).GetFullName());
    else
        SetStatus("Export failed — check the .tig syntax");
}

void MainFrame::OnAutoPreview(wxCommandEvent &e)
{
    m_autoPreview = e.IsChecked();
}

void MainFrame::OnEditorChanged(wxCommandEvent &)
{
    SetModified(true);
    if (m_autoPreview) {
        m_debounce.Stop();
        m_debounce.StartOnce(DEBOUNCE_MS);
    }
}

void MainFrame::OnDebounce(wxTimerEvent &)
{
    Generate();
}

void MainFrame::OnInsertEvent(wxCommandEvent &)
{
    AddEventDialog dlg(this);
    if (dlg.ShowModal() != wxID_OK) return;
    InsertAtCursor(dlg.GetSnippet());
    m_editor->SetFocus();
}

void MainFrame::OnInsertSnippet(wxCommandEvent &e)
{
    const char *snip = nullptr;
    switch (e.GetId()) {
        case ID_SNIPPET_CANVAS:     snip = SNIPPET_CANVAS;     break;
        case ID_SNIPPET_THEME:      snip = SNIPPET_THEME;      break;
        case ID_SNIPPET_CALLOUT:    snip = SNIPPET_CALLOUT;    break;
        case ID_SNIPPET_PROGRESS:   snip = SNIPPET_PROGRESS;   break;
        case ID_SNIPPET_SPLIT:      snip = SNIPPET_SPLIT;      break;
        case ID_SNIPPET_TRANSITION: snip = SNIPPET_TRANSITION; break;
    }
    if (snip) InsertAtCursor(wxString::FromUTF8(snip));
}

void MainFrame::OnTheme(wxCommandEvent &)
{
    ThemeDialog dlg(this);
    if (dlg.ShowModal() != wxID_OK) return;
    InsertAtCursor(dlg.GetSnippet());
    m_editor->SetFocus();
}

void MainFrame::OnPlayFullscreen(wxCommandEvent &)
{
    if (m_frames.count == 0) {
        SetStatus("No preview generated yet — click Generate first");
        return;
    }
    auto *player = new PlaybackFrame(this, &m_frames);
    player->Show();
}

void MainFrame::OnClose(wxCloseEvent &e)
{
    if (m_modified && e.CanVeto()) {
        int r = wxMessageBox("Save changes before closing?",
                              "timeline2gif Studio",
                              wxYES_NO | wxCANCEL | wxICON_QUESTION, this);
        if (r == wxCANCEL) { e.Veto(); return; }
        if (r == wxYES)    { wxCommandEvent ev; OnSave(ev); }
    }
    Destroy();
}
