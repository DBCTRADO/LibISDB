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
 @file   DescriptorBlock.hpp
 @brief  記述子ブロック
 @author DBCTRADO
*/


#ifndef LIBISDB_DESCRIPTOR_BLOCK_H
#define LIBISDB_DESCRIPTOR_BLOCK_H


#include "DescriptorBase.hpp"
#include <vector>
#include <memory>


namespace LibISDB
{

	/** 記述子ブロッククラス */
	class DescriptorBlock
	{
	public:
		DescriptorBlock() = default;
		DescriptorBlock(const DescriptorBlock &Src);
		DescriptorBlock(DescriptorBlock &&Src);
		virtual ~DescriptorBlock() = default;
		DescriptorBlock & operator = (const DescriptorBlock &Src);
		DescriptorBlock & operator = (DescriptorBlock &&Src);

		int ParseBlock(const uint8_t *pData, size_t DataLength);
		const DescriptorBase * ParseBlock(const uint8_t *pData, size_t DataLength, uint8_t Tag);

		virtual void Reset();

		int GetDescriptorCount() const;
		const DescriptorBase * GetDescriptorByIndex(int Index) const;
		const DescriptorBase * GetDescriptorByTag(uint8_t Tag) const;
		template<typename T> const T * GetDescriptor() const {
			return dynamic_cast<const T *>(GetDescriptorByTag(T::TAG));
		}

		template<typename TDesc, typename TPred> void EnumDescriptors(TPred Pred) const
		{
			for (auto &e : m_DescriptorList) {
				if (e->GetTag() == TDesc::TAG) {
					const TDesc *pDesc = dynamic_cast<const TDesc *>(e.get());
					if (pDesc != nullptr)
						Pred(pDesc);
				}
			}
		}

	protected:
		std::unique_ptr<DescriptorBase> ParseDescriptor(const uint8_t *pData, uint16_t DataLength);
		static DescriptorBase * CreateDescriptorInstance(uint8_t Tag);

		std::vector<std::unique_ptr<DescriptorBase>> m_DescriptorList;
	};

}	// namespace LibISDB


#endif	// ifndef LIBISDB_DESCRIPTOR_BLOCK_H
