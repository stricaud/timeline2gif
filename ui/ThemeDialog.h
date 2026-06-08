#pragma once
#include <wx/wx.h>
#include <wx/clrpicker.h>

/* Dialog for picking global theme colours.
 * Generates a ready-to-insert theme.* block for the .tig editor.
 */
class ThemeDialog : public wxDialog {
public:
    explicit ThemeDialog(wxWindow *parent);

    /* Returns the .tig snippet; call after ShowModal() == wxID_OK. */
    wxString GetSnippet() const { return m_snippet; }

private:
    wxColourPickerCtrl *m_background;
    wxColourPickerCtrl *m_background2;
    wxColourPickerCtrl *m_accent;
    wxColourPickerCtrl *m_text;

    wxString m_snippet;

    void OnOK(wxCommandEvent &);
    static wxString ToArgb(const wxColour &c);
};
