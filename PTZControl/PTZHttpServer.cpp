// PTZHttpServer.cpp
// Compiled WITHOUT the precompiled header to keep httplib.h away from MFC macros.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <memory>
#include <thread>
#include <sstream>

#include "PTZHttpServer.h"

// httplib is a large header – keep it isolated to this TU.
#define CPPHTTPLIB_NO_EXCEPTIONS  // we handle errors via return codes
#include "httplib.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Send a command to the GUI thread via PostMessage, wait for it to finish,
// and return the populated command.  The GUI thread owns deletion.
static PTZHttpCommand* Dispatch(HWND hwnd, PTZHttpCommand* cmd)
{
    ::PostMessage(hwnd, WM_PTZ_HTTP, 0, reinterpret_cast<LPARAM>(cmd));
    ::WaitForSingleObject(cmd->m_ev, INFINITE);
    return cmd;
}

static int QueryInt(const httplib::Request& req, const char* key, int def)
{
    auto it = req.params.find(key);
    if (it == req.params.end()) return def;
    try { return std::stoi(it->second); } catch (...) { return def; }
}

static void JsonOk(httplib::Response& res, const std::string& body)
{
    res.set_content(body, "application/json");
    res.status = 200;
}

static void JsonErr(httplib::Response& res, int status, const char* msg)
{
    std::ostringstream ss;
    ss << "{\"success\":false,\"message\":\"" << msg << "\"}";
    res.set_content(ss.str(), "application/json");
    res.status = status;
}

// ---------------------------------------------------------------------------
// PTZHttpServer
// ---------------------------------------------------------------------------
class PTZHttpServer
{
public:
    PTZHttpServer(HWND hwnd, int port) : m_hwnd(hwnd), m_port(port) {}

    bool Start()
    {
        // Register routes before binding
        SetupRoutes();

        // Bind first so we know if the port is available
        if (!m_svr.bind_to_port("0.0.0.0", m_port))
            return false;

        m_thread = std::thread([this] { m_svr.listen_after_bind(); });
        return true;
    }

    void Stop()
    {
        m_svr.stop();
        if (m_thread.joinable())
            m_thread.join();
    }

private:
    void SetupRoutes()
    {
        HWND hwnd = m_hwnd;

        // GET /status  →  { "cameras": N }
        m_svr.Get("/status", [hwnd](const httplib::Request&, httplib::Response& res) {
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::GetCount, 0, 0);
            Dispatch(hwnd, cmd);
            std::ostringstream ss;
            ss << "{\"cameras\":" << cmd->intResult << "}";
            delete cmd;
            JsonOk(res, ss.str());
        });

        // GET /zoom?camera=0  →  { "zoom": N }
        m_svr.Get("/zoom", [hwnd](const httplib::Request& req, httplib::Response& res) {
            int cam = QueryInt(req, "camera", -1);
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::GetZoom, cam, 0);
            Dispatch(hwnd, cmd);
            bool ok = cmd->success;
            int z = cmd->intResult;
            delete cmd;
            if (!ok) return JsonErr(res, 400, "invalid camera index");
            std::ostringstream ss; ss << "{\"zoom\":" << z << "}";
            JsonOk(res, ss.str());
        });

        // POST /pan?camera=0&direction=1
        m_svr.Post("/pan", [hwnd](const httplib::Request& req, httplib::Response& res) {
            int cam = QueryInt(req, "camera", -1);
            int dir = QueryInt(req, "direction", 0);
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::Pan, cam, dir);
            Dispatch(hwnd, cmd);
            bool ok = cmd->success;
            delete cmd;
            if (!ok) return JsonErr(res, 400, "invalid camera index");
            JsonOk(res, "{\"success\":true}");
        });

        // POST /tilt?camera=0&direction=1
        m_svr.Post("/tilt", [hwnd](const httplib::Request& req, httplib::Response& res) {
            int cam = QueryInt(req, "camera", -1);
            int dir = QueryInt(req, "direction", 0);
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::Tilt, cam, dir);
            Dispatch(hwnd, cmd);
            bool ok = cmd->success;
            delete cmd;
            if (!ok) return JsonErr(res, 400, "invalid camera index");
            JsonOk(res, "{\"success\":true}");
        });

        // POST /zoom?camera=0&direction=1
        m_svr.Post("/zoom", [hwnd](const httplib::Request& req, httplib::Response& res) {
            int cam = QueryInt(req, "camera", -1);
            int dir = QueryInt(req, "direction", 0);
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::Zoom, cam, dir);
            Dispatch(hwnd, cmd);
            bool ok = cmd->success;
            int z = cmd->intResult;
            delete cmd;
            if (!ok) return JsonErr(res, 400, "invalid camera index");
            std::ostringstream ss; ss << "{\"success\":true,\"zoom\":" << z << "}";
            JsonOk(res, ss.str());
        });

        // POST /home?camera=0
        m_svr.Post("/home", [hwnd](const httplib::Request& req, httplib::Response& res) {
            int cam = QueryInt(req, "camera", -1);
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::GotoHome, cam, 0);
            Dispatch(hwnd, cmd);
            bool ok = cmd->success;
            delete cmd;
            if (!ok) return JsonErr(res, 400, "invalid camera index");
            JsonOk(res, "{\"success\":true}");
        });

        // POST /preset/recall?camera=0&preset=2
        m_svr.Post("/preset/recall", [hwnd](const httplib::Request& req, httplib::Response& res) {
            int cam    = QueryInt(req, "camera", -1);
            int preset = QueryInt(req, "preset", -1);
            if (preset < 0 || preset > 7) return JsonErr(res, 400, "preset must be 0-7");
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::GotoPreset, cam, preset);
            Dispatch(hwnd, cmd);
            bool ok = cmd->success;
            delete cmd;
            if (!ok) return JsonErr(res, 400, "invalid camera index");
            JsonOk(res, "{\"success\":true}");
        });

        // POST /preset/save?camera=0&preset=2
        m_svr.Post("/preset/save", [hwnd](const httplib::Request& req, httplib::Response& res) {
            int cam    = QueryInt(req, "camera", -1);
            int preset = QueryInt(req, "preset", -1);
            if (preset < 0 || preset > 7) return JsonErr(res, 400, "preset must be 0-7");
            auto* cmd = new PTZHttpCommand(PTZHttpCommand::SavePreset, cam, preset);
            Dispatch(hwnd, cmd);
            bool ok = cmd->success;
            delete cmd;
            if (!ok) return JsonErr(res, 400, "invalid camera index");
            JsonOk(res, "{\"success\":true}");
        });
    }

    HWND            m_hwnd;
    int             m_port;
    httplib::Server m_svr;
    std::thread     m_thread;
};

// ---------------------------------------------------------------------------
// C-linkage factory functions used by PTZControlDlg without exposing httplib.h
// ---------------------------------------------------------------------------
PTZHttpServerHandle* PTZHttpServer_Create(HWND hwnd, int port)
{
    auto* h = new PTZHttpServerHandle();
    h->server = std::make_unique<PTZHttpServer>(hwnd, port);
    return h;
}

bool PTZHttpServer_Start(PTZHttpServerHandle* h)
{
    return h && h->server->Start();
}

void PTZHttpServer_Stop(PTZHttpServerHandle* h)
{
    if (h) h->server->Stop();
}

void PTZHttpServer_Delete(PTZHttpServerHandle* h)
{
    delete h;
}
