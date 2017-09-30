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
 @file   epgdatatojson.cpp
 @brief  EPG データを JSON に変換する

 EPG データファイルの番組情報を JSON に変換する。

 epgdatatojson <filename>

 @author DBCTRADO
*/


#include "../LibISDB/LibISDB.hpp"
#include "../LibISDB/EPG/EPGDataFile.hpp"
#include "../LibISDB/Utilities/StringUtilities.hpp"
#include <algorithm>
#include <iostream>
#include <iomanip>

#if defined(LIBISDB_WINDOWS) && defined(LIBISDB_WCHAR)
#include <fcntl.h>
#include <io.h>
#endif


namespace
{


LibISDB::String EscapeString(const LibISDB::String &Src)
{
	LibISDB::String Dst;

	for (LibISDB::CharType c : Src) {
		switch (c) {
		case LIBISDB_CHAR('"'):
			Dst += LIBISDB_STR("\\\"");
			break;
		case LIBISDB_CHAR('\\'):
			Dst += LIBISDB_STR("\\\\");
			break;
		case LIBISDB_CHAR('\r'):
			Dst += LIBISDB_STR("\\r");
			break;
		case LIBISDB_CHAR('\n'):
			Dst += LIBISDB_STR("\\n");
			break;
		case LIBISDB_CHAR('\t'):
			Dst += LIBISDB_STR("\\t");
			break;
		default:
			Dst += c;
			break;
		}
	}

	return Dst;
}


template<typename TOut> class JSONFormatter
{
public:
	JSONFormatter(TOut &Out)
		: m_Out(Out)
		, m_Comma(false)
		, m_Indent(0)
		, m_UseIndent(true)
	{
	}

	void OutValue(const LibISDB::CharType *pKey, const LibISDB::String &Value)
	{
		PreValue();
		m_Out << LIBISDB_STR("\"") << pKey << LIBISDB_STR("\":\"") << EscapeString(Value) << LIBISDB_STR("\"");
	}

	void OutValue(const LibISDB::CharType *pKey, const LibISDB::DateTime &Time)
	{
		PreValue();
		m_Out << LIBISDB_STR("\"") << pKey << LIBISDB_STR("\":\"") 
			<< Time.Year << LIBISDB_STR("-") << Time.Month << LIBISDB_STR("-") << Time.Day
			<< LIBISDB_STR("T") << Time.Hour << LIBISDB_STR(":") << Time.Minute << LIBISDB_STR(":") << Time.Second
			<< LIBISDB_STR("+09:00\"");
	}

	template<typename T> void OutValue(const LibISDB::CharType *pKey, const T &Value)
	{
		PreValue();
		m_Out << LIBISDB_STR("\"") << pKey << LIBISDB_STR("\":") << Value;
	}

	void BeginObject()
	{
		if (m_Comma) {
			OutComma();
			m_Comma = false;
		}
		OutIndent();
		m_Out << LIBISDB_STR("{") << std::endl;
		m_Indent++;
	}

	void EndObject()
	{
		m_Out << std::endl;
		m_Indent--;
		OutIndent();
		m_Out << LIBISDB_STR("}");
		m_Comma = true;
	}

	void BeginArray(const LibISDB::CharType *pKey)
	{
		if (m_Comma) {
			OutComma();
			m_Comma = false;
		}
		OutIndent();
		m_Out << LIBISDB_STR("\"") << pKey << LIBISDB_STR("\":[") << std::endl;
		m_Indent++;
	}

	void EndArray()
	{
		m_Out << std::endl;
		m_Indent--;
		OutIndent();
		m_Out << LIBISDB_STR("]");
		m_Comma = true;
	}

private:
	void OutComma()
	{
		if (m_Comma)
			m_Out << LIBISDB_STR(",") << std::endl;
		else
			m_Comma = true;
	}

	void OutIndent()
	{
		if (m_UseIndent) {
			for (int i = 0; i < m_Indent; i++)
				m_Out << LIBISDB_STR("\t");
		}
	}

	void PreValue()
	{
		OutComma();
		OutIndent();
	}

	TOut &m_Out;
	bool m_Comma;
	int m_Indent;
	bool m_UseIndent;
};


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
		ErrOut << LIBISDB_STR("Need filename") << std::endl;
		return 1;
	}

#if defined(LIBISDB_WINDOWS) && defined(LIBISDB_WCHAR)
	::_setmode(::_fileno(stdout), _O_U8TEXT);
#endif

	LibISDB::EPGDataFile File;
	LibISDB::EPGDatabase Database;

	if (!File.Open(
			&Database, argv[1],
			LibISDB::EPGDataFile::OpenFlag::Read |
			LibISDB::EPGDataFile::OpenFlag::ShareRead)) {
		ErrOut << LIBISDB_STR("Failed to open file : ") << argv[1] << std::endl;
		return 1;
	}

	if (!File.Load()) {
		ErrOut << LIBISDB_STR("Load error") << std::endl;
		return 1;
	}

	File.Close();

#if defined(LIBISDB_WCHAR)
	auto &Out = std::wcout;
#else
	auto &Out = std::cout;
#endif

	JSONFormatter<decltype(Out)> JSON(Out);

	LibISDB::EPGDatabase::ServiceList ServiceList;

	Database.GetServiceList(&ServiceList);

	JSON.BeginObject();
	JSON.BeginArray(LIBISDB_STR("serviceList"));

	for (auto const &Service : ServiceList) {
		JSON.BeginObject();

		JSON.OutValue(LIBISDB_STR("serviceId"), Service.ServiceID);
		JSON.OutValue(LIBISDB_STR("networkId"), Service.NetworkID);
		JSON.OutValue(LIBISDB_STR("transportStreamId"), Service.TransportStreamID);

		LibISDB::EPGDatabase::EventList EventList;

		Database.GetEventListSortedByTime(
			Service.NetworkID, Service.TransportStreamID, Service.ServiceID,
			&EventList);

		JSON.BeginArray(LIBISDB_STR("eventList"));

		for (auto const &Event : EventList) {
			JSON.BeginObject();

			JSON.OutValue(LIBISDB_STR("eventId"), Event.EventID);
			JSON.OutValue(LIBISDB_STR("eventName"), Event.EventName);
			JSON.OutValue(LIBISDB_STR("eventText"), Event.EventText);

			JSON.BeginArray(LIBISDB_STR("extendedText"));
			for (auto const &Text : Event.ExtendedText) {
				JSON.BeginObject();
				JSON.OutValue(LIBISDB_STR("description"), Text.Description);
				JSON.OutValue(LIBISDB_STR("text"), Text.Text);
				JSON.EndObject();
			}
			JSON.EndArray();

			JSON.OutValue(LIBISDB_STR("startTime"), Event.StartTime);
			JSON.OutValue(LIBISDB_STR("duration"), Event.Duration);
			JSON.OutValue(LIBISDB_STR("freeCaMode"), Event.FreeCAMode);

			if (!Event.VideoList.empty()) {
				JSON.BeginArray(LIBISDB_STR("videoList"));
				for (auto const &Video : Event.VideoList) {
					JSON.BeginObject();
					JSON.OutValue(LIBISDB_STR("streamContent"), Video.StreamContent);
					JSON.OutValue(LIBISDB_STR("componentType"), Video.ComponentType);
					JSON.OutValue(LIBISDB_STR("componentTag"), Video.ComponentTag);
					JSON.OutValue(LIBISDB_STR("languageCode"), Video.LanguageCode);
					JSON.OutValue(LIBISDB_STR("text"), Video.Text);
					JSON.EndObject();
				}
				JSON.EndArray();
			}

			if (!Event.AudioList.empty()) {
				JSON.BeginArray(LIBISDB_STR("audioList"));
				for (auto const &Audio : Event.AudioList) {
					JSON.BeginObject();
					JSON.OutValue(LIBISDB_STR("streamContent"), Audio.StreamContent);
					JSON.OutValue(LIBISDB_STR("componentType"), Audio.ComponentType);
					JSON.OutValue(LIBISDB_STR("componentTag"), Audio.ComponentTag);
					JSON.OutValue(LIBISDB_STR("multiLingual"), Audio.ESMultiLingualFlag);
					JSON.OutValue(LIBISDB_STR("mainComponent"), Audio.MainComponentFlag);
					JSON.OutValue(LIBISDB_STR("languageCode"), Audio.LanguageCode);
					JSON.OutValue(LIBISDB_STR("languageCode2"), Audio.LanguageCode2);
					JSON.OutValue(LIBISDB_STR("text"), Audio.Text);
					JSON.EndObject();
				}
				JSON.EndArray();
			}

			if (Event.ContentNibble.NibbleCount > 0) {
				JSON.BeginArray(LIBISDB_STR("contentNibble"));
				for (int i = 0; i < Event.ContentNibble.NibbleCount; i++) {
					JSON.BeginObject();
					JSON.OutValue(LIBISDB_STR("level1"), Event.ContentNibble.NibbleList[i].ContentNibbleLevel1);
					JSON.OutValue(LIBISDB_STR("level2"), Event.ContentNibble.NibbleList[i].ContentNibbleLevel2);
					JSON.OutValue(LIBISDB_STR("user1"), Event.ContentNibble.NibbleList[i].UserNibble1);
					JSON.OutValue(LIBISDB_STR("user2"), Event.ContentNibble.NibbleList[i].UserNibble2);
					JSON.EndObject();
				}
				JSON.EndArray();
			}

			if (!Event.EventGroupList.empty()) {
				JSON.BeginArray(LIBISDB_STR("eventGroup"));
				for (auto const &Group : Event.EventGroupList) {
					JSON.BeginObject();
					JSON.OutValue(LIBISDB_STR("groupType"), Group.GroupType);
					if (!Group.EventList.empty()) {
						JSON.BeginArray(LIBISDB_STR("eventList"));
						for (auto const &Event : Group.EventList) {
							JSON.BeginObject();
							JSON.OutValue(LIBISDB_STR("serviceId"), Event.ServiceID);
							JSON.OutValue(LIBISDB_STR("eventId"), Event.EventID);
							JSON.OutValue(LIBISDB_STR("networkId"), Event.NetworkID);
							JSON.OutValue(LIBISDB_STR("transportStreamId"), Event.TransportStreamID);
							JSON.EndObject();
						}
						JSON.EndArray();
					}
					JSON.EndObject();
				}
				JSON.EndArray();
			}

			if (Event.IsCommonEvent) {
				JSON.OutValue(LIBISDB_STR("commonServiceId"), Event.CommonEvent.ServiceID);
				JSON.OutValue(LIBISDB_STR("commonEventId"), Event.CommonEvent.EventID);
			}

			JSON.EndObject();
		}

		JSON.EndArray();	// "eventList"
		JSON.EndObject();
	}

	JSON.EndArray();	// "serviceList"
	JSON.EndObject();

	return 0;
}
