#include "ThemeDialog.h"
#include <wx/statline.h>

static wxColourPickerCtrl *addRow(wxWindow *win, wxFlexGridSizer *grid,
                                   const wxString &label, const wxColour &def)
{
    grid->Add(new wxStaticText(win, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
    auto *p = new wxColourPickerCtrl(win, wxID_ANY, def);
    grid->Add(p, 0);
    return p;
}

ThemeDialog::ThemeDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Theme Colours",
               wxDefaultPosition, wxDefaultSize,
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto *vbox = new wxBoxSizer(wxVERTICAL);
    const int PAD = 10;

    vbox->Add(new wxStaticText(this, wxID_ANY,
                "Set global theme colours. Click OK to insert a theme block\n"
                "at the cursor position in the editor."),
              0, wxALL, PAD);
    vbox->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);

    auto *grid = new wxFlexGridSizer(4, 2, 6, 12);

    m_background  = addRow(this, grid, "Background (main):",
                            wxColour(16, 21, 62));
    m_background2 = addRow(this, grid, "Background 2 (gradient/shadow):",
                            wxColour(10, 15, 40));
    m_accent      = addRow(this, grid, "Accent (timeline, dots):",
                            wxColour(0, 180, 216));
    m_text        = addRow(this, grid, "Text:",
                            wxColour(224, 224, 224));

    vbox->Add(grid, 0, wxEXPAND | wxALL, PAD);
    vbox->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);
    vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, PAD);

    SetSizerAndFit(vbox);
    CentreOnParent();

    Bind(wxEVT_BUTTON, &ThemeDialog::OnOK, this, wxID_OK);
}

wxString ThemeDialog::ToArgb(const wxColour &c)
{
    return wxString::Format("argb(255,%d,%d,%d)", c.Red(), c.Green(), c.Blue());
}

void ThemeDialog::OnOK(wxCommandEvent &)
{
    m_snippet  = "\n# ── Theme ────────────────────────────────────────────────────\n";
    m_snippet += "theme.background  " + ToArgb(m_background->GetColour())  + "\n";
    m_snippet += "theme.background2 " + ToArgb(m_background2->GetColour()) + "\n";
    m_snippet += "theme.accent      " + ToArgb(m_accent->GetColour())      + "\n";
    m_snippet += "theme.text        " + ToArgb(m_text->GetColour())        + "\n";
    EndModal(wxID_OK);
}
