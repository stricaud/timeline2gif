#include "TigEditor.h"
#include <cctype>

/* ── helpers ──────────────────────────────────────────────────────────── */

static bool is_key_char(char c)
{
    return std::isalnum((unsigned char)c) || c == '.' || c == '_';
}

static bool is_num_char(char c)
{
    return std::isdigit((unsigned char)c) || c == '-' || c == '.' ||
           c == '(' || c == ')' || c == ',';
}

/* ── constructor ──────────────────────────────────────────────────────── */

TigEditor::TigEditor(wxWindow *parent, wxWindowID id)
    : wxStyledTextCtrl(parent, id)
{
    SetLexer(wxSTC_LEX_CONTAINER);
    ApplyTheme();

    Bind(wxEVT_STC_STYLENEEDED, &TigEditor::OnStyleNeeded, this);
    Bind(wxEVT_STC_CHARADDED,   &TigEditor::OnCharAdded,   this);
}

/* ── colour theme (dark) ──────────────────────────────────────────────── */

void TigEditor::ApplyTheme()
{
    /* Global editor settings */
    SetCaretForeground(wxColour(200, 200, 200));
    SetSelBackground(true, wxColour(50, 80, 130));

    /* Background / default text */
    StyleSetBackground(wxSTC_STYLE_DEFAULT, wxColour(16, 20, 40));
    StyleSetForeground(wxSTC_STYLE_DEFAULT, wxColour(200, 205, 220));
    wxFont monoFont(wxFontInfo(12).Family(wxFONTFAMILY_TELETYPE));
    StyleSetFont(wxSTC_STYLE_DEFAULT, monoFont);
    StyleClearAll();  /* propagate default to all styles */

    /* Comments  — dim, italic */
    StyleSetForeground(STYLE_COMMENT, wxColour(90, 100, 130));
    StyleSetItalic    (STYLE_COMMENT, true);

    /* Setting keys  — cyan */
    StyleSetForeground(STYLE_KEY, wxColour(80, 200, 220));
    StyleSetBold      (STYLE_KEY, false);

    /* Quoted strings  — soft green */
    StyleSetForeground(STYLE_STRING, wxColour(120, 210, 130));

    /* Numbers / argb(…)  — warm orange */
    StyleSetForeground(STYLE_NUMBER, wxColour(250, 160, 60));

    /* Line numbers margin */
    SetMarginType(0, wxSTC_MARGIN_NUMBER);
    SetMarginWidth(0, TextWidth(wxSTC_STYLE_LINENUMBER, "_9999"));
    StyleSetBackground(wxSTC_STYLE_LINENUMBER, wxColour(20, 24, 48));
    StyleSetForeground(wxSTC_STYLE_LINENUMBER, wxColour(70, 80, 110));

    /* Fold margin (right of line numbers) */
    SetMarginType(1, wxSTC_MARGIN_SYMBOL);
    SetMarginWidth(1, 4);
    SetMarginMask(1, 0);

    /* Caret line highlight */
    SetCaretLineVisible(true);
    SetCaretLineBackground(wxColour(26, 32, 58));

    /* Edge column guide at 80 chars */
    SetEdgeMode(wxSTC_EDGE_LINE);
    SetEdgeColumn(80);
    SetEdgeColour(wxColour(40, 50, 80));

    /* Tab / indent */
    SetTabWidth(4);
    SetUseTabs(false);
    SetIndent(4);

    /* Wrap long lines */
    SetWrapMode(wxSTC_WRAP_NONE);

    SetScrollWidth(1);
    SetScrollWidthTracking(true);
}

/* ── container lexer ──────────────────────────────────────────────────── */

void TigEditor::ColourRange(int startPos, int endPos)
{
    if (startPos >= endPos) return;
#if wxCHECK_VERSION(3, 1, 0)
    StartStyling(startPos);
#else
    StartStyling(startPos, 0x1f);  /* wx 3.0: mask selects which style bits to set */
#endif

    int i = startPos;
    while (i < endPos) {
        char c = GetCharAt(i);

        /* ── Comment ─────────────────────────────────────── */
        if (c == '#') {
            int j = i + 1;
            while (j < endPos && GetCharAt(j) != '\n') j++;
            SetStyling(j - i, STYLE_COMMENT);
            i = j;
            continue;
        }

        /* ── Quoted string ───────────────────────────────── */
        if (c == '"') {
            int j = i + 1;
            while (j < endPos && GetCharAt(j) != '"' && GetCharAt(j) != '\n')
                j++;
            if (j < endPos && GetCharAt(j) == '"') j++; /* include closing " */
            SetStyling(j - i, STYLE_STRING);
            i = j;
            continue;
        }

        /* ── Setting key (starts with a letter, contains dots) ── */
        if (std::isalpha((unsigned char)c)) {
            int j = i;
            while (j < endPos && is_key_char(GetCharAt(j))) j++;
            /* Only colour as KEY if it contains a dot (e.g. theme.background)
               or is a standalone keyword.  Plain words in values stay default. */
            wxString word = GetTextRange(i, j);
            bool is_setting = word.Contains('.');
            SetStyling(j - i, is_setting ? STYLE_KEY : STYLE_DEFAULT);
            i = j;
            continue;
        }

        /* ── Number / argb literal ───────────────────────── */
        if (std::isdigit((unsigned char)c) || c == '-') {
            int j = i;
            while (j < endPos && is_num_char(GetCharAt(j))) j++;
            SetStyling(j - i, STYLE_NUMBER);
            i = j;
            continue;
        }

        /* ── Everything else ─────────────────────────────── */
        SetStyling(1, STYLE_DEFAULT);
        i++;
    }
}

void TigEditor::OnStyleNeeded(wxStyledTextEvent &e)
{
    int startPos = GetEndStyled();
    int endPos   = e.GetPosition();

    /* Snap back to the start of the line containing startPos so we
       never style mid-token. */
    int line = LineFromPosition(startPos);
    startPos = PositionFromLine(line);

    ColourRange(startPos, endPos);
}

/* ── auto-indent on Enter ─────────────────────────────────────────────── */

void TigEditor::OnCharAdded(wxStyledTextEvent &e)
{
    if (e.GetKey() != '\n') return;

    int curLine = GetCurrentLine();
    if (curLine <= 0) return;

    wxString prev = GetLine(curLine - 1);
    int indent = 0;
    while (indent < (int)prev.size() && (prev[indent] == ' ' || prev[indent] == '\t'))
        indent++;

    if (indent > 0) {
        wxString spaces(indent, ' ');
        InsertText(GetCurrentPos(), spaces);
        GotoPos(GetCurrentPos() + indent);
    }
}
