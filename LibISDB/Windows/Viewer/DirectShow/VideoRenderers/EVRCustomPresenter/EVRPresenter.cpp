/*
  LibISDB
  Copyright(c) 2017-2020 DBCTRADO

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
 @file   EVRPresenter.cpp
 @brief  EVR プレゼンタ
 @author DBCTRADO
*/


#include "../../../../../LibISDBPrivate.hpp"
#include "../../../../../LibISDBWindows.hpp"
#include "EVRPresenterBase.hpp"
#include "EVRPresenter.hpp"
#include "../../../../../Base/DebugDef.hpp"


namespace LibISDB::DirectShow
{

namespace
{


#ifndef LODWORD
constexpr DWORD LODWORD(UINT64 v) { return static_cast<DWORD>(v & 0xFFFFFFFF_u64); }
#endif
#ifndef HIDWORD
constexpr DWORD HIDWORD(UINT64 v) { return static_cast<DWORD>(v >> 32); }
#endif


inline float MFOffsetToFloat(const MFOffset &Offset)
{
	return static_cast<float>(Offset.value) + (static_cast<float>(Offset.fract) / 65536.0f);
}


RECT CorrectAspectRatio(const RECT &SrcRect, const MFRatio &SrcPAR, const MFRatio &DestPAR)
{
	RECT rc = {0, 0, SrcRect.right - SrcRect.left, SrcRect.bottom - SrcRect.top};

	if ((SrcPAR.Numerator != DestPAR.Numerator) || (SrcPAR.Denominator != DestPAR.Denominator)) {
		if (SrcPAR.Numerator > SrcPAR.Denominator) {
			rc.right = ::MulDiv(rc.right, SrcPAR.Numerator, SrcPAR.Denominator);
		} else if (SrcPAR.Numerator < SrcPAR.Denominator) {
			rc.bottom = ::MulDiv(rc.bottom, SrcPAR.Denominator, SrcPAR.Numerator);
		}

		if (DestPAR.Numerator > DestPAR.Denominator) {
			rc.bottom = ::MulDiv(rc.bottom, DestPAR.Numerator, DestPAR.Denominator);
		} else if (DestPAR.Numerator < DestPAR.Denominator) {
			rc.right = ::MulDiv(rc.right, DestPAR.Denominator, DestPAR.Numerator);
		}
	}

	return rc;
}


bool AreMediaTypesEqual(IMFMediaType *pType1, IMFMediaType *pType2)
{
	if (pType1 == pType2) {
		return true;
	}
	if ((pType1 == nullptr) || (pType2 == nullptr)) {
		return false;
	}

	DWORD Flags = 0;
	HRESULT hr = pType1->IsEqual(pType2, &Flags);

	return hr == S_OK;
}


HRESULT ValidateVideoArea(const MFVideoArea &Area, UINT32 Width, UINT32 Height)
{
	float OffsetX = MFOffsetToFloat(Area.OffsetX);
	float OffsetY = MFOffsetToFloat(Area.OffsetY);

	if ((static_cast<LONG>(OffsetX) + Area.Area.cx > static_cast<LONG>(Width)) ||
		(static_cast<LONG>(OffsetY) + Area.Area.cy > static_cast<LONG>(Height))) {
		return MF_E_INVALIDMEDIATYPE;
	}

	return S_OK;
}


HRESULT SetDesiredSampleTime(IMFSample *pSample, LONGLONG hnsSampleTime, LONGLONG hnsDuration)
{
	if (pSample == nullptr) {
		return E_POINTER;
	}

	HRESULT hr;
	IMFDesiredSample *pDesired = nullptr;

	hr = pSample->QueryInterface(IID_PPV_ARGS(&pDesired));
	if (SUCCEEDED(hr)) {
		pDesired->SetDesiredSampleTimeAndDuration(hnsSampleTime, hnsDuration);
		pDesired->Release();
	}

	return hr;
}


HRESULT ClearDesiredSampleTime(IMFSample *pSample)
{
	if (pSample == nullptr) {
		return E_POINTER;
	}

	HRESULT hr;

	IUnknown *pUnkSwapChain = nullptr;
	IMFDesiredSample *pDesired = nullptr;

	UINT32 Counter = ::MFGetAttributeUINT32(pSample, SampleAttribute_Counter, (UINT32)-1);

	pSample->GetUnknown(SampleAttribute_SwapChain, IDD_PPV_ARGS_IUNKNOWN(&pUnkSwapChain));

	hr = pSample->QueryInterface(IID_PPV_ARGS(&pDesired));
	if (SUCCEEDED(hr)) {
		pDesired->Clear();

		hr = pSample->SetUINT32(SampleAttribute_Counter, Counter);
		if (SUCCEEDED(hr)) {
			if (pUnkSwapChain != nullptr) {
				hr = pSample->SetUnknown(SampleAttribute_SwapChain, pUnkSwapChain);
			}
		}

		pDesired->Release();
	}

	SafeRelease(pUnkSwapChain);

	return hr;
}


bool IsSampleTimePassed(IMFClock *pClock, IMFSample *pSample)
{
	if ((pSample == nullptr) || (pClock == nullptr)) {
		return false;
	}

	HRESULT hr;
	MFTIME hnsTimeNow = 0;
	MFTIME hnsSystemTime = 0;
	MFTIME hnsSampleStart = 0;
	MFTIME hnsSampleDuration = 0;

	hr = pClock->GetCorrelatedTime(0, &hnsTimeNow, &hnsSystemTime);

	if (SUCCEEDED(hr)) {
		hr = pSample->GetSampleTime(&hnsSampleStart);
	}
	if (SUCCEEDED(hr)) {
		hr = pSample->GetSampleDuration(&hnsSampleDuration);
	}

	if (SUCCEEDED(hr)) {
		if (hnsSampleStart + hnsSampleDuration < hnsTimeNow) {
			return true;
		}
	}

	return false;
}


HRESULT SetMixerSourceRect(IMFTransform *pMixer, const MFVideoNormalizedRect &nrcSource)
{
	if (pMixer == nullptr) {
		return E_POINTER;
	}

	HRESULT hr;
	IMFAttributes *pAttributes = nullptr;

	hr = pMixer->GetAttributes(&pAttributes);
	if (SUCCEEDED(hr)) {
		hr = pAttributes->SetBlob(VIDEO_ZOOM_RECT, (const UINT8*)&nrcSource, sizeof(nrcSource));
		pAttributes->Release();
	}

	return hr;
}


}




HRESULT EVRPresenter::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
	if (ppv == nullptr) {
		return E_POINTER;
	}

	*ppv = nullptr;

	if (pUnkOuter != nullptr) {
		return CLASS_E_NOAGGREGATION;
	}

	HRESULT hr = S_OK;

	EVRPresenter *pPresenter;
	try {
		pPresenter = new EVRPresenter(hr);
	} catch (...) {
		return E_OUTOFMEMORY;
	}

	hr = pPresenter->QueryInterface(riid, ppv);

	pPresenter->Release();

	return hr;
}


LIBISDB_PRAGMA_MSVC(warning(disable : 4355))	// 'this' used in base member initializer list

EVRPresenter::EVRPresenter(HRESULT &hr)
	: m_RenderState(RenderState::Shutdown)
	, m_SampleNotify(false)
	, m_Repaint(false)
	, m_EndStreaming(false)
	, m_Prerolled(false)
	, m_NativeVideoSize()
	, m_NativeAspectRatio()
	, m_fRate(1.0f)
	, m_AspectRatioMode(MFVideoARMode_PreservePicture)
	, m_RenderPrefs(0)
	, m_TokenCounter(0)
	, m_SampleFreeCB(this, &EVRPresenter::OnSampleFree)
{
	hr = S_OK;

	m_nrcSource.top    = 0.0f;
	m_nrcSource.left   = 0.0f;
	m_nrcSource.bottom = 1.0f;
	m_nrcSource.right  = 1.0f;

	try {
		m_PresentEngine.reset(new EVRPresentEngine(hr));

		m_Scheduler.SetCallback(m_PresentEngine.get());
	} catch (...) {
		hr = E_OUTOFMEMORY;
	}
}


EVRPresenter::~EVRPresenter()
{
}


HRESULT EVRPresenter::QueryInterface(REFIID riid, void **ppv)
{
	if (ppv == nullptr) {
		return E_POINTER;
	}

	if (riid == __uuidof(IUnknown)) {
		*ppv = static_cast<IUnknown*>(static_cast<IMFVideoPresenter*>(this));
	} else if (riid == __uuidof(IMFVideoDeviceID)) {
		*ppv = static_cast<IMFVideoDeviceID*>(this);
	} else if (riid == __uuidof(IMFVideoPresenter)) {
		*ppv = static_cast<IMFVideoPresenter*>(this);
	} else if (riid == __uuidof(IMFClockStateSink)) {
		*ppv = static_cast<IMFClockStateSink*>(this);
	} else if (riid == __uuidof(IMFRateSupport)) {
		*ppv = static_cast<IMFRateSupport*>(this);
	} else if (riid == __uuidof(IMFGetService)) {
		*ppv = static_cast<IMFGetService*>(this);
	} else if (riid == __uuidof(IMFTopologyServiceLookupClient)) {
		*ppv = static_cast<IMFTopologyServiceLookupClient*>(this);
	} else if (riid == __uuidof(IMFVideoDisplayControl)) {
		*ppv = static_cast<IMFVideoDisplayControl*>(this);
	} else {
		*ppv = nullptr;
		return E_NOINTERFACE;
	}

	AddRef();

	return S_OK;
}


ULONG EVRPresenter::AddRef()
{
	return RefCountedObject::AddRef();
}


ULONG EVRPresenter::Release()
{
	return RefCountedObject::Release();
}


HRESULT EVRPresenter::GetService(REFGUID guidService, REFIID riid, LPVOID *ppvObject)
{
	if (ppvObject == nullptr) {
		return E_POINTER;
	}

	*ppvObject = nullptr;

	HRESULT hr = m_PresentEngine->GetService(guidService, riid, ppvObject);

	if (FAILED(hr)) {
		if (guidService != MR_VIDEO_RENDER_SERVICE) {
			return MF_E_UNSUPPORTED_SERVICE;
		}

		hr = QueryInterface(riid, ppvObject);
	}

	return hr;
}


HRESULT EVRPresenter::GetDeviceID(IID *pDeviceID)
{
	if (pDeviceID == nullptr) {
		return E_POINTER;
	}

	*pDeviceID = __uuidof(IDirect3DDevice9);

	return S_OK;
}


HRESULT EVRPresenter::InitServicePointers(IMFTopologyServiceLookup *pLookup)
{
	if (pLookup == nullptr) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	if (IsActive()) {
		return MF_E_INVALIDREQUEST;
	}

	m_Clock.Release();
	m_Mixer.Release();
	m_MediaEventSink.Release();

	HRESULT hr;

	DWORD ObjectCount = 1;

	pLookup->LookupService(
		MF_SERVICE_LOOKUP_GLOBAL,
		0,
		MR_VIDEO_RENDER_SERVICE,
		IID_PPV_ARGS(m_Clock.GetPP()),
		&ObjectCount
	);

	ObjectCount = 1;

	hr = pLookup->LookupService(
		MF_SERVICE_LOOKUP_GLOBAL,
		0,
		MR_VIDEO_MIXER_SERVICE,
		IID_PPV_ARGS(m_Mixer.GetPP()),
		&ObjectCount
	);
	if (FAILED(hr)) {
		return hr;
	}

	hr = ConfigureMixer(m_Mixer.Get());
	if (FAILED(hr)) {
		return hr;
	}

	ObjectCount = 1;

	hr = pLookup->LookupService(
		MF_SERVICE_LOOKUP_GLOBAL,
		0,
		MR_VIDEO_RENDER_SERVICE,
		IID_PPV_ARGS(m_MediaEventSink.GetPP()),
		&ObjectCount
	);
	if (FAILED(hr)) {
		return hr;
	}

	m_RenderState = RenderState::Stopped;

	return S_OK;
}


HRESULT EVRPresenter::ReleaseServicePointers()
{
	{
		BlockLock Lock(m_ObjectLock);
		m_RenderState = RenderState::Shutdown;
	}

	Flush();

	SetMediaType(nullptr);

	m_Clock.Release();
	m_Mixer.Release();
	m_MediaEventSink.Release();

	return S_OK;
}


HRESULT EVRPresenter::ProcessMessage(MFVP_MESSAGE_TYPE eMessage, ULONG_PTR ulParam)
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	switch (eMessage) {
	// 待機中のサンプルをフラッシュ
	case MFVP_MESSAGE_FLUSH:
		LIBISDB_TRACE(LIBISDB_STR("MFVP_MESSAGE_FLUSH\n"));
		hr = Flush();
		break;

	// メディアタイプの交渉
	case MFVP_MESSAGE_INVALIDATEMEDIATYPE:
		LIBISDB_TRACE(LIBISDB_STR("MFVP_MESSAGE_INVALIDATEMEDIATYPE\n"));
		hr = RenegotiateMediaType();
		break;

	// ミキサーがサンプルを受け取った
	case MFVP_MESSAGE_PROCESSINPUTNOTIFY:
		hr = ProcessInputNotify();
		break;

	// ストリーミング開始
	case MFVP_MESSAGE_BEGINSTREAMING:
		LIBISDB_TRACE(LIBISDB_STR("MFVP_MESSAGE_BEGINSTREAMING\n"));
		hr = BeginStreaming();
		break;

	// ストリームング終了(EVR停止)
	case MFVP_MESSAGE_ENDSTREAMING:
		LIBISDB_TRACE(LIBISDB_STR("MFVP_MESSAGE_ENDSTREAMING\n"));
		hr = EndStreaming();
		break;

	// 入力ストリーム終了
	case MFVP_MESSAGE_ENDOFSTREAM:
		LIBISDB_TRACE(LIBISDB_STR("MFVP_MESSAGE_ENDOFSTREAM\n"));
		m_EndStreaming = true; 
		hr = CheckEndOfStream();
		break;

	// フレーム飛ばし
	case MFVP_MESSAGE_STEP:
		hr = PrepareFrameStep(LODWORD(ulParam));
		break;

	// フレーム飛ばしキャンセル
	case MFVP_MESSAGE_CANCELSTEP:
		hr = CancelFrameStep();
		break;

	default:
		hr = E_INVALIDARG;
		break;
	}

	return hr;
}


HRESULT EVRPresenter::GetCurrentMediaType(IMFVideoMediaType **ppMediaType)
{
	if (ppMediaType == nullptr) {
		return E_POINTER;
	}

	*ppMediaType = nullptr;

	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (!m_MediaType) {
		return MF_E_NOT_INITIALIZED;
	}

	return m_MediaType->QueryInterface(IID_PPV_ARGS(ppMediaType));
}


HRESULT EVRPresenter::OnClockStart(MFTIME hnsSystemTime, LONGLONG llClockStartOffset)
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (IsActive()) {
		m_RenderState = RenderState::Started;

		if (llClockStartOffset != PRESENTATION_CURRENT_POSITION) {
			Flush();
		}
	} else {
		m_RenderState = RenderState::Started;

		hr = StartFrameStep();
		if (FAILED(hr)) {
			return hr;
		}
	}

	ProcessOutputLoop();

	return S_OK;
}


HRESULT EVRPresenter::OnClockRestart(MFTIME hnsSystemTime)
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	LIBISDB_ASSERT(m_RenderState == RenderState::Paused);

	m_RenderState = RenderState::Started;

	hr = StartFrameStep();
	if (FAILED(hr)) {
		return hr;
	}

	ProcessOutputLoop();

	return S_OK;
}


HRESULT EVRPresenter::OnClockStop(MFTIME hnsSystemTime)
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (m_RenderState != RenderState::Stopped) {
		m_RenderState = RenderState::Stopped;
		Flush();

		if (m_FrameStep.State != FrameStepState::None) {
			CancelFrameStep();
		}
	}

	return S_OK;
}


HRESULT EVRPresenter::OnClockPause(MFTIME hnsSystemTime)
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	m_RenderState = RenderState::Paused;

	return S_OK;
}


HRESULT EVRPresenter::OnClockSetRate(MFTIME hnsSystemTime, float fRate)
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if ((m_fRate == 0.0f) && (fRate != 0.0f)) {
		CancelFrameStep();
		m_FrameStep.Samples.Clear();
	}

	m_fRate = fRate;

	m_Scheduler.SetClockRate(fRate);

	return S_OK;
}


HRESULT EVRPresenter::GetSlowestRate(MFRATE_DIRECTION eDirection, BOOL bThin, float *pfRate)
{
	if (pfRate == nullptr) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	*pfRate = 0;

	return S_OK;
}


HRESULT EVRPresenter::GetFastestRate(MFRATE_DIRECTION eDirection, BOOL bThin, float *pfRate)
{
	if (pfRate == nullptr) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	HRESULT hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	float fMaxRate = GetMaxRate(bThin);

	if (eDirection == MFRATE_REVERSE) {
		fMaxRate = -fMaxRate;
	}

	*pfRate = fMaxRate;

	return S_OK;
}


HRESULT EVRPresenter::IsRateSupported(BOOL bThin, float fRate, float *pfNearestSupportedRate)
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	float fNearestRate = fRate;
	float fMaxRate = GetMaxRate(bThin);

	if (std::fabs(fRate) > fMaxRate) {
		hr = MF_E_UNSUPPORTED_RATE;

		fNearestRate = fMaxRate;
		if (fRate < 0.0) {
			fNearestRate = -fNearestRate;
		}
	}

	if (pfNearestSupportedRate != nullptr) {
		*pfNearestSupportedRate = fNearestRate;
	}

	return S_OK;
}


HRESULT EVRPresenter::SetVideoWindow(HWND hwndVideo)
{
	if (!::IsWindow(hwndVideo)) {
		return E_INVALIDARG;
	}

	BlockLock Lock(m_ObjectLock);

	HRESULT hr = S_OK;
	HWND hwndOld = m_PresentEngine->GetVideoWindow();

	if (hwndOld != hwndVideo) {
		hr = m_PresentEngine->SetVideoWindow(hwndVideo);

		NotifyEvent(EC_DISPLAY_CHANGED, 0, 0);
	}

	return hr;
}


HRESULT EVRPresenter::GetVideoWindow(HWND* phwndVideo)
{
	if (phwndVideo == nullptr) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	*phwndVideo = m_PresentEngine->GetVideoWindow();

	return S_OK;
}


HRESULT EVRPresenter::GetNativeVideoSize(SIZE *pszVideo, SIZE *pszARVideo)
{
	if ((pszVideo == nullptr) || (pszARVideo == nullptr)) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (pszVideo != nullptr) {
		*pszVideo = m_NativeVideoSize;
	}

	if (pszARVideo != nullptr) {
		pszARVideo->cx = m_NativeAspectRatio.Numerator;
		pszARVideo->cy = m_NativeAspectRatio.Denominator;
	}

	return S_OK;
}


HRESULT EVRPresenter::SetVideoPosition(const MFVideoNormalizedRect *pnrcSource, const LPRECT prcDest)
{
	if ((pnrcSource == nullptr) && (prcDest == nullptr)) {
		return E_POINTER;
	}

	if (pnrcSource != nullptr) {
		if ((pnrcSource->left > pnrcSource->right) ||
			(pnrcSource->top > pnrcSource->bottom)) {
			return E_INVALIDARG;
		}

		if ((pnrcSource->left < 0.0f) || (pnrcSource->right > 1.0f) ||
			(pnrcSource->top < 0.0f) || (pnrcSource->bottom > 1.0f)) {
			return E_INVALIDARG;
		}
	}

	if (prcDest != nullptr) {
		if ((prcDest->left > prcDest->right) ||
			(prcDest->top > prcDest->bottom)) {
			return E_INVALIDARG;
		}
	}

	BlockLock Lock(m_ObjectLock);

	HRESULT hr;

	bool bChanged = false;

	if (pnrcSource != nullptr) {
		if ((pnrcSource->left != m_nrcSource.left) ||
			(pnrcSource->top != m_nrcSource.top) ||
			(pnrcSource->right != m_nrcSource.right) ||
			(pnrcSource->bottom != m_nrcSource.bottom)) {
			m_nrcSource = *pnrcSource;

			if (m_Mixer) {
				hr = SetMixerSourceRect(m_Mixer.Get(), m_nrcSource);
				if (FAILED(hr)) {
					return hr;
				}
			}

			bChanged = true;
		}
	}

	if (prcDest != nullptr) {
		RECT rcOldDest = m_PresentEngine->GetDestinationRect();

		if (!::EqualRect(prcDest, &rcOldDest)) {
			hr = m_PresentEngine->SetDestinationRect(*prcDest);
			if (FAILED(hr)) {
				return hr;
			}

			bChanged = true;
		}
	}

	if (bChanged && m_Mixer) {
		hr = RenegotiateMediaType();
		if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) {
			hr = S_OK;
		} else {
			if (FAILED(hr)) {
				return hr;
			}

			m_Repaint = true;
			ProcessOutput();
		}
	}

	return S_OK;
}


HRESULT EVRPresenter::GetVideoPosition(MFVideoNormalizedRect *pnrcSource, LPRECT prcDest)
{
	if ((pnrcSource == nullptr) || (prcDest == nullptr)) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	*pnrcSource = m_nrcSource;
	*prcDest = m_PresentEngine->GetDestinationRect();

	return S_OK;
}


HRESULT EVRPresenter::SetAspectRatioMode(DWORD dwAspectRatioMode)
{
	if (dwAspectRatioMode & ~MFVideoARMode_Mask) {
		return E_INVALIDARG;
	}

	BlockLock Lock(m_ObjectLock);

	m_AspectRatioMode = dwAspectRatioMode;

	return S_OK;
}


HRESULT EVRPresenter::GetAspectRatioMode(DWORD *pdwAspectRatioMode)
{
	if (pdwAspectRatioMode == nullptr) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	*pdwAspectRatioMode = m_AspectRatioMode;

	return S_OK;
}


HRESULT EVRPresenter::RepaintVideo()
{
	BlockLock Lock(m_ObjectLock);

	HRESULT hr = S_OK;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (m_Prerolled) {
		m_Repaint = true;
		ProcessOutput();
	}

	return S_OK;
}


HRESULT EVRPresenter::GetCurrentImage(BITMAPINFOHEADER *pBih, BYTE **pDib, DWORD *pcbDib, LONGLONG *pTimeStamp)
{
	return m_PresentEngine->GetCurrentImage(pBih, pDib, pcbDib, pTimeStamp);
}


HRESULT EVRPresenter::SetRenderingPrefs(DWORD dwRenderFlags)
{
	if (dwRenderFlags & ~MFVideoRenderPrefs_Mask) {
		return E_INVALIDARG;
	}

	BlockLock Lock(m_ObjectLock);

	m_RenderPrefs = dwRenderFlags;

	return S_OK;
}


HRESULT EVRPresenter::GetRenderingPrefs(DWORD *pdwRenderFlags)
{
	if (pdwRenderFlags == nullptr) {
		return E_POINTER;
	}

	BlockLock Lock(m_ObjectLock);

	*pdwRenderFlags = m_RenderPrefs;

	return S_OK;
}


HRESULT EVRPresenter::ConfigureMixer(IMFTransform *pMixer)
{
	HRESULT hr;

	IMFVideoDeviceID *pDeviceID = nullptr;

	hr = pMixer->QueryInterface(IID_PPV_ARGS(&pDeviceID));

	if (SUCCEEDED(hr)) {
		IID DeviceID = GUID_NULL;

		hr = pDeviceID->GetDeviceID(&DeviceID);
		if (SUCCEEDED(hr)) {
			if (DeviceID == __uuidof(IDirect3DDevice9)) {
				SetMixerSourceRect(pMixer, m_nrcSource);
			} else {
				hr = MF_E_INVALIDREQUEST;
			}
		}

		pDeviceID->Release();
	}

	return hr;
}


HRESULT EVRPresenter::RenegotiateMediaType()
{
	if (!m_Mixer) {
		return MF_E_INVALIDREQUEST;
	}

	HRESULT hr;
	bool bFoundMediaType = false;

	DWORD TypeIndex = 0;

	do {
		IMFMediaType *pMixerType = nullptr;

		hr = m_Mixer->GetOutputAvailableType(0, TypeIndex++, &pMixerType);
		if (FAILED(hr)) {
			break;
		}

		hr = IsMediaTypeSupported(pMixerType);

		if (SUCCEEDED(hr)) {
			IMFMediaType *pOptimalType = nullptr;

			hr = CreateOptimalVideoType(pMixerType, &pOptimalType);

			if (SUCCEEDED(hr)) {
				hr = m_Mixer->SetOutputType(0, pOptimalType, MFT_SET_TYPE_TEST_ONLY);

				if (SUCCEEDED(hr)) {
					hr = SetMediaType(pOptimalType);

					if (SUCCEEDED(hr)) {
						hr = m_Mixer->SetOutputType(0, pOptimalType, 0);

						if (SUCCEEDED(hr)) {
							VideoType mt(pMixerType);

							mt.GetFrameDimensions((UINT32*)&m_NativeVideoSize.cx, (UINT32*)&m_NativeVideoSize.cy);
							m_NativeAspectRatio = mt.GetPixelAspectRatio();

							bFoundMediaType = true;
						} else {
							SetMediaType(nullptr);
						}
					}
				}

				pOptimalType->Release();
			}
		}

		pMixerType->Release();
	} while (!bFoundMediaType);

	return hr;
}


HRESULT EVRPresenter::Flush()
{
	m_Prerolled = false;

	m_Scheduler.Flush();

	m_FrameStep.Samples.Clear();

	if (m_RenderState == RenderState::Stopped) {
		m_PresentEngine->PresentSample(nullptr, 0);
	}

	return S_OK;
}


HRESULT EVRPresenter::ProcessInputNotify()
{
	HRESULT hr = S_OK;

	m_SampleNotify = true;

	if (!m_MediaType) {
		hr = MF_E_TRANSFORM_TYPE_NOT_SET;
	} else {
		ProcessOutputLoop();
	}

	return hr;
}


HRESULT EVRPresenter::BeginStreaming()
{
	return m_Scheduler.StartScheduler(m_Clock.Get());
}


HRESULT EVRPresenter::EndStreaming()
{
	return m_Scheduler.StopScheduler();
}


HRESULT EVRPresenter::CheckEndOfStream()
{
	if (!m_EndStreaming) {
		return S_OK;
	}

	if (m_SampleNotify) {
		return S_OK;
	}

	if (m_SamplePool.AreSamplesPending()) {
		return S_OK;
	}

	NotifyEvent(EC_COMPLETE, S_OK, 0);

	m_EndStreaming = false;

	return S_OK;
}


HRESULT EVRPresenter::PrepareFrameStep(DWORD cSteps)
{
	m_FrameStep.Steps += cSteps;
	m_FrameStep.State = FrameStepState::WaitingStart;

	HRESULT hr = S_OK;

	if (m_RenderState == RenderState::Started) {
		hr = StartFrameStep();
	}

	return hr;
}


HRESULT EVRPresenter::StartFrameStep()
{
	LIBISDB_ASSERT(m_RenderState == RenderState::Started);

	HRESULT hr = S_OK;

	if (m_FrameStep.State == FrameStepState::WaitingStart) {
		m_FrameStep.State = FrameStepState::Pending;

		while (!m_FrameStep.Samples.IsEmpty() && (m_FrameStep.State == FrameStepState::Pending)) {
			IMFSample *pSample = nullptr;

			hr = m_FrameStep.Samples.RemoveFront(&pSample);

			if (SUCCEEDED(hr)) {
				hr = DeliverFrameStepSample(pSample);

				pSample->Release();
			}

			if (FAILED(hr)) {
				break;
			}
		}
	} else if (m_FrameStep.State == FrameStepState::None) {
		while (!m_FrameStep.Samples.IsEmpty()) {
			IMFSample *pSample = nullptr;

			hr = m_FrameStep.Samples.RemoveFront(&pSample);

			if (SUCCEEDED(hr)) {
				hr = DeliverSample(pSample, FALSE);

				pSample->Release();
			}

			if (FAILED(hr)) {
				break;
			}
		}
	}

	return hr;
}


HRESULT EVRPresenter::CompleteFrameStep(IMFSample *pSample)
{
	HRESULT hr = S_OK;

	m_FrameStep.State = FrameStepState::Complete;
	m_FrameStep.pSampleNoRef = nullptr;

	NotifyEvent(EC_STEP_COMPLETE, FALSE, 0);

	if (IsScrubbing()) {
		MFTIME hnsSampleTime = 0;
		MFTIME hnsSystemTime = 0;

		hr = pSample->GetSampleTime(&hnsSampleTime);
		if (FAILED(hr)) {
			if (m_Clock) {
				m_Clock->GetCorrelatedTime(0, &hnsSampleTime, &hnsSystemTime);
			}
			hr = S_OK;
		}

		NotifyEvent(EC_SCRUB_TIME, LODWORD(hnsSampleTime), HIDWORD(hnsSampleTime));
	}

	return hr;
}


HRESULT EVRPresenter::CancelFrameStep()
{
	FrameStepState OldState = m_FrameStep.State;

	m_FrameStep.State = FrameStepState::None;
	m_FrameStep.Steps = 0;
	m_FrameStep.pSampleNoRef = nullptr;

	if ((OldState > FrameStepState::None) && (OldState < FrameStepState::Complete)) {
		NotifyEvent(EC_STEP_COMPLETE, TRUE, 0);
	}

	return S_OK;
}


HRESULT EVRPresenter::CreateOptimalVideoType(IMFMediaType *pProposedType, IMFMediaType **ppOptimalType)
{
	if (ppOptimalType == nullptr) {
		return E_POINTER;
	}

	*ppOptimalType = nullptr;

	HRESULT hr;

	VideoType mtOptimal;

	hr = mtOptimal.CopyFrom(pProposedType);
	if (FAILED(hr)) {
		return hr;
	}

	RECT rcOutput = m_PresentEngine->GetDestinationRect();
	if (::IsRectEmpty(&rcOutput)) {
		hr = CalculateOutputRectangle(pProposedType, &rcOutput);
		if (FAILED(hr)) {
			return hr;
		}
	}

	if (m_AspectRatioMode & MFVideoARMode_PreservePicture) {
		hr = mtOptimal.SetPixelAspectRatio(1, 1);
		if (FAILED(hr)) {
			return hr;
		}
	} else {
		UINT32 SrcWidth = 0, SrcHeight = 0;
		hr = mtOptimal.GetFrameDimensions(&SrcWidth, &SrcHeight);
		if (FAILED(hr)) {
			return hr;
		}
		MFRatio AspectRatio = mtOptimal.GetPixelAspectRatio();
		SrcWidth = ::MulDiv(SrcWidth, AspectRatio.Numerator, AspectRatio.Denominator);

		hr = mtOptimal.SetPixelAspectRatio(
			static_cast<UINT32>(
				static_cast<float>(rcOutput.bottom - rcOutput.top) *
				static_cast<float>(SrcWidth) * (m_nrcSource.right - m_nrcSource.left) + 0.5f),
			static_cast<UINT32>(
				static_cast<float>(rcOutput.right - rcOutput.left) *
				static_cast<float>(SrcHeight) * (m_nrcSource.bottom - m_nrcSource.top) + 0.5f));
		if (FAILED(hr)) {
			return hr;
		}
	}

	hr = mtOptimal.SetFrameDimensions(rcOutput.right, rcOutput.bottom);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoArea DisplayArea = MakeArea(0, 0, rcOutput.right, rcOutput.bottom);

	hr = mtOptimal.SetPanScanEnabled(FALSE);
	if (FAILED(hr)) {
		return hr;
	}

	hr = mtOptimal.SetGeometricAperture(DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	hr = mtOptimal.SetPanScanAperture(DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	hr = mtOptimal.SetMinDisplayAperture(DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	mtOptimal.SetYUVMatrix(MFVideoTransferMatrix_BT709);
	mtOptimal.SetTransferFunction(MFVideoTransFunc_709);
	mtOptimal.SetVideoPrimaries(MFVideoPrimaries_BT709);
	//mtOptimal.SetVideoNominalRange(MFNominalRange_16_235);
	mtOptimal.SetVideoNominalRange(MFNominalRange_0_255);
	mtOptimal.SetVideoLighting(MFVideoLighting_dim);

	*ppOptimalType = mtOptimal.Detach();

	return S_OK;
}


HRESULT EVRPresenter::CalculateOutputRectangle(IMFMediaType *pProposedType, RECT *prcOutput)
{
	HRESULT hr;

	VideoType mtProposed(pProposedType);

	UINT32 SrcWidth = 0, SrcHeight = 0;

	hr = mtProposed.GetFrameDimensions(&SrcWidth, &SrcHeight);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoArea DisplayArea = {};

	hr = mtProposed.GetVideoDisplayArea(&DisplayArea);
	if (FAILED(hr)) {
		return hr;
	}

	LONG OffsetX = static_cast<LONG>(MFOffsetToFloat(DisplayArea.OffsetX));
	LONG OffsetY = static_cast<LONG>(MFOffsetToFloat(DisplayArea.OffsetY));
	RECT rcOutput;

	if ((DisplayArea.Area.cx != 0) &&
		(DisplayArea.Area.cy != 0) &&
		(OffsetX + DisplayArea.Area.cx <= static_cast<LONG>(SrcWidth)) &&
		(OffsetY + DisplayArea.Area.cy <= static_cast<LONG>(SrcHeight))) {
		rcOutput.left   = OffsetX;
		rcOutput.right  = OffsetX + DisplayArea.Area.cx;
		rcOutput.top    = OffsetY;
		rcOutput.bottom = OffsetY + DisplayArea.Area.cy;
	} else {
		rcOutput.left   = 0;
		rcOutput.top    = 0;
		rcOutput.right  = SrcWidth;
		rcOutput.bottom = SrcHeight;
	}

	MFRatio InputPAR = mtProposed.GetPixelAspectRatio();
	MFRatio OutputPAR = {1, 1};

	*prcOutput = CorrectAspectRatio(rcOutput, InputPAR, OutputPAR);

	return S_OK;
}


HRESULT EVRPresenter::SetMediaType(IMFMediaType *pMediaType)
{
	if (pMediaType == nullptr) {
		m_MediaType.Release();
		ReleaseResources();
		return S_OK;
	}

	HRESULT hr;

	hr = CheckShutdown();
	if (FAILED(hr)) {
		return hr;
	}

	if (AreMediaTypesEqual(m_MediaType.Get(), pMediaType)) {
		return S_OK;
	}

	m_MediaType.Release();
	ReleaseResources();

	VideoSampleList SampleQueue;
	MFRatio fps = {0, 0};

	hr = m_PresentEngine->CreateVideoSamples(pMediaType, SampleQueue);
	if (FAILED(hr)) {
		goto Done;
	}

	for (VideoSampleList::Position Pos = SampleQueue.FrontPosition();
			Pos != SampleQueue.EndPosition();
			Pos = SampleQueue.Next(Pos)) {
		IMFSample *pSample = nullptr;

		hr = SampleQueue.GetItemPos(Pos, &pSample);
		if (FAILED(hr)) {
			goto Done;
		}

		hr = pSample->SetUINT32(SampleAttribute_Counter, m_TokenCounter);

		pSample->Release();

		if (FAILED(hr)) {
			goto Done;
		}
	}

	hr = m_SamplePool.Initialize(SampleQueue);
	if (FAILED(hr)) {
		goto Done;
	}

	if (SUCCEEDED(GetFrameRate(pMediaType, &fps)) &&
		(fps.Numerator != 0) && (fps.Denominator != 0)) {
		m_Scheduler.SetFrameRate(fps);
	} else {
		static const MFRatio DefaultFrameRate = {30, 1};
		m_Scheduler.SetFrameRate(DefaultFrameRate);
	}

	m_MediaType = pMediaType;

Done:
	if (FAILED(hr)) {
		ReleaseResources();
	}

	return hr;
}


HRESULT EVRPresenter::IsMediaTypeSupported(IMFMediaType *pMediaType)
{
	HRESULT hr;

	VideoType mtProposed(pMediaType);

	BOOL bCompressed = FALSE;

	hr = mtProposed.IsCompressedFormat(&bCompressed);
	if (FAILED(hr)) {
		return hr;
	}

	if (bCompressed) {
		return MF_E_INVALIDMEDIATYPE;
	}

	D3DFORMAT Format = D3DFMT_UNKNOWN;

	hr = mtProposed.GetFourCC((DWORD*)&Format);
	if (FAILED(hr)) {
		return hr;
	}

	hr = m_PresentEngine->CheckFormat(Format);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoInterlaceMode InterlaceMode = MFVideoInterlace_Unknown;

	hr = mtProposed.GetInterlaceMode(&InterlaceMode);
	if (InterlaceMode != MFVideoInterlace_Progressive) {
		return MF_E_INVALIDMEDIATYPE;
	}

	UINT32 Width = 0, Height = 0;

	hr = mtProposed.GetFrameDimensions(&Width, &Height);
	if (FAILED(hr)) {
		return hr;
	}

	MFVideoArea VideoCropArea;

	if (SUCCEEDED(mtProposed.GetPanScanAperture(&VideoCropArea))) {
		hr = ValidateVideoArea(VideoCropArea, Width, Height);
		if (FAILED(hr)) {
			return hr;
		}
	}
	if (SUCCEEDED(mtProposed.GetGeometricAperture(&VideoCropArea))) {
		hr = ValidateVideoArea(VideoCropArea, Width, Height);
		if (FAILED(hr)) {
			return hr;
		}
	}
	if (SUCCEEDED(mtProposed.GetMinDisplayAperture(&VideoCropArea))) {
		hr = ValidateVideoArea(VideoCropArea, Width, Height);
		if (FAILED(hr)) {
			return hr;
		}
	}

	return S_OK;
}


void EVRPresenter::ProcessOutputLoop()
{
	HRESULT hr;

	do {
		if (!m_SampleNotify) {
			hr = MF_E_TRANSFORM_NEED_MORE_INPUT;
			break;
		}

		hr = ProcessOutput();

	} while (hr == S_OK);

	if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
		CheckEndOfStream();
	}
}


HRESULT EVRPresenter::ProcessOutput()
{
	LIBISDB_ASSERT(m_SampleNotify || m_Repaint);

	HRESULT hr = S_OK;
	DWORD Status = 0;
	LONGLONG MixerStartTime = 0, MixerEndTime = 0;
	MFTIME SystemTime = 0;
	bool bRepaint = m_Repaint;

	MFT_OUTPUT_DATA_BUFFER DataBuffer = {};

	IMFSample *pSample = nullptr;

	if ((m_RenderState != RenderState::Started) && !m_Repaint && m_Prerolled) {
		return S_FALSE;
	}

	if (!m_Mixer) {
		return MF_E_INVALIDREQUEST;
	}

	hr = m_SamplePool.GetSample(&pSample);
	if (hr == MF_E_SAMPLEALLOCATOR_EMPTY) {
		return S_FALSE;
	}
	if (FAILED(hr)) {
		return hr;
	}

	if (m_Repaint) {
		SetDesiredSampleTime(pSample, m_Scheduler.LastSampleTime(), m_Scheduler.FrameDuration());
		m_Repaint = false;
	} else {
		ClearDesiredSampleTime(pSample);

		if (m_Clock) {
			m_Clock->GetCorrelatedTime(0, &MixerStartTime, &SystemTime);
		}
	}

	DataBuffer.dwStreamID = 0;
	DataBuffer.pSample = pSample;
	DataBuffer.dwStatus = 0;

	hr = m_Mixer->ProcessOutput(0, 1, &DataBuffer, &Status);

	if (FAILED(hr)) {
		HRESULT hr2 = m_SamplePool.ReturnSample(pSample);
		if (FAILED(hr2)) {
			hr = hr2;
		} else {
			if (hr == MF_E_TRANSFORM_TYPE_NOT_SET) {
				hr = RenegotiateMediaType();
			} else if (hr == MF_E_TRANSFORM_STREAM_CHANGE) {
				SetMediaType(nullptr);
			} else if (hr == MF_E_TRANSFORM_NEED_MORE_INPUT) {
				m_SampleNotify = false;
			}
		}
	} else {
		if (m_Clock && !bRepaint) {
			m_Clock->GetCorrelatedTime(0, &MixerEndTime, &SystemTime);

			LONGLONG LatencyTime = MixerEndTime - MixerStartTime;
			NotifyEvent(EC_PROCESSING_LATENCY, (LONG_PTR)&LatencyTime, 0);
		}

		hr = TrackSample(pSample);
		if (SUCCEEDED(hr)) {
			if ((m_FrameStep.State == FrameStepState::None) || bRepaint) {
				hr = DeliverSample(pSample, bRepaint);
			} else {
				hr = DeliverFrameStepSample(pSample);
			}
			if (SUCCEEDED(hr)) {
				m_Prerolled = true;
			}
		}
	}

	SafeRelease(DataBuffer.pEvents);
	SafeRelease(pSample);

	return hr;
}


HRESULT EVRPresenter::DeliverSample(IMFSample *pSample, BOOL bRepaint)
{
	LIBISDB_ASSERT(pSample != nullptr);

	HRESULT hr;
	EVRPresentEngine::DeviceState State = EVRPresentEngine::DeviceState::OK;

	bool bPresentNow = ((m_RenderState != RenderState::Started) || IsScrubbing() || bRepaint);

	hr = m_PresentEngine->CheckDeviceState(&State);

	if (SUCCEEDED(hr)) {
		hr = m_Scheduler.ScheduleSample(pSample, bPresentNow);
	}

	if (FAILED(hr)) {
		NotifyEvent(EC_ERRORABORT, hr, 0);
	} else if (State == EVRPresentEngine::DeviceState::Reset) {
		NotifyEvent(EC_DISPLAY_CHANGED, S_OK, 0);
	}

	return hr;
}


HRESULT EVRPresenter::DeliverFrameStepSample(IMFSample *pSample)
{
	HRESULT hr = S_OK;

	if (IsScrubbing() && m_Clock && IsSampleTimePassed(m_Clock.Get(), pSample)) {
		// サンプル破棄
	} else if (m_FrameStep.State >= FrameStepState::Scheduled) {
		hr = m_FrameStep.Samples.InsertBack(pSample);
	} else {
		if (m_FrameStep.Steps > 0) {
			m_FrameStep.Steps--;
		}

		if (m_FrameStep.Steps > 0) {
			// サンプル破棄
		} else if (m_FrameStep.State == FrameStepState::WaitingStart) {
			hr = m_FrameStep.Samples.InsertBack(pSample);
		} else {
			hr = DeliverSample(pSample, FALSE);
			if (SUCCEEDED(hr)) {
				IUnknown *pUnk = nullptr;

				hr = pSample->QueryInterface(IDD_PPV_ARGS_IUNKNOWN(&pUnk));
				if (SUCCEEDED(hr)) {
					m_FrameStep.pSampleNoRef = pUnk;
					m_FrameStep.State = FrameStepState::Scheduled;
					pUnk->Release();
				}
			}
		}
	}

	return hr;
}


HRESULT EVRPresenter::TrackSample(IMFSample *pSample)
{
	HRESULT hr;
	IMFTrackedSample *pTracked = nullptr;

	hr = pSample->QueryInterface(IID_PPV_ARGS(&pTracked));
	if (SUCCEEDED(hr)) {
		hr = pTracked->SetAllocator(&m_SampleFreeCB, nullptr);
		pTracked->Release();
	}

	return hr;
}


void EVRPresenter::ReleaseResources()
{
	m_TokenCounter++;

	Flush();

	m_SamplePool.Clear();

	m_PresentEngine->ReleaseResources();
}


HRESULT EVRPresenter::OnSampleFree(IMFAsyncResult *pResult)
{
	HRESULT hr;
	IUnknown *pObject = nullptr;
	IMFSample *pSample = nullptr;

	hr = pResult->GetObject(&pObject);

	if (SUCCEEDED(hr)) {
		hr = pObject->QueryInterface(IID_PPV_ARGS(&pSample));

		if (SUCCEEDED(hr)) {
			if (m_FrameStep.State == FrameStepState::Scheduled)  {
				IUnknown *pUnk = nullptr;

				hr = pSample->QueryInterface(IDD_PPV_ARGS_IUNKNOWN(&pUnk));

				if (SUCCEEDED(hr)) {
					if (m_FrameStep.pSampleNoRef == pUnk) {
						hr = CompleteFrameStep(pSample);
					}

					pUnk->Release();
				}
			}
		}
	}

	if (SUCCEEDED(hr)) {
		BlockLock Lock(m_ObjectLock);

		if (::MFGetAttributeUINT32(pSample, SampleAttribute_Counter, (UINT32)-1) == m_TokenCounter) {
			hr = m_SamplePool.ReturnSample(pSample);

			if (SUCCEEDED(hr)) {
				ProcessOutputLoop();
			}
		}
	}

	if (FAILED(hr)) {
		NotifyEvent(EC_ERRORABORT, hr, 0);
	}

	SafeRelease(pObject);
	SafeRelease(pSample);

	return hr;
}


float EVRPresenter::GetMaxRate(BOOL bThin)
{
	float fMaxRate = std::numeric_limits<float>::max();

	if (!bThin && m_MediaType) {
		MFRatio fps = {0, 0};

		GetFrameRate(m_MediaType.Get(), &fps);

		UINT MonitorRateHz = m_PresentEngine->RefreshRate();

		if (fps.Denominator && fps.Numerator && MonitorRateHz) {
			fMaxRate = static_cast<float>(::MulDiv(MonitorRateHz, fps.Denominator, fps.Numerator));
		}
	}

	return fMaxRate;
}


}	// namespace LibISDB::DirectShow
