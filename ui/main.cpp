#include <wx/wx.h>
#include <wx/image.h>
#include "MainFrame.h"

class App : public wxApp {
public:
    bool OnInit() override {
        wxInitAllImageHandlers();
        m_frame = new MainFrame();
        m_frame->Show();

        // Windows / Linux: a file passed on the command line (e.g. by the OS
        // file association) arrives in argv. macOS delivers it via
        // MacOpenFiles() instead — handled below.
        if (argc > 1) {
            wxString path = argv[1];
            if (!path.empty()) m_frame->OpenFile(path);
        } else if (!m_pendingFile.empty()) {
            // A macOS open-file event arrived before the frame existed.
            m_frame->OpenFile(m_pendingFile);
            m_pendingFile.clear();
        }
        return true;
    }

    // macOS: double-clicking a .tig in Finder (or `open file.tig`) routes
    // here rather than through argv. Open every file we are handed.
    void MacOpenFiles(const wxArrayString &fileNames) override {
        for (const wxString &path : fileNames) {
            if (m_frame) m_frame->OpenFile(path);
            else         m_pendingFile = path;   // arrived before OnInit
        }
    }

private:
    MainFrame *m_frame = nullptr;
    wxString   m_pendingFile;
};

wxIMPLEMENT_APP(App);
