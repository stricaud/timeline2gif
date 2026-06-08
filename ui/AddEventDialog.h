#pragma once
#include <wx/wx.h>
#include <wx/clrpicker.h>
#include <wx/filepicker.h>
#include <wx/spinctrl.h>

/* Dialog for inserting a new timeline event into the .tig editor.
 *
 * Generates a ready-to-paste .tig snippet including optional per-event
 * colour overrides, a dot icon, and callout settings.
 */
class AddEventDialog : public wxDialog {
public:
    explicit AddEventDialog(wxWindow *parent);

    /* Returns the generated .tig snippet; call after ShowModal() == wxID_OK. */
    wxString GetSnippet() const { return m_snippet; }

private:
    /* Core */
    wxTextCtrl *m_time;
    wxTextCtrl *m_label;

    /* Appearance */
    wxCheckBox        *m_useDotColor;
    wxColourPickerCtrl *m_dotColor;
    wxCheckBox        *m_useTextColor;
    wxColourPickerCtrl *m_textColor;
    wxFilePickerCtrl  *m_dotIcon;
    wxSpinCtrl        *m_dotIconSize;

    /* Background override (event.background / event.background2) */
    wxCheckBox        *m_useBackground;
    wxColourPickerCtrl *m_bgColor;
    wxCheckBox        *m_useBackground2;
    wxColourPickerCtrl *m_bg2Color;

    /* Callout */
    wxCheckBox       *m_addCallout;
    wxChoice         *m_calloutEffect;
    wxFilePickerCtrl *m_calloutImage;
    wxSpinCtrl       *m_calloutImageSize;

    wxString m_snippet;

    void OnCalloutToggle(wxCommandEvent &);
    void OnOK(wxCommandEvent &);

    static wxString ColourToArgb(const wxColour &c);
    static wxString RelativePath(const wxString &path);
};
