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
 @file   DescriptorBase.hpp
 @brief  記述子基底クラス
 @author DBCTRADO
*/


#ifndef LIBISDB_DESCRIPTOR_BASE_H
#define LIBISDB_DESCRIPTOR_BASE_H


namespace LibISDB
{

	/** 記述子基底クラス */
	class DescriptorBase
	{
	public:
		DescriptorBase();
		virtual ~DescriptorBase() = default;

		virtual void CopyFrom(const DescriptorBase *pSrc);
		virtual DescriptorBase * Clone() const;

		bool Parse(const uint8_t *pData, uint16_t DataLength);

		bool IsValid() const noexcept { return m_IsValid; }
		uint8_t GetTag() const noexcept { return m_Tag; }
		uint8_t GetLength() const noexcept { return m_Length; }

		virtual void Reset();

	protected:
		virtual bool StoreContents(const uint8_t *pPayload);

		uint8_t m_Tag;    /**< 記述子タグ */
		uint8_t m_Length; /**< 記述子長 */
		bool m_IsValid;   /**< 解析結果 */
	};

	/** 記述子テンプレートクラス */
	template<typename T, uint8_t Tag> class DescriptorTemplate
		: public DescriptorBase
	{
	public:
		static constexpr uint8_t TAG = Tag;

		void CopyFrom(const DescriptorBase *pSrc) override
		{
			if (pSrc != this) {
				const T *pSrcDescriptor = dynamic_cast<const T *>(pSrc);

				if (pSrcDescriptor != nullptr) {
					*static_cast<T *>(this) = *pSrcDescriptor;
				} else {
					DescriptorBase::CopyFrom(pSrc);
				}
			}
		}

		DescriptorBase * Clone() const override
		{
			return new T(*static_cast<const T *>(this));
		}
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DESCRIPTOR_BASE_H
