#pragma once

#include <string>
#include <memory>

// Windows message posted to the dialog when an HTTP command arrives.
// lParam is a PTZHttpCommand* allocated with new; the handler must delete it.
#define WM_PTZ_HTTP (WM_APP + 0x100)

struct PTZHttpCommand
{
    enum Cmd { Pan, Tilt, Zoom, GotoHome, GotoPreset, SavePreset, GetZoom, GetCount };
    Cmd  cmd;
    int  camIndex;  // 0-based; -1 = currently active camera
    int  param;     // direction or preset number

    // Response written back by the GUI thread before it signals m_ev
    bool        success = false;
    int         intResult = 0;
    std::string strResult;

    HANDLE m_ev;  // manual-reset event; signalled by the GUI handler

    PTZHttpCommand(Cmd c, int cam, int p)
        : cmd(c), camIndex(cam), param(p),
          m_ev(::CreateEvent(nullptr, TRUE, FALSE, nullptr)) {}
    ~PTZHttpCommand() { ::CloseHandle(m_ev); }
};

// Forward declaration so PTZControlDlg.h doesn't pull in httplib.h
class PTZHttpServer;

// Opaque owner handle stored as a unique_ptr in the dialog.
// Defined fully in PTZHttpServer.cpp.
struct PTZHttpServerHandle
{
    std::unique_ptr<PTZHttpServer> server;
};
