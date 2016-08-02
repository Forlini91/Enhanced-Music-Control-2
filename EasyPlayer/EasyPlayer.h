#if !defined(AFX_EASYPLAYER_H_INCLUDED_)
#define AFX_EASYPLAYER_H_INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// EasyPlayer.h : header file
//

#include <afxmt.h>

#pragma comment(lib, "vfw32.lib")
/////////////////////////////////////////////////////////////////////////////
// CEasyPlayer window

class CEasyPlayer : public CWnd
{
public:
	CEasyPlayer();
	CEasyPlayer(HWND hwParent);

public:
	HWND hwndParent;

public:
	long lPos;
	void Rwd();
	void Fwd();
	void DecreaseVolume();
	void IncreaseVolume();
	long GetVolume();
	long lVolume;
	void SetVolume(long lVol);
	long GetLength();
	long GetPosition();
	void SetPosition(long lPos);
	HWND GetWindowHandle();
	void Resume();
	void Kill();
	void Break();
	void Loop();
	void Close();
	HWND Initialize();
	void Play();
	void Stop();
	void Pause();
	long GetMode();
	CString GetPath();
	void SetPath(CString sPath);
	void SetParent(HWND hParent);
	HWND GetParent();
	virtual ~CEasyPlayer();

protected:
	HWND c_Player;
	CEvent m_Event;
	CString m_sPath;

	//{{AFX_MSG(CEasyPlayer)
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


class CParams
{
public:
	CParams() {};
	virtual ~CParams() {};

	CWnd* pWnd;
	HWND  hWnd;
	CEvent* pEvent;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EASYPLAYER_H_INCLUDED_)
