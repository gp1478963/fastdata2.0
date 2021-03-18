#include "CoriData.h"




void COriData::Clear()
{
	avgData = QVector<double>(nSize, 0);
	newData = QVector<double>(nSize, 0);
	sumData = QVector<double>(nSize, 0);
	vecData.clear();
}

void COriData::SetData(QVector<double>& list)
{

	if (nCache != 0 && vecData.size() >= nCache)
	{
		vecData.erase(vecData.begin());
	}
	newData = list;
	vecData.push_back(list);
	avgData.clear();
	for (int i = 0; i < 80; i++)
	{
		avgData.push_back(0);
	}
	avgData.reserve(list.size());
	double _data = 0;
	for (auto & h : vecData)
	{
		if (h.size() < list.size())
			return;
	}
	for (int i = 0; i < list.size(); i++)
	{
		for (int j = 0; j < vecData.size(); j++)
		{
			_data += vecData[j][i];
		}
		_data /= vecData.size();
		avgData[i] = _data;
		_data = 0;
	}
}

void COriData::GetData(QVector<double>& list)
{
	list.clear();
	for (unsigned i = 0; i < avgData.size(); i++)
	{
		list.push_back(avgData[i]);
	}
}

void COriData::SetData(QVector<double>& list, int _start, int _end)
{
	if (_start < 0) _start = 0;
	if (_end > list.size())_end = list.size();

	if (nCache != 0 && vecData.size() >= nCache)
	{
		//若存储深度大于指定，则清除多余
		for (unsigned i = 0; i < vecData[0].size(); i++)
		{
			sumData[i] -= vecData[0][i];
		}
		vecData.erase(vecData.begin());
	}

	QVector<double> _vecData = QVector<double>(list.size(), 0);
	for (unsigned i = _start; i < _end; i++) {
		_vecData[i] = list[i];
	}
	vecData.push_back(_vecData);

	for (unsigned i = _start; i < _end; i++)
	{
		sumData[i] += _vecData[i];
		avgData[i] = sumData[i] / vecData.size();
	}

	newData = _vecData;

}

void COriData::GetData(QVector<double>& list, int _start, int _end)
{
	if (_start < 0) _start = 0;
	if (_end > list.size())_end = list.size();

	for (unsigned i = _start; i < _end; i++)
	{
		list[i] = avgData[i];
	}
}

void COriData::RemoveLastData()
{
	if (!vecData.empty()) {
		for (unsigned i = 0; i < vecData.last().size(); i++)
		{
			sumData[i] -= vecData.last()[i];
		}
		vecData.remove(vecData.size() - 1);
	}

}

QVector<double>& COriData::GetLastData()
{
	if (!vecData.empty())
	{
		int _size = vecData.size();
		return vecData[vecData.size() - 1];
	}
	else
	{
		return QVector<double>();
	}

}

QVector<double> COriData::GetNullData()
{
	return QVector<double>(nSize, 0);
}

