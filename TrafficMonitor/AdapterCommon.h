#pragma once
#include "Common.h"

//Save a network connection information
struct NetWorkConection
{
	int index{};			//The index of this connection in MIB_IFTABLE
	string description;		//Network description (obtained from GetAdapterInfo)
	string description_2;	//Network description (obtained from GetIfTable)
	unsigned int in_bytes;	//Number of bytes received initially
	unsigned int out_bytes;	//Number of bytes sent initially
	wstring ip_address{ L"-.-.-.-" };	//IP address
	wstring subnet_mask{ L"-.-.-.-" };	//subnet mask
	wstring default_gateway{ L"-.-.-.-" };	//Default gateway
};

class CAdapterCommon
{
public:
	CAdapterCommon();
	~CAdapterCommon();

	//Get the network connection list and fill in the network description, IP address, subnet mask, and default gateway information
	static void GetAdapterInfo(vector<NetWorkConection>& adapters);

	//Refresh the IP address, subnet mask, and default gateway information in the network connection list
	static void RefreshIpAddress(vector<NetWorkConection>& adapters);

	//Get the index in MIB_IFTABLE of each network connection in the network list, and the information about the number of bytes received/sent initially
	static void GetIfTableInfo(vector<NetWorkConection>& adapters, MIB_IFTABLE* pIfTable);

	//Directly add all connections in MIB_IFTABLE to the adapters container
	static void GetAllIfTableInfo(vector<NetWorkConection>& adapters, MIB_IFTABLE* pIfTable);
private:
	//Determine whether it is in the IfTable list based on a network connection description and return the index. If not found, return -1.
	static int FindConnectionInIfTable(string connection, MIB_IFTABLE* pIfTable);

	//Determine whether it is in the IfTable connection list based on a network connection description, and return the index. If not found, return -1. Only a partial match is required
	static int FindConnectionInIfTableFuzzy(string connection, MIB_IFTABLE* pIfTable);
};

