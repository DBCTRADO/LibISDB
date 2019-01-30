/*
  LibISDB
  Copyright(c) 2017-2019 DBCTRADO

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/**
 @file   EVRScheduler.hpp
 @brief  EVR スケジューラ
 @author DBCTRADO
*/


#ifndef LIBISDB_EVR_SCHEDULER_H
#define LIBISDB_EVR_SCHEDULER_H


namespace LibISDB::DirectShow
{

	/** EVR スケジューラコールバッククラス */
	struct EVRSchedulerCallback
	{
		virtual HRESULT PresentSample(IMFSample *pSample, LONGLONG llTarget) = 0;
	};

	/** EVR スケジューラクラス */
	class EVRScheduler
	{
	public:
		EVRScheduler();
		virtual ~EVRScheduler();

		void SetCallback(EVRSchedulerCallback *pCallback) { m_pCallback = pCallback; }

		void SetFrameRate(const MFRatio &fps);
		void SetClockRate(float Rate) { m_Rate = Rate; }

		LONGLONG LastSampleTime() const { return m_LastSampleTime; }
		LONGLONG FrameDuration() const { return m_PerFrameInterval; }

		HRESULT StartScheduler(IMFClock *pClock);
		HRESULT StopScheduler();

		HRESULT ScheduleSample(IMFSample *pSample, bool bPresentNow);
		HRESULT ProcessSamplesInQueue(LONG *pNextSleep);
		HRESULT ProcessSample(IMFSample *pSample, LONG *pNextSleep);
		HRESULT Flush();

	private:
		static unsigned int __stdcall SchedulerThreadProc(void *pParameter);

		unsigned int SchedulerThread();

		ThreadSafeQueue<IMFSample> m_ScheduledSamples;

		COMPointer<IMFClock> m_Clock;
		EVRSchedulerCallback *m_pCallback;

		DWORD m_ThreadID;
		HANDLE m_hSchedulerThread;
		HANDLE m_hThreadReadyEvent;
		HANDLE m_hFlushEvent;

		float m_Rate;
		MFTIME m_PerFrameInterval;
		LONGLONG m_PerFrame_1_4th;
		MFTIME m_LastSampleTime;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_EVR_SCHEDULER_H
