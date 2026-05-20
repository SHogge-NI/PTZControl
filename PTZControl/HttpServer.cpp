#include "pch.h"
#include "HttpServer.h"

// cpp-httplib requires winsock, ensure we link it
#pragma comment(lib, "ws2_32.lib")

// Disable warnings from the third-party header
#pragma warning(push)
#pragma warning(disable: 4267 4244 4996)
#include "httplib.h"
#pragma warning(pop)

CHttpServer::CHttpServer()
	: m_hWndTarget(NULL)
	, m_nPort(8989)
	, m_bRunning(false)
	, m_pThread(nullptr)
	, m_pServer(nullptr)
{
}

CHttpServer::~CHttpServer()
{
	Stop();
}

bool CHttpServer::Start(HWND hWndTarget, int nPort)
{
	if (m_bRunning)
		return true;

	m_hWndTarget = hWndTarget;
	m_nPort = nPort;

	m_pThread = AfxBeginThread(&ServerThread, this, THREAD_PRIORITY_NORMAL, 0, 0);
	if (!m_pThread)
		return false;

	return true;
}

void CHttpServer::Stop()
{
	if (m_pServer)
	{
		auto* pSvr = static_cast<httplib::Server*>(m_pServer);
		pSvr->stop();
	}

	if (m_pThread)
	{
		::WaitForSingleObject(m_pThread->m_hThread, 5000);
		m_pThread = nullptr;
	}

	m_bRunning = false;
}

UINT AFX_CDECL CHttpServer::ServerThread(LPVOID pParam)
{
	auto* pThis = static_cast<CHttpServer*>(pParam);
	httplib::Server svr;
	pThis->m_pServer = &svr;

	// Helper to post a command and return 200
	// lParam carries optional hold duration in milliseconds (0 = single pulse)
	auto PostCmd = [&](HttpCommandId cmd, LPARAM durationMs = 0) {
		::PostMessage(pThis->m_hWndTarget, WM_HTTP_COMMAND, (WPARAM)cmd, durationMs);
	};

	// Helper to extract optional duration_ms query param
	auto GetDuration = [](const httplib::Request& req) -> int {
		if (req.has_param("duration_ms"))
		{
			try { return std::stoi(req.get_param_value("duration_ms")); }
			catch (...) { return 0; }
		}
		return 0;
	};

	// --- Pan/Tilt ---
	svr.Post("/api/pan/left", [&](const httplib::Request& req, httplib::Response& res) {
		PostCmd(HTTP_CMD_PAN_LEFT, GetDuration(req));
		res.status = 200;
	});
	svr.Post("/api/pan/right", [&](const httplib::Request& req, httplib::Response& res) {
		PostCmd(HTTP_CMD_PAN_RIGHT, GetDuration(req));
		res.status = 200;
	});
	svr.Post("/api/tilt/up", [&](const httplib::Request& req, httplib::Response& res) {
		PostCmd(HTTP_CMD_TILT_UP, GetDuration(req));
		res.status = 200;
	});
	svr.Post("/api/tilt/down", [&](const httplib::Request& req, httplib::Response& res) {
		PostCmd(HTTP_CMD_TILT_DOWN, GetDuration(req));
		res.status = 200;
	});

	// --- Zoom ---
	svr.Post("/api/zoom/in", [&](const httplib::Request& req, httplib::Response& res) {
		PostCmd(HTTP_CMD_ZOOM_IN, GetDuration(req));
		res.status = 200;
	});
	svr.Post("/api/zoom/out", [&](const httplib::Request& req, httplib::Response& res) {
		PostCmd(HTTP_CMD_ZOOM_OUT, GetDuration(req));
		res.status = 200;
	});

	// --- Home ---
	svr.Post("/api/home", [&](const httplib::Request&, httplib::Response& res) {
		PostCmd(HTTP_CMD_HOME);
		res.status = 200;
	});

	// --- Presets (goto) ---
	svr.Post(R"(/api/preset/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
		int preset = std::stoi(req.matches[1]);
		if (preset >= 1 && preset <= 8)
		{
			PostCmd(static_cast<HttpCommandId>(HTTP_CMD_PRESET_1 + preset - 1));
			res.status = 200;
		}
		else
			res.status = 400;
	});

	// --- Presets (save) ---
	svr.Post(R"(/api/preset/(\d+)/save)", [&](const httplib::Request& req, httplib::Response& res) {
		int preset = std::stoi(req.matches[1]);
		if (preset >= 1 && preset <= 8)
		{
			PostCmd(static_cast<HttpCommandId>(HTTP_CMD_SAVE_PRESET_1 + preset - 1));
			res.status = 200;
		}
		else
			res.status = 400;
	});

	// --- Camera switch ---
	svr.Post(R"(/api/camera/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
		int cam = std::stoi(req.matches[1]);
		if (cam >= 1 && cam <= 3)
		{
			PostCmd(static_cast<HttpCommandId>(HTTP_CMD_CAMERA_1 + cam - 1));
			res.status = 200;
		}
		else
			res.status = 400;
	});

	// --- Status ---
	svr.Get("/api/status", [&](const httplib::Request&, httplib::Response& res) {
		res.set_content("OK", "text/plain");
		res.status = 200;
	});

	// Start listening
	pThis->m_bRunning = true;
	svr.listen("127.0.0.1", pThis->m_nPort);
	pThis->m_bRunning = false;
	pThis->m_pServer = nullptr;

	return 0;
}
