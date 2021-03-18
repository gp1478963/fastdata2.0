#include "PortCtrl.h"
#include <iostream>
#include <QSerialPort>


PortCtrl::PortCtrl(int _commBaud, int _nDataNumInBoard, bool _isFlag, QString _PortData, QObject *parent) : QObject(parent)
{
	//串口参数初始
	this->mPortBaud = _commBaud; //串口波特率
	this->mPortName = _PortData; //端口名字

	port_Timer = new QTimer(this);
	port_Timer->setSingleShot(true);
	port_Timer->setInterval(200);
	connect(port_Timer, &QTimer::timeout, this, &PortCtrl::dataSecondsDeal);//第二次数据处理

	faEndTimer = new QTimer(this);
	faEndTimer->setInterval(500);
	connect(faEndTimer, &QTimer::timeout, this, &PortCtrl::slotEndPortCmd);

	end_Timer = new QTimer(this);
	end_Timer->setInterval(500);
	connect(end_Timer, &QTimer::timeout, this, &PortCtrl::slotEndPort);

	sendDataTimer = new QTimer(this);
	sendDataTimer->setInterval(100);
	connect(sendDataTimer, &QTimer::timeout,
		[=]()
	{
		sendData(mCmd);
	});
}

PortCtrl::~PortCtrl()
{
}

//初始化
PortCtrl& PortCtrl::initPort()
{
	//设置串口参数
	m_port = new QSerialPort;
	m_port->setPortName(this->mPortName);
	m_port->setBaudRate(this->mPortBaud);
	m_port->setDataBits(QSerialPort::Data8);
	m_port->setParity(QSerialPort::NoParity);
	m_port->setStopBits(QSerialPort::OneStop);
	m_port->setFlowControl(QSerialPort::NoFlowControl);
	QObject::connect(m_port, &QSerialPort::readyRead, this, &PortCtrl::fromReadyReadData);
	return *this;
}

PortCtrl& PortCtrl::openPort()
{
	bool res = true;
	if (!m_port->isOpen())
		res = m_port->open(QIODevice::ReadWrite);
	return *this;
}

bool PortCtrl::closePort()
{
	if (!m_port) return true;
	tempD.clear();
	readDate.clear();
	end_Timer->start();
	return false;
}

void PortCtrl::slotEndPortCmd()
{
	faEndTimer->stop();
	m_port->clear();
}

void PortCtrl::slotEndPort()
{
	end_Timer->stop();
	m_port->clear();
	if (m_port->isOpen())
		m_port->close();
}
//接口

PortCtrl & PortCtrl::sendData(QString & cmd)
{
	QByteArray AA;
	AA.append(cmd.toInt(nullptr, 16));
	m_port->write(AA);
	return *this;
}

void PortCtrl::startSendCmd(QPair<QString, QMap<QString, QString>>& _mTempCmd)
{
	this->mTempCmd = _mTempCmd;
	mCmd = mTempCmd.first;
	mNums = mTempCmd.second["num"].toInt(nullptr, 10);
	mStart = mTempCmd.second["datastartflag"].toInt(nullptr, 16);
	mEnd = mTempCmd.second["dataendflag"].toInt(nullptr, 16);
	mIsOnce = mTempCmd.second["isonce"].toInt(nullptr, 10);
	mIsOrder = mTempCmd.second["ishaveorder"].toInt(nullptr, 10);
	mRecvlen = mNums * 2 + mIsOrder + 2; //一条数据长度
	sendData(mCmd);
	if (mIsOnce == false)
		sendDataTimer->start();
}

void PortCtrl::stopSendCmd()
{
	if(sendDataTimer->isActive())
		sendDataTimer->stop();
	sendData((mTempCmd.second["endcmd"]));
}

void PortCtrl::clearCache()
{
	m_port->clear();
	readDate.clear();
}

//数据处理
void PortCtrl::dealFlagStartNotEqualEnd(QVector<int>&_signPos)
{
	int lastEpos = readDate.lastIndexOf(mEnd);//结尾标志位
	QVector<int> realData;
	for (auto it = _signPos.cbegin(); it != _signPos.cend(); ++it)
	{
		if (lastEpos - *it == mRecvlen - 1)
			realData.push_back(*it);
	}
	_signPos.swap(realData);
}

void PortCtrl::dealFlagStartEqualEnd(QVector<int>&_signPos)
{
	QVector<int> realdata;
	for (auto it = _signPos.begin() + 1; it != _signPos.end(); ++it)
	{
		if (*it - *(it - 1) == mRecvlen - 1)
			realdata.push_back(*(it - 1));
	}
	_signPos.swap(realdata);
}

void PortCtrl::searchForFrontFLag(QVector<int>&_signPos)
{
	int pos = -1, lastPos = 0;
	do
	{
		pos = readDate.indexOf(mStart, lastPos);//查找所有开始标志位位置
		lastPos = pos + 1;
		_signPos.append(pos);
	} while (pos != -1);
}

void PortCtrl::removeFlag(QVector<int>&_signPos)
{
	if (_signPos.size() > 0)
	{
		QByteArray temp(readDate.mid(_signPos.last(), mRecvlen));
		temp.remove(mRecvlen - 1, 1).remove(0, 1);
		if (mIsOrder == 1)
			temp.remove(0, 1);
		orderData(temp);//截取特定长度的字符
	}
}


void PortCtrl::dataSecondsDeal()
{
	int recvlen = readDate.size();
	if (recvlen < mRecvlen)
		return;
	QVector<int> _sigpos;
	searchForFrontFLag(_sigpos);
	if (mStart == mEnd)
		dealFlagStartEqualEnd(_sigpos);
	if (mStart != mEnd)
		dealFlagStartNotEqualEnd(_sigpos);
	removeFlag(_sigpos);
}

void PortCtrl::fromReadyReadData()
{
	m_port->blockSignals(true);

	if (!port_Timer->isActive())
		port_Timer->start();
	readDate.append(m_port->readAll());
	m_port->blockSignals(false);
}

void PortCtrl::orderData(QByteArray &Data)
{
	for (int i = 0; i < Data.size();)
	{
		tempData[0] = Data[i];
		tempData[1] = Data[i + 1];
		channelsData.push_back(tempData.toHex().toInt(nullptr, 16));
		i++;i++;
	}
	double a = channelsData[3];
	emit OnCtrlEvent(channelsData);
	channelsData.clear();
}

