#ifndef WXTERMINALCOLOURHANDLER_H
#define WXTERMINALCOLOURHANDLER_H

#include <wx/textctrl.h>
#include <unordered_map>
#include <map>
#include <vector>

enum class eColourHandlerState {
    kNormal = 0,
    kInEscape, // found ESC char
    kInOsc,    // Operating System Command
    kInCsi,    // Control Sequence Introducer
    kFoundCR,  // Found CR
};

class wxTerminalCtrl;
class wxTerminalColourHandler
{
    wxTextCtrl* m_ctrl = nullptr;
    wxTerminalCtrl* m_terminal = nullptr;
    wxString m_escapeSequence;
    std::unordered_map<int, wxColour> m_colours;
    wxTextAttr m_defaultAttr;
    wxString m_title;

protected:
    wxTerminalColourHandler();
    eColourHandlerState m_state = eColourHandlerState::kNormal;
    wxColour GetColour(long colour_number);

protected:
    void Append(const wxString& buffer);
    void SetStyleFromEscape(const wxString& escape);
    void Clear();
    void FlushBuffer(wxString& line);

public:
    wxTerminalColourHandler(wxTerminalCtrl* ctrl);
    ~wxTerminalColourHandler();

    wxTerminalColourHandler& operator<<(const wxString& buffer);
    void SetCtrl(wxTextCtrl* ctrl);
    void SetDefaultStyle(const wxTextAttr& attr);
};

#endif // WXTERMINALCOLOURHANDLER_H
