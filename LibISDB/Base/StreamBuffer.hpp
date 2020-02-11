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
 @file   StreamBuffer.hpp
 @brief  ストリームバッファ
 @author DBCTRADO
*/


#ifndef LIBISDB_STREAM_BUFFER_H
#define LIBISDB_STREAM_BUFFER_H


#include "DataStorage.hpp"
#include "DataStorageManager.hpp"
#include "../Utilities/Lock.hpp"
#include <memory>
#include <deque>
#include <map>


namespace LibISDB
{

	/** ストリームバッファクラス */
	class StreamBuffer
	{
	public:
		typedef long long PosType;

		static constexpr PosType POS_BEGIN   = -1;
		static constexpr PosType POS_INVALID = -2;

		class Reader
		{
		public:
			virtual ~Reader() = default;

			virtual bool Open(const std::shared_ptr<StreamBuffer> &Buffer);
			virtual void Close();
			bool IsOpen() const noexcept { return static_cast<bool>(m_Buffer); }
			virtual size_t Read(void *pBuffer, size_t Size) = 0;
			virtual bool SetPos(PosType Pos) = 0;
			virtual bool SeekToBegin() = 0;
			virtual bool SeekToEnd() = 0;
			virtual bool IsDataAvailable() const = 0;

		protected:
			std::shared_ptr<StreamBuffer> m_Buffer;
		};

		class SequentialReader : public Reader
		{
		public:
			SequentialReader();
			~SequentialReader();

			bool Open(const std::shared_ptr<StreamBuffer> &Buffer) override;
			void Close() override;
			size_t Read(void *pBuffer, size_t Size) override;
			bool SetPos(PosType Pos) override;
			bool SeekToBegin() override;
			bool SeekToEnd() override;
			bool IsDataAvailable() const override;

		protected:
			void ResetPos();

			PosType m_Pos;
		};

		StreamBuffer() noexcept;
		~StreamBuffer();

		bool Create(
			size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount,
			DataStorageManager *pDataStorageManager = nullptr);
		void Destroy();
		bool IsCreated() const noexcept;
		void Clear();
		bool SetSize(size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount, bool Discard);
		bool IsEmpty() const;
		bool IsFull() const;
		size_t GetFreeSpace() const;
		size_t GetBlockSize() const noexcept { return m_BlockSize; }
		size_t GetMinBlockCount() const noexcept { return m_MinBlockCount; }
		size_t GetMaxBlockCount() const noexcept { return m_MaxBlockCount; }

		size_t PushBack(const uint8_t *pData, size_t DataSize);
		size_t PushBack(const DataBuffer *pData);

	private:
		class QueueBlock
		{
		public:
			QueueBlock() noexcept;
			QueueBlock(const QueueBlock &) = delete;
			QueueBlock(QueueBlock &&Src) noexcept;
			~QueueBlock();

			QueueBlock & operator = (const QueueBlock &) = delete;
			QueueBlock & operator = (QueueBlock &&Src) noexcept;

			bool SetStorage(DataStorage *pStorage);
			void Free() noexcept;
			void Reuse();
			size_t Write(const void *pData, size_t DataSize);
			size_t Read(size_t Offset, void *pData, size_t DataSize);

			size_t GetCapacity() const;
			size_t GetDataSize() const;
			bool IsFull() const;

			PosType GetSerialPos() const noexcept { return m_SerialPos; }
			void SetSerialPos(PosType Pos) noexcept { m_SerialPos = Pos; }

		private:
			std::unique_ptr<DataStorage> m_Storage;
			PosType m_SerialPos;
		};

		int GetBlockIndexBySerialPos(PosType Pos) const;
		QueueBlock * GetBlockBySerialPos(PosType Pos);
		bool SetReaderPos(Reader *pReader, PosType Pos);
		bool ResetReaderPos(Reader *pReader);
		PosType GetBeginPos() const;
		PosType GetEndPos() const;
		bool GetDataRange(ReturnArg<PosType> Begin, ReturnArg<PosType> End) const;
		bool IsBlockLocked(const QueueBlock &Block) const;
		void FreeUnusedBlocks();
		size_t Read(PosType *pPos, void *pBuffer, size_t Size);

		static bool CheckBufferSize(size_t BlockSize, size_t MinBlockCount, size_t MaxBlockCount);

		size_t m_BlockSize;
		size_t m_MinBlockCount;
		size_t m_MaxBlockCount;
		std::deque<QueueBlock> m_Queue;
		PosType m_SerialPos;
		mutable MutexLock m_Lock;
		std::shared_ptr<DataStorageManager> m_DataStorageManager;
		std::map<Reader *, PosType> m_ReaderPosList;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_STREAM_BUFFER_H
