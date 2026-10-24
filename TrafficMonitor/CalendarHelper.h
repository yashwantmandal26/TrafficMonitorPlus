#pragma once
#define CALENDAR_WIDTH 7
#define CALENDAR_HEIGHT 6

struct DayTraffic
{
	int day;
	__int64 up_traffic;
	__int64 down_traffic;
	bool mixed;
	CRect rect;

	__int64 traffic() const
	{
		return up_traffic + down_traffic;
	}
};

class CCalendarHelper
{
public:
	CCalendarHelper();
	~CCalendarHelper();

	//Is it a leap year?
	static bool IsLeapYear(int year);
	//Calculate the day of the week based on the date (0~6: Sunday~Saturday)
	static int CaculateWeekDay(int y, int m, int d);
	//Calculate how many days there are in a month
	static int DaysInMonth(int year, int month);

	//Get the calendar data of the specified month and save it in the calendar array
	//If sunday_first is true, make Sunday the first day of the week, otherwise, make Monday the first day of the week
	static void GetCalendar(int year, int month, DayTraffic calendar[CALENDAR_HEIGHT][CALENDAR_WIDTH], bool sunday_first = true);

};

