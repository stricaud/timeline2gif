#include "AddEventDialog.h"
#include <wx/spinctrl.h>
#include <wx/statline.h>
#include <wx/filename.h>

/* Convert a wxColour to the argb() notation used in .tig files. */
wxString AddEventDialog::ColourToArgb(const wxColour &c)
{
    return wxString::Format("argb(255,%d,%d,%d)", c.Red(), c.Green(), c.Blue());
}

/* If `path` is inside the current working directory, return a relative path;
   otherwise return the absolute path. Both are valid in .tig files. */
wxString AddEventDialog::RelativePath(const wxString &path)
{
    if (path.empty()) return path;
    wxFileName fn(path);
    fn.MakeRelativeTo(wxGetCwd());
    return fn.GetFullPath();
}

AddEventDialog::AddEventDialog(wxWindow *parent)
    : wxDialog(parent, wxID_ANY, "Add Timeline Event",
               wxDefaultPosition, wxSize(460, -1),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
    auto *vbox = new wxBoxSizer(wxVERTICAL);
    const int PAD = 8;

    auto add_label = [&](const wxString &text) {
        auto *lbl = new wxStaticText(this, wxID_ANY, text);
        wxFont f = lbl->GetFont();
        f.MakeBold();
        lbl->SetFont(f);
        vbox->Add(lbl, 0, wxLEFT | wxTOP, PAD);
    };

    /* ── Core ──────────────────────────────────────────────────────── */
    add_label("Event");
    auto *grid = new wxFlexGridSizer(2, 2, 4, 8);
    grid->AddGrowableCol(1);

    grid->Add(new wxStaticText(this, wxID_ANY, "Time / Year:"),
               0, wxALIGN_CENTER_VERTICAL);
    m_time = new wxTextCtrl(this, wxID_ANY, "2024");
    grid->Add(m_time, 1, wxEXPAND);

    grid->Add(new wxStaticText(this, wxID_ANY, "Description:"),
               0, wxALIGN_CENTER_VERTICAL);
    m_label = new wxTextCtrl(this, wxID_ANY, "");
    grid->Add(m_label, 1, wxEXPAND);

    vbox->Add(grid, 0, wxEXPAND | wxALL, PAD);
    vbox->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);

    /* ── Appearance ─────────────────────────────────────────────────── */
    add_label("Appearance (optional)");
    auto *appGrid = new wxFlexGridSizer(6, 3, 4, 8);
    appGrid->AddGrowableCol(2);

    /* Dot color */
    m_useDotColor = new wxCheckBox(this, wxID_ANY, "");
    m_dotColor    = new wxColourPickerCtrl(this, wxID_ANY,
                        wxColour(0, 180, 216));
    m_dotColor->Enable(false);
    appGrid->Add(m_useDotColor, 0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(new wxStaticText(this, wxID_ANY, "Dot colour:"),
                  0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(m_dotColor, 0);

    /* Text color */
    m_useTextColor = new wxCheckBox(this, wxID_ANY, "");
    m_textColor    = new wxColourPickerCtrl(this, wxID_ANY,
                         wxColour(224, 224, 224));
    m_textColor->Enable(false);
    appGrid->Add(m_useTextColor, 0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(new wxStaticText(this, wxID_ANY, "Text colour:"),
                  0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(m_textColor, 0);

    /* Dot icon */
    appGrid->Add(new wxStaticText(this, wxID_ANY, ""),  0);
    appGrid->Add(new wxStaticText(this, wxID_ANY, "Dot icon (SVG/PNG/GIF):"),
                  0, wxALIGN_CENTER_VERTICAL);
    m_dotIcon = new wxFilePickerCtrl(this, wxID_ANY, "",
                    "Choose dot icon",
                    "Images (*.svg;*.png;*.gif;*.jpg)|*.svg;*.png;*.gif;*.jpg",
                    wxDefaultPosition, wxDefaultSize,
                    wxFLP_USE_TEXTCTRL | wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
    appGrid->Add(m_dotIcon, 1, wxEXPAND);

    /* Dot icon size */
    appGrid->Add(new wxStaticText(this, wxID_ANY, ""), 0);
    appGrid->Add(new wxStaticText(this, wxID_ANY, "Icon size (px):"),
                  0, wxALIGN_CENTER_VERTICAL);
    m_dotIconSize = new wxSpinCtrl(this, wxID_ANY, "28",
                       wxDefaultPosition, wxSize(70, -1),
                       wxSP_ARROW_KEYS, 8, 256, 28);
    appGrid->Add(m_dotIconSize, 0);

    vbox->Add(appGrid, 0, wxEXPAND | wxALL, PAD);

    /* Background color */
    m_useBackground = new wxCheckBox(this, wxID_ANY, "");
    m_bgColor       = new wxColourPickerCtrl(this, wxID_ANY, wxColour(6, 12, 28));
    m_bgColor->Enable(false);
    appGrid->Add(m_useBackground, 0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(new wxStaticText(this, wxID_ANY, "Background:"),
                  0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(m_bgColor, 0);

    /* Background2 color */
    m_useBackground2 = new wxCheckBox(this, wxID_ANY, "");
    m_bg2Color       = new wxColourPickerCtrl(this, wxID_ANY, wxColour(3, 7, 16));
    m_bg2Color->Enable(false);
    appGrid->Add(m_useBackground2, 0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(new wxStaticText(this, wxID_ANY, "Background 2:"),
                  0, wxALIGN_CENTER_VERTICAL);
    appGrid->Add(m_bg2Color, 0);

    /* Enable/disable colour pickers when checkboxes toggle */
    m_useDotColor->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) {
        m_dotColor->Enable(m_useDotColor->IsChecked());
    });
    m_useTextColor->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) {
        m_textColor->Enable(m_useTextColor->IsChecked());
    });
    m_useBackground->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) {
        m_bgColor->Enable(m_useBackground->IsChecked());
    });
    m_useBackground2->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent &) {
        m_bg2Color->Enable(m_useBackground2->IsChecked());
    });

    vbox->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);

    /* ── Callout ─────────────────────────────────────────────────────── */
    m_addCallout = new wxCheckBox(this, wxID_ANY, " Add callout spotlight for this event");
    wxFont bold = m_addCallout->GetFont(); bold.MakeBold();
    m_addCallout->SetFont(bold);
    vbox->Add(m_addCallout, 0, wxALL, PAD);

    auto *calloutGrid = new wxFlexGridSizer(3, 2, 4, 8);
    calloutGrid->AddGrowableCol(1);

    /* Effect */
    calloutGrid->Add(new wxStaticText(this, wxID_ANY, "Effect:"),
                      0, wxALIGN_CENTER_VERTICAL);
    wxArrayString effects;
    effects.Add("zoom");
    effects.Add("funnel");
    effects.Add("float");
    effects.Add("fan");
    effects.Add("none");
    m_calloutEffect = new wxChoice(this, wxID_ANY, wxDefaultPosition,
                                    wxDefaultSize, effects);
    m_calloutEffect->SetSelection(0);
    calloutGrid->Add(m_calloutEffect, 0);

    /* Callout image */
    calloutGrid->Add(new wxStaticText(this, wxID_ANY, "Image (SVG/PNG/GIF):"),
                      0, wxALIGN_CENTER_VERTICAL);
    m_calloutImage = new wxFilePickerCtrl(this, wxID_ANY, "",
                         "Choose callout image",
                         "Images (*.svg;*.png;*.gif;*.jpg)|*.svg;*.png;*.gif;*.jpg",
                         wxDefaultPosition, wxDefaultSize,
                         wxFLP_USE_TEXTCTRL | wxFLP_OPEN | wxFLP_FILE_MUST_EXIST);
    calloutGrid->Add(m_calloutImage, 1, wxEXPAND);

    /* Callout image size */
    calloutGrid->Add(new wxStaticText(this, wxID_ANY, "Image size (px):"),
                      0, wxALIGN_CENTER_VERTICAL);
    m_calloutImageSize = new wxSpinCtrl(this, wxID_ANY, "60",
                             wxDefaultPosition, wxSize(70, -1),
                             wxSP_ARROW_KEYS, 8, 512, 60);
    calloutGrid->Add(m_calloutImageSize, 0);

    auto *calloutBox = new wxStaticBoxSizer(wxVERTICAL, this);
    calloutBox->Add(calloutGrid, 0, wxEXPAND | wxALL, PAD - 2);
    vbox->Add(calloutBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, PAD);

    /* Initially hidden until checkbox is ticked */
    calloutBox->Show(false);
    m_addCallout->Bind(wxEVT_CHECKBOX, [this, calloutBox](wxCommandEvent &) {
        calloutBox->Show(m_addCallout->IsChecked());
        Fit();
    });

    /* ── Buttons ─────────────────────────────────────────────────────── */
    vbox->Add(new wxStaticLine(this), 0, wxEXPAND | wxLEFT | wxRIGHT, PAD);
    auto *btnSizer = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    vbox->Add(btnSizer, 0, wxEXPAND | wxALL, PAD);

    SetSizerAndFit(vbox);
    CentreOnParent();

    Bind(wxEVT_BUTTON, &AddEventDialog::OnOK, this, wxID_OK);
}

void AddEventDialog::OnOK(wxCommandEvent &)
{
    wxString time  = m_time->GetValue().Trim(true).Trim(false);
    wxString label = m_label->GetValue().Trim(true).Trim(false);

    if (time.empty()) {
        wxMessageBox("Please enter a time / year label.", "Missing field",
                     wxOK | wxICON_WARNING, this);
        m_time->SetFocus();
        return;
    }

    wxString s;
    s += "\n# ── Event ────────────────────────────────────────────────\n";

    if (m_useDotColor->IsChecked())
        s += "event.dot_color   " + ColourToArgb(m_dotColor->GetColour()) + "\n";

    if (m_useTextColor->IsChecked())
        s += "event.text_color  " + ColourToArgb(m_textColor->GetColour()) + "\n";

    if (m_useBackground->IsChecked())
        s += "event.background  " + ColourToArgb(m_bgColor->GetColour()) + "\n";

    if (m_useBackground2->IsChecked())
        s += "event.background2 " + ColourToArgb(m_bg2Color->GetColour()) + "\n";

    wxString icon = m_dotIcon->GetPath();
    if (!icon.empty()) {
        s += "event.image       \"" + RelativePath(icon) + "\"\n";
        s += wxString::Format("event.image_size  %d\n", m_dotIconSize->GetValue());
    }

    if (m_addCallout->IsChecked()) {
        s += "event.callout_effect  " +
             m_calloutEffect->GetString(m_calloutEffect->GetSelection()) + "\n";

        wxString cimg = m_calloutImage->GetPath();
        if (!cimg.empty()) {
            s += "event.callout_image      \"" + RelativePath(cimg) + "\"\n";
            s += wxString::Format("event.callout_image_size %d\n",
                                   m_calloutImageSize->GetValue());
        }
    }

    s += "\"" + time + "\"  \"" + label + "\"\n";

    m_snippet = s;
    EndModal(wxID_OK);
}
