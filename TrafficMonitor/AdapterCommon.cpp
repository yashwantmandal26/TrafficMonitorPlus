#include "stdafx.h"
#include "AdapterCommon.h"


CAdapterCommon::CAdapterCommon()
{
}


CAdapterCommon::~CAdapterCommon()
{
}

void CAdapterCommon::GetAdapterInfo(vector<NetWorkConection>& adapters)
{
	adapters.clear();
	PIP_ADAPTER_INFO pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[sizeof(IP_ADAPTER_INFO)];		//The PIP_ADAPTER_INFO structure pointer stores local network card information
	unsigned long stSize = sizeof(IP_ADAPTER_INFO);		//Get the structure size, used for GetAdaptersInfo parameters
	int nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);	//Call the GetAdaptersInfo function and fill in the pIpAdapterInfo pointer variable; the stSize parameter is both an input and an output.

	if (ERROR_BUFFER_OVERFLOW == nRel)
	{
		//If the function returns ERROR_BUFFER_OVERFLOW
		//It means that the memory space passed by the GetAdaptersInfo parameter is not enough, and at the same time, it passes out stSize, indicating the required space size.
		//This also explains why stSize is both an input quantity and an output quantity.
		delete[] (BYTE*)pIpAdapterInfo;	//Release original memory space
		pIpAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[stSize];	//Reapply for memory space to store all network card information
		nRel = GetAdaptersInfo(pIpAdapterInfo, &stSize);		//Call the GetAdaptersInfo function again to fill in the pIpAdapterInfo pointer variable
	}

	PIP_ADAPTER_INFO pIpAdapterInfoHead = pIpAdapterInfo;	//Save the address of the first element in the pIpAdapterInfo linked list
	if (ERROR_SUCCESS == nRel)
	{
		while (pIpAdapterInfo)
		{
			NetWorkConection connection;
			connection.description = pIpAdapterInfo->Description;
			connection.ip_address = CCommon::StrToUnicode(pIpAdapterInfo->IpAddressList.IpAddress.String);
			connection.subnet_mask = CCommon::StrToUnicode(pIpAdapterInfo->IpAddressList.IpMask.String);
			connection.default_gateway = CCommon::StrToUnicode(pIpAdapterInfo->GatewayList.IpAddress.String);

			adapters.push_back(connection);
			pIpAdapterInfo = pIpAdapterInfo->Next;
		}
	}
	//Free up memory space
	if (pIpAdapterInfoHead)
	{
		delete[] (BYTE*)pIpAdapterInfoHead;
	}
	if (adapters.empty())
	{
		NetWorkConection connection{};
		connection.description = CCommon::UnicodeToStr(CCommon::LoadText(L"<", IDS_NO_CONNECTION, L">"));
		adapters.push_back(connection);
	}
}

void CAdapterCommon::RefreshIpAddress(vector<NetWorkConection>& adapters)
{
	vector<NetWorkConection> adapters_tmp;
	GetAdapterInfo(adapters_tmp);
	for (const auto& adapter_tmp : adapters_tmp)
	{
		for (auto& adapter : adapters)
		{
			if (adapter_tmp.description == adapter.description)
			{
				adapter.ip_address = adapter_tmp.ip_address;
				adapter.subnet_mask = adapter_tmp.subnet_mask;
				adapter.default_gateway = adapter_tmp.default_gateway;
			}
		}
	}
}

void CAdapterCommon::GetIfTableInfo(vector<NetWorkConection>& adapters, MIB_IFTABLE* pIfTable)
{
	//Find each connection in the IfTable in turn
	for (size_t i{}; i < adapters.size(); i++)
	{
		if (adapters[i].description.empty())
			continue;
		int index;
		index = FindConnectionInIfTable(adapters[i].description, pIfTable);
		if (index == -1)		//If the exact match is not found, the fuzzy match will be used to search again.
			index = FindConnectionInIfTableFuzzy(adapters[i].description, pIfTable);
		//if (index != -1)
		//{
		adapters[i].index = index;
		adapters[i].in_bytes = pIfTable->table[index].dwInOctets;
		adapters[i].out_bytes = pIfTable->table[index].dwOutOctets;
		adapters[i].description_2 = (const char*)pIfTable->table[index].bDescr;
		//}
	}
}

void CAdapterCommon::GetAllIfTableInfo(vector<NetWorkConection>& adapters, MIB_IFTABLE * pIfTable)
{
	vector<NetWorkConection> adapters_tmp;
	GetAdapterInfo(adapters_tmp);		//Get IP address
	adapters.clear();
	for (size_t i{}; i < pIfTable->dwNumEntries; i++)
	{
		NetWorkConection connection;
		connection.description = connection.description_2 = (const char*)pIfTable->table[i].bDescr;
		connection.index = i;
		connection.in_bytes = pIfTable->table[i].dwInOctets;
		connection.out_bytes = pIfTable->table[i].dwOutOctets;
		for (size_t j{}; j < adapters_tmp.size(); j++)
		{
			if (connection.description.find(adapters_tmp[j].description) != string::npos)
			{
				connection.ip_address = adapters_tmp[j].ip_address;
				connection.subnet_mask = adapters_tmp[j].subnet_mask;
				connection.default_gateway = adapters_tmp[j].default_gateway;
				break;
			}
		}
		adapters.push_back(connection);
	}
}

int CAdapterCommon::FindConnectionInIfTable(string connection, MIB_IFTABLE* pIfTable)
{
	for (size_t i{}; i < pIfTable->dwNumEntries; i++)
	{
		string descr = (const char*)pIfTable->table[i].bDescr;
		if (descr == connection)
			return i;
	}
	return -1;
}

int CAdapterCommon::FindConnectionInIfTableFuzzy(string connection, MIB_IFTABLE* pIfTable)
{
	for (size_t i{}; i < pIfTable->dwNumEntries; i++)
	{
		string descr = (const char*)pIfTable->table[i].bDescr;
		size_t index;
		//Find a shorter string within a longer string
		if (descr.size() >= connection.size())
			index = descr.find(connection);
		else
			index = connection.find(descr);
		if (index != wstring::npos)
			return i;
	}
	//If still not found, use string matching algorithm to find
	double max_degree{};
	int best_index{};
	for (size_t i{}; i < pIfTable->dwNumEntries; i++)
	{
		string descr = (const char*)pIfTable->table[i].bDescr;
		double degree = CCommon::StringSimilarDegree_LD(descr, connection);
		if (degree > max_degree)
		{
			max_degree = degree;
			best_index = i;
		}
	}
	return best_index;
}
