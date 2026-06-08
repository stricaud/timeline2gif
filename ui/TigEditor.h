#pragma once
#include <wx/wx.h>
#include <wx/stc/stc.h>

/* Scintilla-based editor with syntax highlighting for .tig files.
 *
 * Colour scheme (dark background):
 *   Default text    — light gray
 *   Comments (#…)  — dim gray / italic
 *   Setting keys    — cyan  (theme.*, callout.*, event.*, …)
 *   Quoted strings  — green
 *   Numbers         — orange
 *   argb(…)         — purple (coloured as a number run)
 */
class TigEditor : public wxStyledTextCtrl {
public:
    explicit TigEditor(wxWindow *parent, wxWindowID id = wxID_ANY);

    /* Style IDs used by the container lexer */
    enum {
        STYLE_DEFAULT = 0,
        STYLE_COMMENT,
        STYLE_KEY,
        STYLE_STRING,
        STYLE_NUMBER,
    };

private:
    void ApplyTheme();
    void ColourRange(int start, int end);

    void OnStyleNeeded(wxStyledTextEvent &);
    void OnCharAdded  (wxStyledTextEvent &);
};
