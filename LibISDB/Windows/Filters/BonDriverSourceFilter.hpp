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
 @file   BonDriverSourceFilter.hpp
 @brief  BonDriver ソースフィルタ
 @author DBCTRADO
*/


#ifndef LIBISDB_BON_DRIVER_SOURCE_FILTER_H
#define LIBISDB_BON_DRIVER_SOURCE_FILTER_H


#include "../../Filters/SourceFilter.hpp"
#include "../../Utilities/Thread.hpp"
#include "../../Utilities/ConditionVariable.hpp"
#include "../Base/BonDriver.hpp"
#include "../../Utilities/BitRateCalculator.hpp"
#include <deque>
#include <atomic>


namespace LibISDB
{

	/** BonDriver ソースフィルタクラス */
	class BonDriverSourceFilter
		: public SourceFilter
		, protected Thread
	{
	public:
		/** エラーコード */
		enum class ErrorCode {
			Success,               /**< 成功 */
			NotLoaded,             /**< ライブラリが読み込まれていない */
			AlreadyLoaded,         /**< ライブラリが既に読み込まれている */
			CreateBonDriverFailed, /**< CreateBonDriver 失敗 */
			TunerOpenFailed,       /**< チューナオープンエラー */
			TunerNotOpened,        /**< チューナが開かれていない */
			TunerAlreadyOpened,    /**< チューナが既に開かれている */
			SetChannelFailed,      /**< チャンネル設定失敗 */
			Pending,               /**< 前の処理が終わっていない */
		};

		struct ErrorCategory : public std::error_category {
			const char * name() const noexcept override { return "BonDriverSourceFilter"; }
			std::string message(int ev) const override;
		};

		static constexpr unsigned long FIRST_CHANNEL_SET_DELAY_MAX = 5000UL;
		static constexpr unsigned long CHANNEL_CHANGE_INTERVAL_MAX = 5000UL;

		static constexpr uint32_t SPACE_INVALID   = BonDriver::SPACE_INVALID;
		static constexpr uint32_t CHANNEL_INVALID = BonDriver::CHANNEL_INVALID;

		BonDriverSourceFilter();
		~BonDriverSourceFilter();

	// ObjectBase
		const CharType * GetObjectName() const noexcept override { return LIBISDB_STR("BonDriverSourceFilter"); }

	// FilterBase
		void Reset() override;
		void ResetGraph() override;
		bool StartStreaming() override;
		bool StopStreaming() override;

	// SourceFilter
		bool OpenSource(const String &Name) override;
		bool CloseSource() override;
		bool IsSourceOpen() const override;
		SourceMode GetAvailableSourceModes() const noexcept override { return SourceMode::Push; }

	// BonDriverSourceFilter
		bool LoadBonDriver(const String &FileName);
		bool UnloadBonDriver();
		bool IsBonDriverLoaded() const;
		bool OpenTuner();
		bool CloseTuner();
		bool IsTunerOpen() const;

		bool SetChannel(uint8_t Channel);
		bool SetChannel(uint32_t Space, uint32_t Channel);
		bool SetChannelAndPlay(uint32_t Space, uint32_t Channel);
		uint32_t GetCurSpace() const;
		uint32_t GetCurChannel() const;

		bool IsIBonDriver2() const;

		const IBonDriver2::CharType * GetSpaceName(uint32_t Space) const;
		const IBonDriver2::CharType * GetChannelName(uint32_t Space, uint32_t Channel) const;
		int GetSpaceCount() const;
		const IBonDriver2::CharType * GetTunerName() const;

		bool PurgeStream();

		float GetSignalLevel() const;
		unsigned long GetBitRate() const;
		uint32_t GetStreamRemain() const;

		bool SetStreamingThreadPriority(int Priority);
		int GetStreamingThreadPriority() const noexcept { return m_StreamingThreadPriority; }
		void SetPurgeStreamOnChannelChange(bool Purge);
		bool IsPurgeStreamOnChannelChange() const noexcept { return m_PurgeStreamOnChannelChange; }
		bool SetFirstChannelSetDelay(unsigned long Delay);
		unsigned long GetFirstChannelSetDelay() const noexcept { return m_FirstChannelSetDelay; }
		bool SetMinChannelChangeInterval(unsigned long Interval);
		unsigned long GetMinChannelChangeInterval() const noexcept { return m_MinChannelChangeInterval; }

		static const ErrorCategory & GetErrorCategory() noexcept { return m_ErrorCategory; }

	private:
		struct StreamingRequest {
			enum class RequestType {
				End,
				Reset,
				SetChannel,
				SetChannel2,
				PurgeStream,
			};

			RequestType Type;
			bool IsProcessing;

			union {
				struct {
					uint8_t Channel;
				} SetChannel;
				struct {
					uint32_t Space;
					uint32_t Channel;
				} SetChannel2;
			};
		};

	// Thread
		const CharType * GetThreadName() const noexcept override { return LIBISDB_STR("BonDriverSource"); }
		void ThreadMain() override;

	// BonDriverSourceFilter
		void StreamingMain();

		void ResetStatus();
		void AddRequest(const StreamingRequest &Request);
		void AddRequests(const StreamingRequest *pRequestList, int RequestCount);
		bool WaitAllRequests(const std::chrono::milliseconds &Timeout);
		bool HasPendingRequest();
		void SetRequestTimeoutError();
		void SetChannelWait();

		static ErrorCategory m_ErrorCategory;

		BonDriver m_BonDriver;

		std::deque<StreamingRequest> m_RequestQueue;
		MutexLock m_RequestLock;
		ConditionVariable m_RequestQueued;
		ConditionVariable m_RequestProcessed;
		std::chrono::milliseconds m_RequestTimeout;
		bool m_RequestResult;

		std::atomic<bool> m_IsStreaming;

		float m_SignalLevel;
		BitRateCalculator m_BitRateCalculator;
		uint32_t m_StreamRemain;

		int m_StreamingThreadPriority;
		bool m_PurgeStreamOnChannelChange;

		unsigned long m_FirstChannelSetDelay;
		unsigned long m_MinChannelChangeInterval;
		TickClock::ClockType m_TunerOpenTime;
		TickClock::ClockType m_SetChannelTime;
		unsigned long m_SetChannelCount;

		TickClock m_Clock;
	};

	inline std::error_code make_error_code(BonDriverSourceFilter::ErrorCode Code) noexcept
	{
		return std::error_code(static_cast<int>(Code), BonDriverSourceFilter::GetErrorCategory());
	}

}	// namespace LibISDB


namespace std
{
	template <> struct is_error_code_enum<LibISDB::BonDriverSourceFilter::ErrorCode> : true_type {};
}


#endif	// ifndef LIBISDB_BON_DRIVER_SOURCE_FILTER_H
