#pragma once
#include "CommonData.h"
class CHistoryTrafficFile
{
public:
	CHistoryTrafficFile(const wstring& file_path);
	~CHistoryTrafficFile();

	void Save() const;
	void SaveTodayOnly() const;	// Only update the first row (lines count) and today's record (second row) to reduce I/O when saving frequently
	void Load();
	void LoadSize();			//Read only the size of the file
	void Merge(const CHistoryTrafficFile& history_traffic, bool ignore_same_data = false);		//Merge another CHistoryTrafficFile object. If ignore_same_data is true, items with the same date are ignored, otherwise traffic data for the same date are added.
	void OnDateChanged();		//Called when the date changes, move today's record to the history record, and create a new today's record

	const wstring& GetFilePath() const { return m_file_path; }
	const void SetFilePath(const wstring& file_path) { m_file_path = file_path; }
	
	// Get the complete traffic list (for statistical functions), return deque reference to be compatible with existing code
	deque<HistoryTraffic>& GetTraffics() { if (m_cache_dirty) UpdateCache(); return m_traffics_cache; }
	const deque<HistoryTraffic>& GetTraffics() const { if (m_cache_dirty) UpdateCache(); return m_traffics_cache; }
	
	// Get today's record (first record)
	HistoryTraffic& GetTodayTraffic() { return m_today_traffic; }
	const HistoryTraffic& GetTodayTraffic() const { return m_today_traffic; }
	
	// Get history list
	const list<HistoryTraffic>& GetHistoryTraffics() const { return m_history_traffics; }
	
	__int64 GetTodayUpTraffic() const { return m_today_up_traffic; }
	__int64 GetTodayDownTraffic() const { return m_today_down_traffic; }
	size_t Size() const { return m_size; }

private:
	void MormalizeData();		//Sort historical traffic data and merge identical items
	bool IsTodayRecord() const;	//Check if today's recorded date is correct
	void UpdateCache() const;	//Update cache (for statistics functions)
	void WriteTrafficRecord(ofstream& file, const HistoryTraffic& traffic) const;	//Write a traffic record to a file
	HistoryTraffic CreateTodayTraffic() const;	//Create today's record (date is current date, traffic is 0)
	void InvalidateCache() const { m_cache_dirty = true; }	//Mark cache expired

private:
	wstring m_file_path;
	HistoryTraffic m_today_traffic;        // Today's records (stored separately, updated frequently)
	list<HistoryTraffic> m_history_traffics;	// History linked list (sorted by date from largest to smallest)
	mutable deque<HistoryTraffic> m_traffics_cache;	// Cache: merged complete list (used for statistical functions, updated on demand)
	mutable bool m_cache_dirty{ true };	// Whether the cache is expired (needs to be updated)
	__int64 m_today_up_traffic{};	//Upload traffic used today
	__int64 m_today_down_traffic{};	//Download traffic used today
	size_t m_size{};				//Amount of traffic data
};

