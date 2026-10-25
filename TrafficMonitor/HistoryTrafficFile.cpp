#include "stdafx.h"
#include "HistoryTrafficFile.h"
#include "Common.h"

CHistoryTrafficFile::CHistoryTrafficFile(const wstring& file_path)
	: m_file_path(file_path)
{
}

CHistoryTrafficFile::~CHistoryTrafficFile()
{
}

HistoryTraffic CHistoryTrafficFile::CreateTodayTraffic() const
{
	SYSTEMTIME current_time;
	GetLocalTime(&current_time);
	HistoryTraffic today_traffic;
	today_traffic.year = current_time.wYear;
	today_traffic.month = current_time.wMonth;
	today_traffic.day = current_time.wDay;
	today_traffic.up_kBytes = 0;
	today_traffic.down_kBytes = 0;
	today_traffic.mixed = false;
	return today_traffic;
}

void CHistoryTrafficFile::WriteTrafficRecord(ofstream& file, const HistoryTraffic& traffic) const
{
	char buff[64];
	if (traffic.mixed)
	{
		sprintf_s(buff, "%.4d/%.2d/%.2d %llu", traffic.year, traffic.month, 
			traffic.day, traffic.down_kBytes);
	}
	else
	{
		sprintf_s(buff, "%.4d/%.2d/%.2d %llu/%llu", traffic.year, traffic.month, 
			traffic.day, traffic.up_kBytes, traffic.down_kBytes);
	}
	file << buff << "\n";
}

void CHistoryTrafficFile::UpdateCache() const
{
	// Update cache: merge today’s records and history list
	m_traffics_cache.clear();
	m_traffics_cache.push_front(m_today_traffic);
	m_traffics_cache.insert(m_traffics_cache.end(), m_history_traffics.begin(), m_history_traffics.end());
	m_cache_dirty = false; // Tag cache updated
}

void CHistoryTrafficFile::Save() const
{
	return; // Disk writes disabled
	ofstream file{ m_file_path };
	if (!file.is_open())
	{
		return;
	}

	char buff[64];
	
	// First line: total number of records (today’s records + historical records)
	size_t total_size = 1 + m_history_traffics.size();
	sprintf_s(buff, "lines: \"%u\"", static_cast<unsigned int>(total_size));
	file << buff << "\n";

	// Second line: Today’s record
	WriteTrafficRecord(file, m_today_traffic);

	// The third line and after: history linked list
	for (const auto& history_traffic : m_history_traffics)
	{
		WriteTrafficRecord(file, history_traffic);
	}

	file.close();
}

bool CHistoryTrafficFile::IsTodayRecord() const
{
	// Check if today's recorded date is correct
	SYSTEMTIME current_time;
	GetLocalTime(&current_time);
	
	return (m_today_traffic.year == current_time.wYear &&
			m_today_traffic.month == current_time.wMonth &&
			m_today_traffic.day == current_time.wDay);
}

void CHistoryTrafficFile::SaveTodayOnly() const
{
	return; // Disk writes disabled
	// Incremental save: only update the first row (lines count) and second row (today's record)
	// Prerequisite: Today’s record date is correct (the program maintains it by itself, no need to read the file to judge)
	
	// If today's record date is incorrect, it means the date has just changed, use full save
	if (!IsTodayRecord())
	{
		Save();
		return;
	}

	// When file does not exist, use full save (first save)
	if (!CCommon::FileExist(m_file_path.c_str()))
	{
		Save();
		return;
	}

	// Read the remaining lines of the file (line 3 and after) for incremental updates
	vector<string> remaining_lines;
	ifstream in_file{ m_file_path };
	if (in_file.is_open())
	{
		string line;
		int line_num = 0;
		while (std::getline(in_file, line))
		{
			line_num++;
			if (line_num > 2) // skip first two lines
			{
				remaining_lines.push_back(line);
			}
		}
		in_file.close();
	}

	// Write updated file
	ofstream out_file{ m_file_path };
	if (!out_file.is_open())
	{
		return;
	}

	// First line: lines count (today’s record + history)
	size_t total_size = 1 + m_history_traffics.size();
	char buff[64];
	sprintf_s(buff, "lines: \"%u\"", static_cast<unsigned int>(total_size));
	out_file << buff << "\n";

	// Second line: Today’s record
	WriteTrafficRecord(out_file, m_today_traffic);

	// Remaining lines: read from file, write directly (no formatting)
	for (const auto& line : remaining_lines)
	{
		out_file << line << "\n";
	}

	out_file.close();
}

void CHistoryTrafficFile::Load()
{
	m_today_traffic = HistoryTraffic{}; // Initialize today’s records
	m_history_traffics.clear(); // Clear history list
	InvalidateCache(); // Mark cache expired

	ifstream file{ m_file_path };
	string current_line, temp;
	HistoryTraffic traffic;
	bool is_first_data_line = true; // Whether the mark is the first data row (today's record)
	
	if (CCommon::FileExist(m_file_path.c_str()))
	{
		// Skip the first line (lines:)
		std::getline(file, current_line);
		
		while (!file.eof())
		{
			std::getline(file, current_line);

			if (current_line.size() < 12)
			{
				continue;
			}
			temp = current_line.substr(0, 4);
			traffic.year = atoi(temp.c_str());
			if (traffic.year < 1900 || traffic.year > 3000)
			{
				continue;
			}
			temp = current_line.substr(5, 2);
			traffic.month = atoi(temp.c_str());
			if (traffic.month < 1 || traffic.month > 12)
			{
				continue;
			}
			temp = current_line.substr(8, 2);
			traffic.day = atoi(temp.c_str());
			if (traffic.day < 1 || traffic.day > 31)
			{
				continue;
			}

			size_t separator_index = current_line.find(L'/', 11);
			traffic.mixed = (separator_index == wstring::npos);
			if (traffic.mixed)
			{
				temp = current_line.substr(11);
				traffic.down_kBytes = atoll(temp.c_str());
				traffic.up_kBytes = 0;
			}
			else
			{
				temp = current_line.substr(11, separator_index - 11);
				traffic.up_kBytes = atoll(temp.c_str());
				temp = current_line.substr(separator_index + 1);
				traffic.down_kBytes = atoll(temp.c_str());
			}
			
			if (traffic.year > 0 && traffic.month > 0 && traffic.day > 0 && traffic.kBytes() > 0)
			{
				if (is_first_data_line)
				{
					// The first data row is today's record
					m_today_traffic = traffic;
					is_first_data_line = false;
				}
				else
				{
					// The rest is history, inserted into the linked list
					m_history_traffics.push_back(traffic);
				}
			}
		}
	}

	MormalizeData();
}

void CHistoryTrafficFile::LoadSize()
{
	ifstream file{ m_file_path };
	string current_line, temp;
	if (CCommon::FileExist(m_file_path.c_str()))
	{
		std::getline(file, current_line); // Read first line
		size_t index = current_line.find("lines:");
		if (index != wstring::npos)
		{
			index = current_line.find("\"", index + 6);
			size_t index1 = current_line.find("\"", index + 1);
			temp = current_line.substr(index + 1, index1 - index - 1);
			m_size = atoll(temp.c_str());
		}
	}
}

void CHistoryTrafficFile::Merge(const CHistoryTrafficFile& history_traffic, bool ignore_same_data)
{
	HistoryTraffic today_traffic = CreateTodayTraffic();

	// Merge today's records (only merge records with the same date)
	// Note: If ignore_same_data=true, it means that you are restoring from backup. You should take a larger value instead of accumulation to avoid repeated accumulation.
	if (HistoryTraffic::DateEqual(m_today_traffic, history_traffic.m_today_traffic))
	{
		if (ignore_same_data)
		{
			// When restoring from backup, take a larger value (to avoid repeated accumulation)
			// The backup file is usually the complete data when the program exits, and the current file may be the incomplete data after the program is started.
			if (history_traffic.m_today_traffic.up_kBytes > m_today_traffic.up_kBytes)
			{
				m_today_traffic.up_kBytes = history_traffic.m_today_traffic.up_kBytes;
			}
			if (history_traffic.m_today_traffic.down_kBytes > m_today_traffic.down_kBytes)
			{
				m_today_traffic.down_kBytes = history_traffic.m_today_traffic.down_kBytes;
			}
		}
		else
		{
			// During normal merging, the data is accumulated
			m_today_traffic.up_kBytes += history_traffic.m_today_traffic.up_kBytes;
			m_today_traffic.down_kBytes += history_traffic.m_today_traffic.down_kBytes;
		}
	}

	// Merge history linked list
	for (const HistoryTraffic& traffic : history_traffic.m_history_traffics)
	{
		// Skip "future" records (system time may have been adjusted)
		if (HistoryTraffic::DateGreater(traffic, today_traffic))
		{
			continue; // Skip "future" records
		}

		if (ignore_same_data)
		{
			// If you want to ignore items with the same date, use linear search (list does not support random access)
			auto it = std::find_if(m_history_traffics.begin(), m_history_traffics.end(),
				[&traffic](const HistoryTraffic& existing) {
					return HistoryTraffic::DateEqual(existing, traffic);
				});
			if (it != m_history_traffics.end())
			{
				continue; // Find records with the same date, skip
			}
		}
		m_history_traffics.push_back(traffic);
	}
	
	MormalizeData();
	InvalidateCache(); // Mark cache expired
}

void CHistoryTrafficFile::OnDateChanged()
{
	// When the date changes, move today's record to the front of the history list, and then create a new today's record
	
	// If today's record has data, move it to the history linked list
	if (m_today_traffic.kBytes() > 0)
	{
		m_history_traffics.push_front(m_today_traffic);
		// Sort immediately to ensure data consistency (from large to small by date)
		if (m_history_traffics.size() >= 2)
		{
			m_history_traffics.sort(HistoryTraffic::DateGreater);
		}
	}
	
	// Create a new record for today
	m_today_traffic = CreateTodayTraffic();
	
	// Update statistics
	m_today_up_traffic = 0;
	m_today_down_traffic = 0;
	m_size = 1 + m_history_traffics.size();
	InvalidateCache(); // Mark cache expired
}

void CHistoryTrafficFile::MormalizeData()
{
	HistoryTraffic today_traffic = CreateTodayTraffic();

	// First sort the history linked list (from large to small by date) to facilitate subsequent search and merging
	if (m_history_traffics.size() >= 2)
	{
		m_history_traffics.sort(HistoryTraffic::DateGreater);

		// Merge records with the same date
		auto it = m_history_traffics.begin();
		while (it != m_history_traffics.end())
		{
			auto next_it = it;
			++next_it;
			if (next_it != m_history_traffics.end() && HistoryTraffic::DateEqual(*it, *next_it))
			{
				it->up_kBytes += next_it->up_kBytes;
				it->down_kBytes += next_it->down_kBytes;
				m_history_traffics.erase(next_it);
			}
			else
			{
				++it;
			}
		}
	}

	// Clean up history records with dates later than the current date (the system time may have been adjusted)
	// Historical records should all be past dates and should not have future dates.
	if (!m_history_traffics.empty())
	{
		auto it = m_history_traffics.begin();
		while (it != m_history_traffics.end())
		{
			// If the date of the historical record is later than today, it means that it is a "future" record and should be deleted.
			if (HistoryTraffic::DateGreater(*it, today_traffic))
			{
				it = m_history_traffics.erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	// If the date of m_today_traffic is later than the current date, it means that the system time has been adjusted and should be reset.
	if (HistoryTraffic::DateGreater(m_today_traffic, today_traffic))
	{
		// If today's record has data, it should be moved to the history record (but the date is later than today and will be deleted by the above cleaning logic)
		// Reset directly to today's record
		m_today_traffic = today_traffic;
	}

	// Find today's record in the history (maybe the history contains today's data)
	auto it = std::find_if(m_history_traffics.begin(), m_history_traffics.end(),
		[&today_traffic](const HistoryTraffic& traffic) {
			return HistoryTraffic::DateEqual(traffic, today_traffic);
		});

	if (it != m_history_traffics.end())
	{
		// Today's record was found in history
		if (HistoryTraffic::DateEqual(m_today_traffic, today_traffic))
		{
			// If m_today_traffic is also from today, merge the data (to avoid data loss)
			m_today_traffic.up_kBytes += it->up_kBytes;
			m_today_traffic.down_kBytes += it->down_kBytes;
		}
		else
		{
			// If m_today_traffic is not from today, replace it with the one from history
			m_today_traffic = *it;
		}
		// Remove today's record from history (since it should only be in m_today_traffic)
		m_history_traffics.erase(it);
	}
	else if (!HistoryTraffic::DateEqual(m_today_traffic, today_traffic))
	{
		// There is no record for today in the history, and m_today_traffic is not for today.
		// If m_today_traffic has data, it should be moved to the history linked list
		if (m_today_traffic.kBytes() > 0)
		{
			m_history_traffics.push_front(m_today_traffic);
			// Reorder (because new records were inserted)
			if (m_history_traffics.size() >= 2)
			{
				m_history_traffics.sort(HistoryTraffic::DateGreater);
			}
		}
		// Create a new record for today
		m_today_traffic = today_traffic;
	}

	// Update traffic statistics for today
	m_today_up_traffic = static_cast<__int64>(m_today_traffic.up_kBytes) * 1024;
	m_today_down_traffic = static_cast<__int64>(m_today_traffic.down_kBytes) * 1024;
	m_today_traffic.mixed = false;

	// Update total number of records
	m_size = 1 + m_history_traffics.size();
	InvalidateCache(); // Mark cache expired
}
