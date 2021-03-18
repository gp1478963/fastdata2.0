#pragma once
#include <QVector>
#include <QSerialPort>
#include <QTimer>
#include <QMutex>
#include <QDebug>
#include <QMap>
#include <map>
#include <iostream>
#include <map>
#include <utility>
#include <QPair>

class PortCtrl:public QObject
{
	Q_OBJECT

public:
	PortCtrl() = default;
	PortCtrl(int _commBaud, int _nDataNumInBoard, bool _isFlag, QString _PortData, QObject *parent = nullptr);
	~PortCtrl();

	PortCtrl& initPort();
	PortCtrl& sendData(QString& cmd);
	PortCtrl& openPort();
	bool closePort();
	void slotEndPortCmd();
	void slotEndPort();
	void startSendCmd(QPair<QString, QMap<QString, QString>>& _mTempCmd);
	void stopSendCmd();
	void clearCache();

private:
	void fromReadyReadData();
	void dataSecondsDeal();
	void dealFlagStartNotEqualEnd(QVector<int>&_signPos);
	void dealFlagStartEqualEnd(QVector<int>&_signPos);
	void searchForFrontFLag(QVector<int>&_signPos);
	void removeFlag(QVector<int>&_signPos);
	void orderData(QByteArray &Data);

signals:
	void OnCtrlEvent(QVector<double>& data);

private:
	//串口参数
	QString mPortName;//本对象串口名字
	int mPortBaud;    //波特率参数
	int m_nDataNumInBoard;	//每个电路板 探头通道个数
	QSerialPort* m_port = nullptr;//串口
	//协议相关
	QString mCmd;
	int mNums;
	int mStart;
	int mEnd;
	int mIsOnce;
	int mIsOrder;
	int mRecvlen; //一条数据长度
	QPair<QString, QMap<QString, QString>> mTempCmd;
	//定时器
	QTimer* tonTimer; // 
	QTimer* port_Timer;// 
	QTimer* end_Timer;
	QTimer* faEndTimer;
	QTimer* sendDataTimer;
	//数据处理相关
	QByteArray readDate;//读取数据流
	QByteArray tempData;
	QByteArray tempD;//
	QVector<double>channelsData;//通道数据
	QVector<QMap<QString, QVariant>> allCmds;//能够存储所有的指令以及信息
	QMap<QString, QVariant> theTempCmds;
};

