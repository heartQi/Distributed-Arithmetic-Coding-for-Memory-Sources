// Profiler.cpp: implementation of the CProfiler class.
//
//////////////////////////////////////////////////////////////////////

#include "wtypes.h"
#include "Profiler.h"


CProfiler::CProfiler()
{
    m_bTimerStopped = TRUE;
    m_llQPFTicksPerSec = 0;

    LARGE_INTEGER qwTicksPerSec;
    m_bUsingQPF = QueryPerformanceFrequency(&qwTicksPerSec) != 0;
    if (m_bUsingQPF)
    {
        m_llQPFTicksPerSec = qwTicksPerSec.QuadPart;
        m_llStopTime = 0;
        m_llLastElapsedTime = 0;
        m_llBaseTime = 0;
    }
    else
    {
        m_fLastElapsedTime = 0.0;
        m_fBaseTime = 0.0;
        m_fStopTime = 0.0;
    }
}

void CProfiler::QPFPrepare()
{
    if (m_llStopTime != 0)
        m_qwTime.QuadPart = m_llStopTime;
    else
        QueryPerformanceCounter(&m_qwTime);
}

void CProfiler::Prepare()
{
    if (m_fStopTime != 0.0)
        m_fTime = m_fStopTime;
    else

        m_fTime = timeGetTime() * 0.001;
}

void CProfiler::Reset()
{
    if (m_bUsingQPF)
    {
        QPFPrepare();
        m_llBaseTime        = m_qwTime.QuadPart;
        m_llLastElapsedTime = m_qwTime.QuadPart;
        m_llStopTime        = 0;
        m_bTimerStopped     = FALSE;
    }
    else
    {
        Prepare();
        m_fBaseTime         = m_fTime;
        m_fLastElapsedTime  = m_fTime;
        m_fStopTime         = 0;
        m_bTimerStopped     = FALSE;
    }
}

void CProfiler::Start()
{
    if (m_bUsingQPF)
    {
        QueryPerformanceCounter(&m_qwTime);
        if (m_bTimerStopped)
            m_llBaseTime += m_qwTime.QuadPart - m_llStopTime;
        m_llStopTime = 0;
        m_llLastElapsedTime = m_qwTime.QuadPart;
        m_bTimerStopped = FALSE;
    }
    else
    {
        m_fTime = timeGetTime() * 0.001;
        if (m_bTimerStopped)
            m_fBaseTime += m_fTime - m_fStopTime;
        m_fStopTime = 0.0f;
        m_fLastElapsedTime  = m_fTime;
        m_bTimerStopped = FALSE;
    }
}

void CProfiler::Stop()
{
    if (m_bUsingQPF)
    {
        QPFPrepare();
        m_llStopTime = m_qwTime.QuadPart;
        m_llLastElapsedTime = m_qwTime.QuadPart;
        m_bTimerStopped = TRUE;
    }
    else
    {
        Prepare();
        m_fStopTime = m_fTime;
        m_fLastElapsedTime  = m_fTime;
        m_bTimerStopped = TRUE;
    }
}

void CProfiler::Advance()
{
    if (m_bUsingQPF)
    {
        QPFPrepare();
        m_llStopTime += m_llQPFTicksPerSec / 10;
    }
    else
    {
        Prepare();
        m_fStopTime += 0.1f;
    }
}

REAL CProfiler::GetAbsTime()
{
    if (m_bUsingQPF)
    {
        QueryPerformanceCounter(&m_qwTime);
        return (REAL)(m_qwTime.QuadPart / (REAL)m_llQPFTicksPerSec);
    }
    else
    {
        m_fTime = timeGetTime() * 0.001;
        return m_fTime;
    }
}

REAL CProfiler::GetAppTime()
{
    if (m_bUsingQPF)
    {
        QPFPrepare();
        REAL fAppTime = (REAL)(m_qwTime.QuadPart - m_llBaseTime) / (REAL)m_llQPFTicksPerSec;
        return fAppTime;
    }
    else
    {
        Prepare();
        return (REAL)(m_fTime - m_fBaseTime);
    }
}

DWORD CProfiler::GetAppMS()
{
    if (m_bUsingQPF)
    {
        QPFPrepare();
        return (DWORD)((m_qwTime.QuadPart - m_llBaseTime) * 1000 / m_llQPFTicksPerSec);
    }
    else
    {
        Prepare();
        return (DWORD)((m_fTime - m_fBaseTime) * 1000);
    }
}

REAL CProfiler::GetElapsedTime()
{
    REAL fElapsedTime;

    if (m_bUsingQPF)
    {

        QPFPrepare();
        fElapsedTime = (REAL)(m_qwTime.QuadPart - m_llLastElapsedTime) / (REAL)m_llQPFTicksPerSec;
        m_llLastElapsedTime = m_qwTime.QuadPart;
        return (REAL)fElapsedTime;
    }
    else
    {
        Prepare();
        fElapsedTime = (REAL)(m_fTime - m_fLastElapsedTime);
        m_fLastElapsedTime = m_fTime;
        return (REAL)fElapsedTime;
    }
}
