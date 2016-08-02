// EasyPlayer.cpp : implementation file
//

#include "stdafx.h"

#include "EasyPlayer.h"
#include "vfw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


UINT LoopThread(LPVOID pParam);
/////////////////////////////////////////////////////////////////////////////
// CEasyPlayer

CEasyPlayer::CEasyPlayer(HWND hwParent)
{
	hwndParent=hwParent;

	m_sPath="";
	c_Player=NULL;
}

CEasyPlayer::CEasyPlayer()
{
	hwndParent=NULL;

	m_sPath="";
	c_Player=NULL;;
}

CEasyPlayer::~CEasyPlayer()
{
	MCIWndDestroy(c_Player);
	c_Player=NULL;
	m_sPath="";
}

BEGIN_MESSAGE_MAP(CEasyPlayer, CWnd)
	//{{AFX_MSG_MAP(CEasyPlayer)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEasyPlayer message handlers

void CEasyPlayer::OnDestroy() 
{
	CWnd::OnDestroy();
	
	MCIWndDestroy(c_Player);
	
}

void CEasyPlayer::SetParent(HWND hParent)
{
	hwndParent=hParent;
}

HWND CEasyPlayer::GetParent()
{
	return hwndParent;
}

void CEasyPlayer::SetPath(CString sPath)
{
	m_sPath=sPath;
}

CString CEasyPlayer::GetPath()
{
	return m_sPath;
}

long CEasyPlayer::GetMode()
{
	char lp[3];
	long lMode;
	
	lMode=MCIWndGetMode(c_Player,lp,sizeof(lp));

	return lMode;
}

void CEasyPlayer::Kill()
{
	MCIWndClose(c_Player);
	MCIWndDestroy(c_Player);
}

void CEasyPlayer::Resume()
{
	MCIWndResume(c_Player);
}

void CEasyPlayer::Play()
{
	MCIWndPlay(c_Player);
}

void CEasyPlayer::Stop()
{
	MCIWndStop(c_Player);
}

void CEasyPlayer::Pause()
{
	MCIWndPause(c_Player);
}

HWND CEasyPlayer::Initialize()
{
	c_Player=MCIWndCreate(hwndParent,AfxGetInstanceHandle(),
		MCIWNDF_NOMENU| MCIWNDF_NOTIFYALL| MCIWNDF_NOPLAYBAR,
		m_sPath);
	MCIWndSetTimeFormat(c_Player,"frames");
	MCIWndSetActiveTimer(c_Player,100);
	lVolume=MCIWndGetVolume(c_Player);

	return c_Player;
}

void CEasyPlayer::Close()
{
	MCIWndClose(c_Player);
}

UINT LoopThread(LPVOID pParam)
{
	CParams* pParameters;
	pParameters=(CParams*)pParam;

	CEasyPlayer* pWnd=(CEasyPlayer*)pParameters->pWnd;
	CEvent* pEvent=(CEvent*)pParameters->pEvent;
	HWND c_Player=(HWND)pParameters->hWnd;

	while(WaitForSingleObject(pEvent->m_hObject,5)==WAIT_TIMEOUT)
	{
		if (pWnd->GetMode()==MCI_MODE_STOP)
		::PostMessage(c_Player, MCI_PLAY, 0, 0);
	}

	return 0;
}

void CEasyPlayer::Loop()
{
	CParams Params;

	m_Event.ResetEvent();

	Params.pWnd=(CEasyPlayer*)this;
	Params.pEvent=&m_Event;
	Params.hWnd=c_Player;

	AfxBeginThread(LoopThread,&Params);
}

void CEasyPlayer::Break()
{
	m_Event.SetEvent();
	Stop();
}

HWND CEasyPlayer::GetWindowHandle()
{
	return c_Player;
}

void CEasyPlayer::SetPosition(long lPos)
{
	MCIWndSeek(c_Player,lPos);
}

long CEasyPlayer::GetPosition()
{
	return MCIWndGetPosition(c_Player);
}

long CEasyPlayer::GetLength()
{
	return MCIWndGetLength(c_Player);
}

void CEasyPlayer::SetVolume(long lVol)
{
	lVolume=lVol;
	MCIWndSetVolume(c_Player,lVol);
}

long CEasyPlayer::GetVolume()
{
	return lVolume;
}

void CEasyPlayer::IncreaseVolume()
{
	lVolume+=50;
	if (lVolume>=1000)
		lVolume=1000;

	SetVolume(lVolume);
}

void CEasyPlayer::DecreaseVolume()
{
	lVolume-=50;
	if (lVolume<=0)
		lVolume=0;

	SetVolume(lVolume);
}

void CEasyPlayer::Fwd()
{
	lPos=GetPosition();
	lPos+=10000;
	if (lPos>GetLength())
		lPos=GetLength();

	SetPosition(lPos);
}

void CEasyPlayer::Rwd()
{
	lPos=GetPosition();
	lPos-=10000;
	if (lPos<0)
		lPos=0;

	SetPosition(lPos);
}
