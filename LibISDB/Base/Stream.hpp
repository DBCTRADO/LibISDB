/*
  LibISDB
  Copyright(c) 2017 DBCTRADO

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
 @file   Stream.hpp
 @brief  ストリーム
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_H
#define LIBISDB_STREAM_H


#include "ErrorHandler.hpp"


namespace LibISDB
{

	/** ストリーム基底クラス */
	class Stream
		: public ErrorHandler
	{
	public:
#if defined(LIBISDB_WINDOWS) || (_FILE_OFFSET_BITS == 64)
		typedef unsigned long long SizeType;
		typedef long long OffsetType;
#else
		typedef unsigned long SizeType;
		typedef long OffsetType;
#endif

		enum class SetPosType {
			Begin,
			Current,
			End
		};

		Stream() = default;
		virtual ~Stream() = default;
		Stream(const Stream &) = delete;
		Stream & operator = (const Stream &) = delete;

		virtual bool Close() = 0;
		virtual bool IsOpen() const = 0;

		virtual size_t Read(void *pBuff, size_t Size) = 0;
		virtual size_t Write(const void *pBuff, size_t Size) = 0;
		virtual bool Flush() = 0;

		virtual SizeType GetSize() = 0;
		virtual OffsetType GetPos() = 0;
		virtual bool SetPos(OffsetType Pos, SetPosType Type = SetPosType::Begin) = 0;

		virtual bool IsEnd() const = 0;
	};

	/** ファイルストリーム基底クラス */
	class FileStreamBase
		: public Stream
	{
	public:
		/** オープンフラグ */
		enum class OpenFlag : unsigned int {
			None            = 0x0000U, /**< 指定なし */
			Read            = 0x0001U, /**< 読み込み */
			Write           = 0x0002U, /**< 書き出し */
			Create          = 0x0004U, /**< 作成 */
			Append          = 0x0008U, /**< 追加 */
			Truncate        = 0x0010U, /**< 切り詰める */
			New             = 0x0020U, /**< 新規作成 */
			ShareRead       = 0x0040U, /**< 共有読み込み */
			ShareWrite      = 0x0080U, /**< 共有書き出し */
			ShareDelete     = 0x0100U, /**< 共有削除 */
			SequentialRead  = 0x0200U, /**< 順次読み込み */
			RandomAccess    = 0x0400U, /**< ランダムアクセス */
			PriorityLow     = 0x0800U, /**< 低優先度 */
			PriorityIdle    = 0x1000U, /**< 最低優先度 */
		};

		virtual bool Open(const CStringView &FileName, OpenFlag Flags) = 0;

		virtual bool Preallocate(SizeType Size) { return false; }
		virtual bool SetPreallocationUnit(SizeType Unit) { return false; }
		virtual SizeType GetPreallocationUnit() const { return 0; }
		virtual SizeType GetPreallocatedSpace() { return 0; }

		const String & GetFileName() const { return m_FileName; }

	protected:
		String m_FileName;
	};

	LIBISDB_ENUM_FLAGS(FileStreamBase::OpenFlag)

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_H
