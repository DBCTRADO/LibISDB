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
 @file   EVRPresenter.hpp
 @brief  EVR プレゼンタ
 @author DBCTRADO
*/


#ifndef LIBISDB_EVR_PRESENTER_H
#define LIBISDB_EVR_PRESENTER_H


#include "EVRPresentEngine.hpp"
#include <memory>


namespace LibISDB::DirectShow
{

	/** EVR プレゼンタクラス */
	class EVRPresenter
		: public IMFVideoDeviceID
		, public IMFVideoPresenter
		, public IMFRateSupport
		, public IMFGetService
		, public IMFTopologyServiceLookupClient
		, public IMFVideoDisplayControl
		, private RefCountedObject
	{
	public:
		static HRESULT CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv);

	// IUnknown
		STDMETHODIMP QueryInterface(REFIID riid, void **ppv) override;
		STDMETHODIMP_(ULONG) AddRef() override;
		STDMETHODIMP_(ULONG) Release() override;

	// IMFGetService
		STDMETHODIMP GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject) override;

	// IMFVideoPresenter
		STDMETHODIMP ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam) override;
		STDMETHODIMP GetCurrentMediaType(IMFVideoMediaType **ppMediaType) override;

	// IMFClockStateSink
		STDMETHODIMP OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset) override;
		STDMETHODIMP OnClockStop(MFTIME hnsSystemTime) override;
		STDMETHODIMP OnClockPause(MFTIME hnsSystemTime) override;
		STDMETHODIMP OnClockRestart(MFTIME hnsSystemTime) override;
		STDMETHODIMP OnClockSetRate(MFTIME hnsSystemTime, float flRate) override;

	// IMFRateSupport
		STDMETHODIMP GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL bThin, float *pflRate) override;
		STDMETHODIMP GetFastestRate(MFRATE_DIRECTION eDirection, BOOL bThin, float *pflRate) override;
		STDMETHODIMP IsRateSupported(BOOL bThin, float flRate, float *pflNearestSupportedRate) override;

	// IMFVideoDeviceID
		STDMETHODIMP GetDeviceID(IID *pDeviceID) override;

	// IMFTopologyServiceLookupClient
		STDMETHODIMP InitServicePointers(IMFTopologyServiceLookup *pLookup) override;
		STDMETHODIMP ReleaseServicePointers() override;

	// IMFVideoDisplayControl
		STDMETHODIMP GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo) override;
		STDMETHODIMP GetIdealVideoSize(SIZE *pszMin, SIZE *pszMax) override { return E_NOTIMPL; }
		STDMETHODIMP SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest) override;
		STDMETHODIMP GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest) override;
		STDMETHODIMP SetAspectRatioMode(DWORD dwAspectRatioMode) override;
		STDMETHODIMP GetAspectRatioMode(DWORD *pdwAspectRatioMode) override;
		STDMETHODIMP SetVideoWindow(HWND hwndVideo) override;
		STDMETHODIMP GetVideoWindow(HWND *phwndVideo) override;
		STDMETHODIMP RepaintVideo() override;
		STDMETHODIMP GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp) override;
		STDMETHODIMP SetBorderColor(COLORREF Clr) override { return E_NOTIMPL; }
		STDMETHODIMP GetBorderColor(COLORREF *pClr) override { return E_NOTIMPL; }
		STDMETHODIMP SetRenderingPrefs(DWORD dwRenderFlags) override;
		STDMETHODIMP GetRenderingPrefs(DWORD *pdwRenderFlags) override;
		STDMETHODIMP SetFullscreen(BOOL bFullscreen) override { return E_NOTIMPL; }
		STDMETHODIMP GetFullscreen(BOOL *pbFullscreen) override { return E_NOTIMPL; }

	protected:
		enum class RenderState {
			Started,
			Stopped,
			Paused,
			Shutdown,
		};

		enum class FrameStepState {
			None,
			WaitingStart,
			Pending,
			Scheduled,
			Complete,
		};

		EVRPresenter(HRESULT &hr);
		virtual ~EVRPresenter();

		inline HRESULT CheckShutdown() const
		{
			if (m_RenderState == RenderState::Shutdown) {
				return MF_E_SHUTDOWN;
			}
			return S_OK;
		}

		inline bool IsActive() const
		{
			return (m_RenderState == RenderState::Started) || (m_RenderState == RenderState::Paused);
		}

		inline bool IsScrubbing() const { return m_fRate == 0.0f; }

		void NotifyEvent(long EventCode, LONG_PTR Param1, LONG_PTR Param2)
		{
			if (m_MediaEventSink) {
				m_MediaEventSink->Notify(EventCode, Param1, Param2);
			}
		}

		float GetMaxRate(BOOL bThin);

		HRESULT ConfigureMixer(IMFTransform *pMixer);

		HRESULT CreateOptimalVideoType(IMFMediaType* pProposed, IMFMediaType **ppOptimal);
		HRESULT CalculateOutputRectangle(IMFMediaType *pProposed, RECT *prcOutput);
		HRESULT SetMediaType(IMFMediaType *pMediaType);
		HRESULT IsMediaTypeSupported(IMFMediaType *pMediaType);

		HRESULT Flush();
		HRESULT RenegotiateMediaType();
		HRESULT ProcessInputNotify();
		HRESULT BeginStreaming();
		HRESULT EndStreaming();
		HRESULT CheckEndOfStream();

		void ProcessOutputLoop();
		HRESULT ProcessOutput();
		HRESULT DeliverSample(IMFSample *pSample, BOOL bRepaint);
		HRESULT TrackSample(IMFSample *pSample);
		void ReleaseResources();

		HRESULT PrepareFrameStep(DWORD cSteps);
		HRESULT StartFrameStep();
		HRESULT DeliverFrameStepSample(IMFSample *pSample);
		HRESULT CompleteFrameStep(IMFSample *pSample);
		HRESULT CancelFrameStep();

		HRESULT OnSampleFree(IMFAsyncResult *pResult);

		AsyncCallback<EVRPresenter> m_SampleFreeCB;

		struct FrameStep {
			FrameStepState State = FrameStepState::None;
			VideoSampleList Samples;
			DWORD Steps = 0;
			IUnknown *pSampleNoRef = nullptr;
		};

		RenderState m_RenderState;
		FrameStep m_FrameStep;

		MutexLock m_ObjectLock;

		EVRScheduler m_Scheduler;
		SamplePool m_SamplePool;
		DWORD m_TokenCounter;

		bool m_SampleNotify;
		bool m_Repaint;
		bool m_Prerolled;
		bool m_EndStreaming;

		SIZE m_NativeVideoSize;
		MFRatio m_NativeAspectRatio;

		MFVideoNormalizedRect m_nrcSource;
		float m_fRate;

		DWORD m_AspectRatioMode;
		DWORD m_RenderPrefs;

		std::unique_ptr<EVRPresentEngine> m_PresentEngine;

		COMPointer<IMFClock> m_Clock;
		COMPointer<IMFTransform> m_Mixer;
		COMPointer<IMediaEventSink> m_MediaEventSink;
		COMPointer<IMFMediaType> m_MediaType;
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_EVR_PRESENTER_H
