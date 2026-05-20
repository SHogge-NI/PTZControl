#pragma once

// HTTP command IDs dispatched to the main dialog
enum HttpCommandId
{
	HTTP_CMD_PAN_LEFT = 1,
	HTTP_CMD_PAN_RIGHT,
	HTTP_CMD_TILT_UP,
	HTTP_CMD_TILT_DOWN,
	HTTP_CMD_ZOOM_IN,
	HTTP_CMD_ZOOM_OUT,
	HTTP_CMD_STOP,
	HTTP_CMD_HOME,
	HTTP_CMD_PRESET_1,
	HTTP_CMD_PRESET_2,
	HTTP_CMD_PRESET_3,
	HTTP_CMD_PRESET_4,
	HTTP_CMD_PRESET_5,
	HTTP_CMD_PRESET_6,
	HTTP_CMD_PRESET_7,
	HTTP_CMD_PRESET_8,
	HTTP_CMD_SAVE_PRESET_1,
	HTTP_CMD_SAVE_PRESET_2,
	HTTP_CMD_SAVE_PRESET_3,
	HTTP_CMD_SAVE_PRESET_4,
	HTTP_CMD_SAVE_PRESET_5,
	HTTP_CMD_SAVE_PRESET_6,
	HTTP_CMD_SAVE_PRESET_7,
	HTTP_CMD_SAVE_PRESET_8,
	HTTP_CMD_CAMERA_1,
	HTTP_CMD_CAMERA_2,
	HTTP_CMD_CAMERA_3,
};

// Custom Windows message for HTTP commands
#define WM_HTTP_COMMAND (WM_APP + 100)

// Zoom hold timers
#define TIMER_HTTP_ZOOM_REPEAT		4718	// Repeated zoom steps during hold
#define TIMER_HTTP_ZOOM_STOP		4719	// Stop zoom repeating
#define HTTP_ZOOM_REPEAT_INTERVAL	100		// Zoom step every 100ms during hold

class CHttpServer
{
public:
	CHttpServer();
	~CHttpServer();

	bool Start(HWND hWndTarget, int nPort = 8989);
	void Stop();
	bool IsRunning() const { return m_bRunning; }
	int GetPort() const { return m_nPort; }

private:
	static UINT AFX_CDECL ServerThread(LPVOID pParam);

	HWND m_hWndTarget;		// Window to receive WM_HTTP_COMMAND messages
	int m_nPort;
	bool m_bRunning;
	CWinThread* m_pThread;
	void* m_pServer;		// httplib::Server* (opaque to avoid header in .h)
};
