#include <wx/wx.h>
#include <wx/image.h>
#include "MainFrame.h"

class App : public wxApp {
public:
    bool OnInit() override {
        wxInitAllImageHandlers();
        MainFrame *frame = new MainFrame();
        frame->Show();
        return true;
    }
};

wxIMPLEMENT_APP(App);
