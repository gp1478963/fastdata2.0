#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_fastdat.h"
#include <QSerialPort>
#include <iostream>
#include <QVector>
#include <QStringList>
#include <QPair>
#include <map>
#include <utility>

#include "qcustomplot.h"
#include "PortCtrl.h"
#include "CoriData.h"
#include <set>
using namespace std;
enum DoWhat
{
	DoNothing, DoBGData, DoENData,
};
class newFastData : public QMainWindow
{
	Q_OBJECT
private:
	Ui::fastdatClass ui;

public:
	newFastData(QWidget *parent = Q_NULLPTR);
	~newFastData();

private:
	void searchUserfulPort();//寻找可用串口
	void refreshTable();
	void initUi();
	void loadConfig();
	void writeUiToConfig();

	void slotStableDisplay();
	void slotUpdataTon();
	void tableClear();//清除表格所有内容
	void plotClear(); //清除柱形图所有内容

	double CalcBK(QVector<double>& CalcData);
	void setBKData(double _BK);
	bool slotRcevData(QVector<double> Data);
	void calculateRange();

public slots:
	void on_pBtBGtest_clicked();
	void on_pBtENtest_clicked();
	void slotUpdataTempCmd();
	void on_pBtSendCmd_clicked();
	void on_pBtReflash_clicked();

	void on_pBtOpenPort_clicked();
	void on_pBtClosePort_clicked();

	void on_pBtSaveBG_clicked();
	void on_pBtSaveEN_clicked();
	void on_pBtDelete_clicked();
	void on_pBtSavaConfigure_clicked();
	void on_checkBoxIsOrder_stateChanged(int state);
	void on_cmBoxChoseBaud_currentIndexChanged(int index);
	void on_cmBoxAllCmd_currentTextChanged(const QString &text);
private:
	//串口参数
	int mBaud = 9600;//波特率
	int mParity;//校验位
	int mDataBits = 1;//数据位
	int mStopBits = 0;//停止位
	int storeNum = 80;//环数
	QString mPortName = nullptr;
	PortCtrl *mPortCtrl = nullptr;
	//协议
	QString mCmdStart;
	QString mCmdEnd;
	QString mCmdSend;
	bool isFlag;
	bool isOrder;
	int mFlagSignalOrDouble;//标志位：单发指令还是双发指令
	int choce;
	bool loopSendData = true;//循环发送数据
	DoWhat doWhat;
	QStringList cmds;
	QVector<QString>ecmd;
	bool isPause = false;
	QMap<QString, std::map<QString, QString>> mAllInstructions;//能够存储所有的指令以及信息


	QMap<QString, QMap<QString, QString>> mAllInstr;//能够存储所有的指令以及信息
	QString mTheTempCmdStr;
	QMap<QString, QString> mapAllInstr;
	QPair<QString, QMap<QString, QString>> mTempCmd;

	//数据处理相关
	QVector<QVector<double>>vecData;
	QVector<QVector<double>>vecENData;
	QVector<QVector<double>>vecBGData;
	QVector<double>mtempData;
	QVector<int> Order;
	QVector<double> keys;
	COriData *BGData;
	//控件
	QTableView *tableView;
	QScrollArea *scrollArea;
	QWidget *scrollAreaWidgetContents;
	QMap<QString, QCustomPlot *>plotList;//图表
	QMap<QString, QCPBars *>barsList;//柱状图
	QStringList headerList;
	QStandardItemModel  * mModel;//数据模型

	QTimer* tonTimer;
	QTimer* SDisplay;
};
