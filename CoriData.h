#pragma once

#include <QMap>
#include <QVector>

class COriData
{
public:
	COriData() {};
	COriData(int _nNum, int _nCache = 10) :nSize(_nNum), nCache(_nCache), avgData(_nNum, 0), newData(_nNum, 0), sumData(_nNum, 0) {};
	COriData(const COriData &_data) :
		avgData(_data.avgData), newData(_data.newData), sumData(_data.sumData),
		vecData(_data.vecData), nCache(_data.nCache) {};
	~COriData() {};

public:
	QVector<double>avgData;					//平均
	QVector<double>newData;					//最新压入的数据
	QVector<double>sumData;					//

private:
	int nSize;								//数据个数，0环不参与计算，故 = 计算个数 + 1
	int nCache;								//数据存储深度
	QVector<QVector<double>>vecData;		//数据列表

public:
	COriData& operator=(const COriData &_data)
	{
		if (this == &_data) return *this;

		nSize = _data.nSize;
		nCache = _data.nCache;

		avgData = QVector<double>(_data.avgData);
		newData = QVector<double>(_data.newData);
		sumData = QVector<double>(_data.sumData);

		vecData = QVector<QVector<double>>(_data.vecData);

		return *this;
	}

public:
	void Clear();
	void SetData(QVector<double>& list);
	void GetData(QVector<double>& list);

	void SetData(QVector<double>& list, int _start, int _end);
	void GetData(QVector<double>& list, int _start, int _end);
	inline int GetRealDeepNumb() { return vecData.size(); }//返回原始数据存储实际深度
	void RemoveLastData();
	QVector<double>& GetLastData();
	QVector<double> GetNullData();

};


