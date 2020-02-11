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
 @file   tspidinfo.cpp
 @brief  TS の PID 情報の出力

 TS ファイルの各 PID の情報を出力する。

 tspidinfo <filename>

 @author DBCTRADO
*/


#include "../LibISDB/LibISDB.hpp"
#include "../LibISDB/Engine/StreamSourceEngine.hpp"
#include "../LibISDB/Filters/StreamSourceFilter.hpp"
#include "../LibISDB/Filters/TSPacketParserFilter.hpp"
#include "../LibISDB/Filters/AnalyzerFilter.hpp"
#include "../LibISDB/Filters/AsyncStreamingFilter.hpp"
#include "../LibISDB/Base/StandardStream.hpp"
#include "../LibISDB/TS/TSInformation.hpp"
#include "../LibISDB/Utilities/StringUtilities.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>


namespace
{


class PIDInfoEngine : public LibISDB::StreamSourceEngine
{
public:
	LibISDB::String GetPIDDescription(std::uint16_t PID) const;

private:
	void OnPMTUpdated(LibISDB::AnalyzerFilter *pAnalyzer, std::uint16_t ServiceID) override;
	void OnCATUpdated(LibISDB::AnalyzerFilter *pAnalyzer) override;

	struct ESInfo {
		std::uint16_t PID;
		std::uint8_t StreamType;

		bool operator == (const ESInfo &rhs) const noexcept
		{
			return (PID == rhs.PID)
				&& (StreamType == rhs.StreamType);
		}

		bool operator != (const ESInfo &rhs) const noexcept
		{
			return !(*this == rhs);
		}
	};

	struct ServicePIDInfo {
		std::uint16_t ServiceID;
		std::vector<std::uint16_t> PMTPID;
		std::vector<std::uint16_t> PCRPID;
		std::vector<std::uint16_t> ECMPID;
		std::vector<ESInfo> ESList;
	};

	std::vector<ServicePIDInfo> m_ServiceList;
	LibISDB::AnalyzerFilter::EMMPIDList m_EMMPIDList;
};


LibISDB::String PIDInfoEngine::GetPIDDescription(std::uint16_t PID) const
{
	LibISDB::String Text;

	const LibISDB::CharType *pText = LibISDB::GetPredefinedPIDText(PID);
	if (pText != nullptr)
		Text += pText;

	if (auto it = std::find(m_EMMPIDList.begin(), m_EMMPIDList.end(), PID); it != m_EMMPIDList.end())
		Text += LIBISDB_STR("EMM");

	for (auto &Service : m_ServiceList) {
		LibISDB::CharType ServiceText[8];
		LibISDB::StringPrintf(ServiceText, std::size(ServiceText), LIBISDB_STR("[%04X]"), Service.ServiceID);

		for (std::uint16_t PMT : Service.PMTPID) {
			if (PMT == PID) {
				if (!Text.empty())
					Text += LIBISDB_CHAR(' ');

				Text += ServiceText;
				Text += LIBISDB_STR(" PMT");
				break;
			}
		}

		for (std::uint16_t PCR : Service.PCRPID) {
			if (PCR == PID) {
				if (!Text.empty())
					Text += LIBISDB_CHAR(' ');

				Text += ServiceText;
				Text += LIBISDB_STR(" PCR");
				break;
			}
		}

		for (std::uint16_t ECM : Service.ECMPID) {
			if (ECM == PID) {
				if (!Text.empty())
					Text += LIBISDB_CHAR(' ');

				Text += ServiceText;
				Text += LIBISDB_STR(" ECM");
				break;
			}
		}

		for (auto &ES : Service.ESList) {
			if (ES.PID == PID) {
				if (!Text.empty())
					Text += LIBISDB_CHAR(' ');

				Text += ServiceText;

				if (ES.StreamType == LibISDB::STREAM_TYPE_CAPTION)
					pText = LIBISDB_STR("Caption");
				else if (ES.StreamType == LibISDB::STREAM_TYPE_DATA_CARROUSEL)
					pText = LIBISDB_STR("Data");
				else
					pText = LibISDB::GetStreamTypeText(ES.StreamType);
				if (pText != nullptr) {
					Text += LIBISDB_CHAR(' ');
					Text += pText;
				}
			}
		}
	}

	return Text;
}


void PIDInfoEngine::OnPMTUpdated(LibISDB::AnalyzerFilter *pAnalyzer, std::uint16_t ServiceID)
{
	TSEngine::OnPMTUpdated(pAnalyzer, ServiceID);

	LibISDB::AnalyzerFilter::ServiceInfo ServiceInfo;

	pAnalyzer->GetServiceInfoByID(ServiceID, &ServiceInfo);

	auto itService = std::find_if(
		m_ServiceList.begin(), m_ServiceList.end(),
		[ServiceID](const ServicePIDInfo &Info) -> bool {
			return Info.ServiceID == ServiceID;
		});
	if (itService == m_ServiceList.end()) {
		m_ServiceList.emplace_back();
		itService = --m_ServiceList.end();
		itService->ServiceID = ServiceID;
	}

	auto itPMT = std::find(
		itService->PMTPID.begin(), itService->PMTPID.end(), ServiceInfo.PMTPID);
	if (itPMT == itService->PMTPID.end())
		itService->PMTPID.push_back(ServiceInfo.PMTPID);

	auto itPCR = std::find(
		itService->PCRPID.begin(), itService->PCRPID.end(), ServiceInfo.PCRPID);
	if (itPCR == itService->PCRPID.end())
		itService->PCRPID.push_back(ServiceInfo.PCRPID);

	for (const auto &ECM : ServiceInfo.ECMList) {
		auto itECM = std::find(
			itService->ECMPID.begin(), itService->ECMPID.end(), ECM.PID);
		if (itECM == itService->ECMPID.end())
			itService->ECMPID.push_back(ECM.PID);
	}

	for (const auto &ES : ServiceInfo.ESList) {
		ESInfo Info;

		Info.PID = ES.PID;
		Info.StreamType = ES.StreamType;

		auto itES = std::find(
			itService->ESList.begin(), itService->ESList.end(), Info);
		if (itES == itService->ESList.end())
			itService->ESList.push_back(Info);
	}
}


void PIDInfoEngine::OnCATUpdated(LibISDB::AnalyzerFilter *pAnalyzer)
{
	LibISDB::AnalyzerFilter::EMMPIDList List;

	if (pAnalyzer->GetEMMPIDList(&List)) {
		for (std::uint16_t PID : List) {
			auto it = std::find(m_EMMPIDList.begin(), m_EMMPIDList.end(), PID);
			if (it == m_EMMPIDList.end())
				m_EMMPIDList.push_back(PID);
		}
	}
}


}


#if defined(LIBISDB_WCHAR)
int wmain(int argc, wchar_t **argv)
#else
int main(int argc, char **argv)
#endif
{
#if defined(LIBISDB_WCHAR)
	auto &ErrOut = std::wcerr;
#else
	auto &ErrOut = std::cerr;
#endif

	if (argc < 2) {
		ErrOut << LIBISDB_STR("Need filename.") << std::endl;
		return 1;
	}

	LibISDB::StreamSourceFilter *pSource = new LibISDB::StreamSourceFilter;
	LibISDB::TSPacketParserFilter *pParser = new LibISDB::TSPacketParserFilter;
	LibISDB::AnalyzerFilter *pAnalyzer = new LibISDB::AnalyzerFilter;

#define ASYNC
#ifdef ASYNC
	pSource->SetSourceMode(LibISDB::SourceFilter::SourceMode::Pull);
	LibISDB::AsyncStreamingFilter *pAsyncStreaming = new LibISDB::AsyncStreamingFilter;
	pAsyncStreaming->SetSourceFilter(pSource);
	pAsyncStreaming->CreateBuffer(pAsyncStreaming->GetOutputBufferSize(), 3, 3);
#endif

	PIDInfoEngine Engine;

	Engine.BuildEngine({
		pSource,
#ifdef ASYNC
		pAsyncStreaming,
#endif
		pParser,
		pAnalyzer
		});
	Engine.SetStartStreamingOnSourceOpen(true);

	const LibISDB::CharType *pFileName = argv[1];
	if (LibISDB::StringCompare(pFileName, LIBISDB_STR("-")) == 0)
		pFileName = LibISDB::StandardInputStream::Name;
	if (!Engine.OpenSource(pFileName)) {
		ErrOut << LIBISDB_STR("Failed to open file : ") << pFileName << std::endl;
		return 1;
	}

	Engine.WaitForEndOfStream();
#ifdef ASYNC
	pAsyncStreaming->WaitForEndOfStream();
#endif
	Engine.CloseSource();

	LibISDB::TSPacketParserFilter::PacketCountInfo TotalCount = pParser->GetTotalPacketCount();
	LibISDB::TSPacketParserFilter::PacketCountInfo PIDCount[LibISDB::PID_MAX + 1];
	for (std::uint16_t i = 0; i <= LibISDB::PID_MAX; i++) {
		PIDCount[i] = pParser->GetTotalPacketCount(i);
	}

#if defined(LIBISDB_WCHAR)
	auto &Out = std::wcout;
#else
	auto &Out = std::cout;
#endif
	const int CountDigits = 9;

	Out << LIBISDB_STR("Input Bytes     : ") << std::setw(CountDigits) << pParser->GetTotalInputBytes() << std::endl;
	Out << LIBISDB_STR("Input Packets   : ") << std::setw(CountDigits) << TotalCount.Input           << std::endl;
	Out << LIBISDB_STR("Format Error    : ") << std::setw(CountDigits) << TotalCount.FormatError     << std::endl;
	Out << LIBISDB_STR("Transport Error : ") << std::setw(CountDigits) << TotalCount.TransportError  << std::endl;
	Out << LIBISDB_STR("Dropped         : ") << std::setw(CountDigits) << TotalCount.ContinuityError << std::endl;
	Out << LIBISDB_STR("Scrambled       : ") << std::setw(CountDigits) << TotalCount.Scrambled       << std::endl;

	Out << std::endl;

	Out << LIBISDB_STR(" PID : ");
	Out << std::setw(CountDigits) << LIBISDB_STR("Input")     << LIBISDB_STR(" ");
	Out << std::setw(CountDigits) << LIBISDB_STR("Dropped")   << LIBISDB_STR(" ");
	Out << std::setw(CountDigits) << LIBISDB_STR("Scrambled") << LIBISDB_STR(" ");
	Out << LIBISDB_STR(": Description") << std::endl;

	for (std::uint16_t i = 0; i <= LibISDB::PID_MAX; i++) {
		if (PIDCount[i].Input > 0) {
			Out << std::hex << std::uppercase << std::setfill(LIBISDB_CHAR('0')) << std::setw(4) << i << LIBISDB_STR(" : ");
			Out << std::dec << std::setfill(LIBISDB_CHAR(' '));
			Out << std::setw(CountDigits) << PIDCount[i].Input           << LIBISDB_STR(" ");
			Out << std::setw(CountDigits) << PIDCount[i].ContinuityError << LIBISDB_STR(" ");
			Out << std::setw(CountDigits) << PIDCount[i].Scrambled       << LIBISDB_STR(" ");
			Out << LIBISDB_STR(": ") << Engine.GetPIDDescription(i) << std::endl;
		}
	}

	return 0;
}
