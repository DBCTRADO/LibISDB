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
 @file   EVRMediaType.hpp
 @brief  EVR メディアタイプ
 @author DBCTRADO
*/


#ifndef LIBISDB_EVR_MEDIA_TYPE_H
#define LIBISDB_EVR_MEDIA_TYPE_H


#include <mferror.h>


namespace LibISDB::DirectShow
{

	inline MFOffset MakeOffset(float v)
	{
		MFOffset offset;
		offset.value = static_cast<short>(v);
		offset.fract = static_cast<WORD>(65536 * (v - offset.value));
		return offset;
	}

	inline MFVideoArea MakeArea(float x, float y, DWORD width, DWORD height)
	{
		MFVideoArea area;
		area.OffsetX = MakeOffset(x);
		area.OffsetY = MakeOffset(y);
		area.Area.cx = width;
		area.Area.cy = height;
		return area;
	}

	HRESULT GetFrameRate(IMFMediaType *pType, MFRatio *pRatio);
	HRESULT GetVideoDisplayArea(IMFMediaType *pType, MFVideoArea *pArea);
	HRESULT GetDefaultStride(IMFMediaType *pType, LONG *pStride);


	class MediaType
	{
	protected:
		COMPointer<IMFMediaType> m_Type;

	protected:
		bool IsValid() const noexcept
		{
			return static_cast<bool>(m_Type);
		}

		IMFMediaType * GetMediaType()
		{
			LIBISDB_ASSERT(IsValid());
			return m_Type.Get();
		}

		template<typename T> HRESULT GetUINT32(const GUID &guidKey, T *pValue)
		{
			static_assert(sizeof(T) == sizeof(UINT32));
			if (pValue == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->GetUINT32(guidKey, reinterpret_cast<UINT32*>(pValue));
		}

		template<typename T> HRESULT SetUINT32(const GUID &guidKey, T Value)
		{
			static_assert(sizeof(T) <= sizeof(UINT32));
			return GetMediaType()->SetUINT32(guidKey, static_cast<UINT32>(Value));
		}

		template<typename T> HRESULT GetBlob(const GUID &guidKey, T *pValue)
		{
			if (pValue == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->GetBlob(guidKey, reinterpret_cast<UINT8*>(pValue), sizeof(T), nullptr);
		}

		template<typename T> HRESULT SetBlob(const GUID &guidKey, const T &Value)
		{
			return GetMediaType()->SetBlob(guidKey, reinterpret_cast<const UINT8*>(&Value), sizeof(T));
		}

	public:
		MediaType() = default;

		MediaType(IMFMediaType *pType)
			: m_Type(pType)
		{
		}

		MediaType(const MediaType &mt)
			: m_Type(mt.m_Type)
		{
		}

		virtual ~MediaType() = default;

		IMFMediaType ** operator & ()
		{
			return m_Type.GetPP();
		}

		operator IMFMediaType * ()
		{
			return m_Type.Get();
		}

		virtual HRESULT CreateEmptyType()
		{
			return ::MFCreateMediaType(m_Type.GetPP());
		}

		IMFMediaType * Detach()
		{
			return m_Type.Detach();
		}

		HRESULT GetMajorType(GUID *pGuid)
		{
			return GetMediaType()->GetMajorType(pGuid);
		}

		HRESULT IsCompressedFormat(BOOL *pbCompressed)
		{
			return GetMediaType()->IsCompressedFormat(pbCompressed);
		}

		HRESULT IsEqual(IMFMediaType *pType, DWORD *pdwFlags)
		{
			return GetMediaType()->IsEqual(pType, pdwFlags);
		}

		HRESULT GetRepresentation(GUID guidRepresentation, LPVOID *ppvRepresentation)
		{
			return GetMediaType()->GetRepresentation(guidRepresentation, ppvRepresentation);
		}

		HRESULT FreeRepresentation(GUID guidRepresentation, LPVOID pvRepresentation) 
		{
			return GetMediaType()->FreeRepresentation(guidRepresentation, pvRepresentation);
		}

		HRESULT CopyFrom(MediaType *pType)
		{
			if (pType == nullptr) {
				return E_POINTER;
			}
			if (!pType->IsValid()) {
				return E_UNEXPECTED;
			}
			return CopyFrom(pType->m_Type.Get());
		}

		HRESULT CopyFrom(IMFMediaType *pType)
		{
			if (pType == nullptr) {
				return E_POINTER;
			}

			if (!m_Type) {
				const HRESULT hr = CreateEmptyType();
				if (FAILED(hr)) {
					return hr;
				}
			}

			return pType->CopyAllItems(m_Type.Get());
		}

		HRESULT GetMediaType(IMFMediaType **ppType)
		{
			if (ppType == nullptr) {
				return E_POINTER;
			}
			LIBISDB_ASSERT(IsValid());
			*ppType = m_Type.Get();
			(*ppType)->AddRef();
			return S_OK;
		}

		HRESULT SetMajorType(GUID guid)
		{
			return GetMediaType()->SetGUID(MF_MT_MAJOR_TYPE, guid);
		}

		HRESULT GetSubType(GUID* pGuid)
		{
			if (pGuid == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->GetGUID(MF_MT_SUBTYPE, pGuid);
		}

		HRESULT SetSubType(GUID guid)
		{
			return GetMediaType()->SetGUID(MF_MT_SUBTYPE, guid);
		}

		HRESULT GetFourCC(DWORD *pFourCC)
		{
			if (pFourCC == nullptr) {
				return E_POINTER;
			}

			LIBISDB_ASSERT(IsValid());

			GUID guidSubType = GUID_NULL;

			const HRESULT hr = GetSubType(&guidSubType);
			if (SUCCEEDED(hr)) {
				*pFourCC = guidSubType.Data1;
			}
			return hr;
		}

		HRESULT GetAllSamplesIndependent(BOOL *pbIndependent)
		{
			return GetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, pbIndependent);
		}

		HRESULT SetAllSamplesIndependent(BOOL bIndependent)
		{
			return SetUINT32(MF_MT_ALL_SAMPLES_INDEPENDENT, bIndependent);
		}

		HRESULT GetFixedSizeSamples(BOOL *pbFixed)
		{
			return GetUINT32(MF_MT_FIXED_SIZE_SAMPLES, pbFixed);
		}

		HRESULT SetFixedSizeSamples(BOOL bFixed)
		{
			return SetUINT32(MF_MT_FIXED_SIZE_SAMPLES, bFixed);
		}

		HRESULT GetSampleSize(UINT32 *pSize)
		{
			return GetUINT32(MF_MT_SAMPLE_SIZE, pSize);
		}

		HRESULT SetSampleSize(UINT32 Size)
		{
			return SetUINT32(MF_MT_SAMPLE_SIZE, Size);
		}

		HRESULT Unwrap(IMFMediaType **ppOriginal)
		{
			if (ppOriginal == nullptr) {
				return E_POINTER;
			}
			return ::MFUnwrapMediaType(GetMediaType(), ppOriginal);
		}

		BOOL AllSamplesIndependent()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_ALL_SAMPLES_INDEPENDENT, FALSE) != FALSE;
		}

		BOOL FixedSizeSamples()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_FIXED_SIZE_SAMPLES, FALSE) != FALSE;
		}

		UINT32 SampleSize()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_SAMPLE_SIZE, 0);
		}
	};


	class VideoType
		: public MediaType
	{
		friend class MediaType;

	public:
		VideoType(IMFMediaType *pType = nullptr)
			: MediaType(pType)
		{
		}

		HRESULT CreateEmptyType() override
		{
			HRESULT hr = MediaType::CreateEmptyType();
			if (SUCCEEDED(hr)) {
				hr = SetMajorType(MFMediaType_Video);
			}
			return hr;
		}

		HRESULT GetInterlaceMode(MFVideoInterlaceMode *pmode)
		{
			return GetUINT32(MF_MT_INTERLACE_MODE, pmode);
		}

		HRESULT SetInterlaceMode(MFVideoInterlaceMode mode)
		{
			return SetUINT32(MF_MT_INTERLACE_MODE, mode);
		}

		HRESULT GetDefaultStride(LONG *pStride)
		{
			return DirectShow::GetDefaultStride(GetMediaType(), pStride);
		}

		HRESULT SetDefaultStride(LONG Stride)
		{
			return SetUINT32(MF_MT_DEFAULT_STRIDE, Stride);
		}

		HRESULT GetFrameDimensions(UINT32 *pWidthInPixels, UINT32 *pHeightInPixels)
		{
			return ::MFGetAttributeSize(GetMediaType(), MF_MT_FRAME_SIZE, pWidthInPixels, pHeightInPixels);
		}

		HRESULT SetFrameDimensions(UINT32 WidthInPixels, UINT32 HeightInPixels)
		{
			return ::MFSetAttributeSize(GetMediaType(), MF_MT_FRAME_SIZE, WidthInPixels, HeightInPixels);
		}

		HRESULT GetDataBitErrorRate(UINT32 *pRate)
		{
			return GetUINT32(MF_MT_AVG_BIT_ERROR_RATE, pRate);
		}

		HRESULT SetDataBitErrorRate(UINT32 rate)
		{
			return SetUINT32(MF_MT_AVG_BIT_ERROR_RATE, rate);
		}

		HRESULT GetAverageBitRate(UINT32 *pRate)
		{
			return GetUINT32(MF_MT_AVG_BITRATE, pRate);
		}

		HRESULT SetAvgerageBitRate(UINT32 rate)
		{
			return SetUINT32(MF_MT_AVG_BITRATE, rate);
		}

		HRESULT GetCustomVideoPrimaries(MT_CUSTOM_VIDEO_PRIMARIES *pPrimaries)
		{
			return GetBlob(MF_MT_CUSTOM_VIDEO_PRIMARIES, pPrimaries);
		}

		HRESULT SetCustomVideoPrimaries(const MT_CUSTOM_VIDEO_PRIMARIES &primary)
		{
			return SetBlob(MF_MT_CUSTOM_VIDEO_PRIMARIES, primary);
		}

		HRESULT GetFrameRate(UINT32 *pNumerator, UINT32 *pDenominator)
		{
			if ((pNumerator == nullptr) || (pDenominator == nullptr)) {
				return E_POINTER;
			}
			return ::MFGetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, pNumerator, pDenominator);
		}

		HRESULT GetFrameRate(MFRatio *pRatio)
		{
			if (pRatio == nullptr) {
				return E_POINTER;
			}
			return GetFrameRate(reinterpret_cast<UINT32*>(&pRatio->Numerator), reinterpret_cast<UINT32*>(&pRatio->Denominator));
		}

		HRESULT SetFrameRate(UINT32 Numerator, UINT32 Denominator)
		{
			return ::MFSetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, Numerator, Denominator);
		}

		HRESULT SetFrameRate(const MFRatio &ratio)
		{
			return ::MFSetAttributeRatio(GetMediaType(), MF_MT_FRAME_RATE, ratio.Numerator, ratio.Denominator);
		}

		HRESULT GetGeometricAperture(MFVideoArea *pArea)
		{
			return GetBlob(MF_MT_GEOMETRIC_APERTURE, pArea);
		}

		HRESULT SetGeometricAperture(const MFVideoArea& area)
		{
			return SetBlob(MF_MT_GEOMETRIC_APERTURE, area);
		}

		HRESULT GetMaxKeyframeSpacing(UINT32 *pSpacing)
		{
			return GetUINT32(MF_MT_MAX_KEYFRAME_SPACING, pSpacing);
		}

		HRESULT SetMaxKeyframeSpacing(UINT32 Spacing)
		{
			return SetUINT32(MF_MT_MAX_KEYFRAME_SPACING, Spacing);
		}

		HRESULT GetMinDisplayAperture(MFVideoArea *pArea)
		{
			return GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, pArea);
		}

		HRESULT SetMinDisplayAperture(const MFVideoArea &area)
		{
			return SetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, area);
		}

		HRESULT GetPadControlFlags(MFVideoPadFlags *pFlags)
		{
			return GetUINT32(MF_MT_PAD_CONTROL_FLAGS, pFlags);
		}

		HRESULT SetPadControlFlags(MFVideoPadFlags flags)
		{
			return SetUINT32(MF_MT_PAD_CONTROL_FLAGS, flags);
		}

		HRESULT GetPaletteEntries(MFPaletteEntry *paEntries, UINT32 nEntries)
		{
			if (paEntries == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->GetBlob(MF_MT_PALETTE, reinterpret_cast<UINT8*>(paEntries), sizeof(MFPaletteEntry) * nEntries, nullptr);
		}

		HRESULT SetPaletteEntries(const MFPaletteEntry *paEntries, UINT32 nEntries)
		{
			if (paEntries == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->SetBlob(MF_MT_PALETTE, reinterpret_cast<const UINT8*>(paEntries), sizeof(MFPaletteEntry) * nEntries);
		}

		HRESULT GetNumPaletteEntries(UINT32 *pEntries)
		{
			if (pEntries == nullptr) {
				return E_POINTER;
			}

			UINT32 Bytes = 0;
			HRESULT hr = GetMediaType()->GetBlobSize(MF_MT_PALETTE, &Bytes);
			if (SUCCEEDED(hr)) {
				if (Bytes % sizeof(MFPaletteEntry) != 0) {
					hr = E_UNEXPECTED;
				} else {
					*pEntries = Bytes / sizeof(MFPaletteEntry);
				}
			}
			return hr;
		}

		HRESULT GetPanScanAperture(MFVideoArea *pArea)
		{
			return GetBlob(MF_MT_PAN_SCAN_APERTURE, pArea);
		}

		HRESULT SetPanScanAperture(const MFVideoArea &area)
		{
			return SetBlob(MF_MT_PAN_SCAN_APERTURE, area);
		}

		HRESULT IsPanScanEnabled(BOOL *pBool)
		{
			return GetUINT32(MF_MT_PAN_SCAN_ENABLED, pBool);
		}

		HRESULT SetPanScanEnabled(BOOL bEnabled)
		{
			return SetUINT32(MF_MT_PAN_SCAN_ENABLED, bEnabled);
		}

		HRESULT GetPixelAspectRatio(UINT32 *pNumerator, UINT32 *pDenominator)
		{
			if ((pNumerator == nullptr) || (pDenominator == nullptr)) {
				return E_POINTER;
			}
			return ::MFGetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, pNumerator, pDenominator);
		}

		HRESULT SetPixelAspectRatio(UINT32 Numerator, UINT32 Denominator)
		{
			return ::MFSetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, Numerator, Denominator);
		}

		HRESULT SetPixelAspectRatio(const MFRatio &ratio)
		{
			return ::MFSetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, ratio.Numerator, ratio.Denominator);
		}

		HRESULT GetSourceContentHint(MFVideoSrcContentHintFlags *pFlags)
		{
			return GetUINT32(MF_MT_SOURCE_CONTENT_HINT, pFlags);
		}

		HRESULT SetSourceContentHint(MFVideoSrcContentHintFlags Flags)
		{
			return SetUINT32(MF_MT_SOURCE_CONTENT_HINT, Flags);
		}

		HRESULT GetTransferFunction(MFVideoTransferFunction *pFxn)
		{
			return GetUINT32(MF_MT_TRANSFER_FUNCTION, pFxn);
		}

		HRESULT SetTransferFunction(MFVideoTransferFunction Fxn)
		{
			return SetUINT32(MF_MT_TRANSFER_FUNCTION, Fxn);
		}

		HRESULT GetChromaSiting(MFVideoChromaSubsampling *pSampling)
		{
			return GetUINT32(MF_MT_VIDEO_CHROMA_SITING, pSampling);
		}

		HRESULT SetChromaSiting(MFVideoChromaSubsampling Sampling)
		{
			return SetUINT32(MF_MT_VIDEO_CHROMA_SITING, Sampling);
		}

		HRESULT GetVideoLighting(MFVideoLighting *pLighting)
		{
			return GetUINT32(MF_MT_VIDEO_LIGHTING, pLighting);
		}

		HRESULT SetVideoLighting(MFVideoLighting Lighting)
		{
			return SetUINT32(MF_MT_VIDEO_LIGHTING, Lighting);
		}

		HRESULT GetVideoNominalRange(MFNominalRange *pRange)
		{
			return GetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, pRange);
		}

		HRESULT SetVideoNominalRange(MFNominalRange Range)
		{
			return SetUINT32(MF_MT_VIDEO_NOMINAL_RANGE, Range);
		}

		HRESULT GetVideoPrimaries(MFVideoPrimaries *pPrimaries)
		{
			return GetUINT32(MF_MT_VIDEO_PRIMARIES, pPrimaries);
		}

		HRESULT SetVideoPrimaries(MFVideoPrimaries Primaries)
		{
			return SetUINT32(MF_MT_VIDEO_PRIMARIES, Primaries);
		}

		HRESULT GetYUVMatrix(MFVideoTransferMatrix *pMatrix)
		{
			return GetUINT32(MF_MT_YUV_MATRIX, pMatrix);
		}

		HRESULT SetYUVMatrix(MFVideoTransferMatrix Matrix)
		{
			return SetUINT32(MF_MT_YUV_MATRIX, Matrix);
		}

		MFRatio GetPixelAspectRatio()
		{
			MFRatio PAR = {0, 0};

			const HRESULT hr = ::MFGetAttributeRatio(GetMediaType(), MF_MT_PIXEL_ASPECT_RATIO, (UINT32*)&PAR.Numerator, (UINT32*)&PAR.Denominator);
			if (FAILED(hr)) {
				PAR.Numerator = 1;
				PAR.Denominator = 1;
			}
			return PAR;
		}

		BOOL IsPanScanEnabled()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_PAN_SCAN_ENABLED, FALSE) != 0;
		}

		HRESULT GetVideoDisplayArea(MFVideoArea *pArea)
		{
			if (pArea == nullptr) {
				return E_POINTER;
			}
			return DirectShow::GetVideoDisplayArea(GetMediaType(), pArea);
		}
	};


	class AudioType
		: public MediaType
	{
		friend class MediaType;

	public:
		AudioType(IMFMediaType *pType = nullptr)
			: MediaType(pType)
		{
		}

		HRESULT CreateEmptyType() override
		{
			HRESULT hr = MediaType::CreateEmptyType();
			if (SUCCEEDED(hr)) {
				hr = SetMajorType(MFMediaType_Audio);
			}
			return hr;
		}

		HRESULT GetAvgerageBytesPerSecond(UINT32 *pBytes)
		{
			return GetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, pBytes);
		}

		HRESULT SetAvgerageBytesPerSecond(UINT32 Bytes)
		{
			return SetUINT32(MF_MT_AUDIO_AVG_BYTES_PER_SECOND, Bytes);
		}

		HRESULT GetBitsPerSample(UINT32 *pBits)
		{
			return GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, pBits);
		}

		HRESULT SetBitsPerSample(UINT32 Bits)
		{
			return SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, Bits);
		}

		HRESULT GetBlockAlignment(UINT32 *pBytes)
		{
			return GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, pBytes);
		}

		HRESULT SetBlockAlignment(UINT32 Bytes)
		{
			return SetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, Bytes);
		}

		HRESULT GetChannelMask(UINT32 *pMask)
		{
			return GetUINT32(MF_MT_AUDIO_CHANNEL_MASK, pMask);
		}

		HRESULT SetChannelMask(UINT32 Mask)
		{
			return SetUINT32(MF_MT_AUDIO_CHANNEL_MASK, Mask);
		}

		HRESULT GetFloatSamplesPerSecond(double *pfSampleRate)
		{
			if (pfSampleRate == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->GetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, pfSampleRate);
		}

		HRESULT SetFloatSamplesPerSecond(double fSampleRate)
		{
			return GetMediaType()->SetDouble(MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, fSampleRate);
		}

		HRESULT GetNumChannels(UINT32 *pChannels)
		{
			return GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, pChannels);
		}

		HRESULT SetNumChannels(UINT32 Channels)
		{
			return SetUINT32(MF_MT_AUDIO_NUM_CHANNELS, Channels);
		}

		HRESULT GetSamplesPerBlock(UINT32 *pSamples)
		{
			return GetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, pSamples);
		}

		HRESULT SetSamplesPerBlock(UINT32 Samples)
		{
			return SetUINT32(MF_MT_AUDIO_SAMPLES_PER_BLOCK, Samples);
		}

		HRESULT GetSamplesPerSecond(UINT32 *pSampleRate)
		{
			return GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, pSampleRate);
		}

		HRESULT SetSamplesPerSecond(UINT32 SampleRate)
		{
			return SetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, SampleRate);
		}

		HRESULT GetValidBitsPerSample(UINT32 *pBits)
		{
			HRESULT hr = GetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, pBits);
			if (FAILED(hr)) {
				hr = GetBitsPerSample(pBits);
			}
			return hr;
		}

		HRESULT SetValidBitsPerSample(UINT32 Bits)
		{
			return SetUINT32(MF_MT_AUDIO_VALID_BITS_PER_SAMPLE, Bits);
		}

		UINT32 AvgerageBytesPerSecond()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_AVG_BYTES_PER_SECOND, 0);
		}

		UINT32 BitsPerSample()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_BITS_PER_SAMPLE, 0);
		}

		UINT32 GetBlockAlignment()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_BLOCK_ALIGNMENT, 0);
		}

		double FloatSamplesPerSecond()
		{
			return ::MFGetAttributeDouble(GetMediaType(), MF_MT_AUDIO_FLOAT_SAMPLES_PER_SECOND, 0.0);
		}

		UINT32 NumChannels()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_NUM_CHANNELS, 0);
		}

		UINT32 SamplesPerSecond()
		{
			return ::MFGetAttributeUINT32(GetMediaType(), MF_MT_AUDIO_SAMPLES_PER_SECOND, 0);
		}
	};


	class MPEGVideoType
		: public VideoType
	{
		friend class MediaType;

	public:
		MPEGVideoType(IMFMediaType *pType = nullptr)
			: VideoType(pType)
		{
		}

		HRESULT GetMpegSeqHeader(BYTE *pData, UINT32 cbSize)
		{
			if (pData == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->GetBlob(MF_MT_MPEG_SEQUENCE_HEADER, pData, cbSize, nullptr);
		}

		HRESULT SetMpegSeqHeader(const BYTE *pData, UINT32 cbSize)
		{
			if (pData == nullptr) {
				return E_POINTER;
			}
			return GetMediaType()->SetBlob(MF_MT_MPEG_SEQUENCE_HEADER, pData, cbSize);
		}

		HRESULT GetMpegSeqHeaderSize(UINT32 *pcbSize)
		{
			if (pcbSize == nullptr) {
				return E_POINTER;
			}

			*pcbSize = 0;

			HRESULT hr = GetMediaType()->GetBlobSize(MF_MT_MPEG_SEQUENCE_HEADER, pcbSize);
			if (hr == MF_E_ATTRIBUTENOTFOUND) {
				hr = S_OK;
			}
			return hr;
		}

		HRESULT GetStartTimeCode(UINT32 *pTime)
		{
			return GetUINT32(MF_MT_MPEG_START_TIME_CODE, pTime);
		}

		HRESULT SetStartTimeCode(UINT32 Time)
		{
			return SetUINT32(MF_MT_MPEG_START_TIME_CODE, Time);
		}

		HRESULT GetMPEG2Flags(UINT32 *pFlags)
		{
			return GetUINT32(MF_MT_MPEG2_FLAGS, pFlags);
		}

		HRESULT SetMPEG2Flags(UINT32 Flags)
		{
			return SetUINT32(MF_MT_MPEG2_FLAGS, Flags);
		}

		HRESULT GetMPEG2Level(UINT32 *pLevel)
		{
			return GetUINT32(MF_MT_MPEG2_LEVEL, pLevel);
		}

		HRESULT SetMPEG2Level(UINT32 Level)
		{
			return SetUINT32(MF_MT_MPEG2_LEVEL, Level);
		}

		HRESULT GetMPEG2Profile(UINT32 *pProfile)
		{
			return GetUINT32(MF_MT_MPEG2_PROFILE, pProfile);
		}

		HRESULT SetMPEG2Profile(UINT32 Profile)
		{
			return SetUINT32(MF_MT_MPEG2_PROFILE, Profile);
		}
	};

}	// namespace LibISDB::DirectShow


#endif	// ifndef LIBISDB_EVR_MEDIA_TYPE_H
