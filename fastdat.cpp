#include "fastDat.h"
#include <QSerialPortInfo>
#include <QFileInfo>
#include <QStringLiteral>
const int tableDefaultValue = 0;

newFastData::newFastData(QWidget *parent) : QMainWindow(parent)
{
	ui.setupUi(this);
	this->setFixedSize(1400, 900);
	choce = 0;
	mPortCtrl = nullptr;
	searchUserfulPort(); //寻找可用的串口
	BGData = new COriData(80);

	tonTimer = new QTimer(this);
	tonTimer->setInterval(30);
	connect(tonTimer, &QTimer::timeout, this, &newFastData::slotUpdataTon);//能谱刷新

	SDisplay = new QTimer(this);
	SDisplay->setInterval(50);
	connect(SDisplay, &QTimer::timeout, this, &newFastData::slotStableDisplay);
	loadConfig();

	initUi();
	connect(ui.lineEditChannelNum, &QLineEdit::textChanged, this, &newFastData::slotUpdataTempCmd);
	connect(ui.lineEditStartFlag, &QLineEdit::textChanged, this, &newFastData::slotUpdataTempCmd);
	connect(ui.lineEditEndFlag, &QLineEdit::textChanged, this, &newFastData::slotUpdataTempCmd);
	connect(ui.checkBoxFormat, &QCheckBox::stateChanged, this, &newFastData::slotUpdataTempCmd);
	connect(ui.checkBoxSignalData, &QCheckBox::stateChanged, this, &newFastData::slotUpdataTempCmd);
	connect(ui.lineEEndCmd, &QLineEdit::textChanged, this, &newFastData::slotUpdataTempCmd);
	//指令属性发生变化
	
}

newFastData::~newFastData()
{
	delete BGData;
}

//初始化
void newFastData::searchUserfulPort()
{
	for each(const QSerialPortInfo &info in QSerialPortInfo::availablePorts())
	{
		QSerialPort serial;
		serial.setPort(info);
		if (serial.open(QIODevice::ReadWrite))
		{
			ui.cmBoxChoseCom->addItem(info.portName());
			serial.close();
		}
	}
}

void newFastData::initUi()
{
	for (auto a : mAllInstr.keys())
		ui.cmBoxAllCmd->addItem(a);
	if (!mAllInstr.isEmpty())
	{
		ui.cmBoxAllCmd->setEditText(mTempCmd.first);
		ui.lineEditChannelNum->setText(mTempCmd.second["num"]);
		ui.lineEditStartFlag->setText(mTempCmd.second["datastartflag"]);
		ui.lineEditEndFlag->setText(mTempCmd.second["dataendflag"]);
		ui.lineEEndCmd->setText(mTempCmd.second["endcmd"]);
		mTempCmd.second["isonce"] == "1" ? ui.checkBoxSignalData->setChecked(true) : ui.checkBoxSignalData->setChecked(false);
		mTempCmd.second["ishaveorder"] =="1"? ui.checkBoxFormat->setChecked(true) : ui.checkBoxFormat->setChecked(false);
	}
	ui.pBtClosePort->setEnabled(false);
	ui.pBtSendCmd->setEnabled(false);
	//tableWidgetData设置
	for (int i = 0; i < 16; i++)
		ui.tableWidgetData->setColumnWidth(i, 84);
	for (int i = 0; i < 11; i++)
		ui.tableWidgetData->setRowHeight(i, 22);
	headerList.append("Time");
	headerList.append("Value");
	//在布局中添加tableview
	scrollArea = new QScrollArea(this);//滚动条
	scrollArea->setObjectName(QStringLiteral("scrollArea"));
	scrollArea->setWidgetResizable(false);
	scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	//在滚动条中添加界面
	scrollAreaWidgetContents = new QWidget();
	scrollAreaWidgetContents->setObjectName(QStringLiteral("scrollAreaWidgetContents"));
	scrollAreaWidgetContents->setGeometry(QRect(0, 0, 640, 439));
	scrollArea->setWidget(scrollAreaWidgetContents);
	//布局添加滚动条
	ui.horizontalLayout->addWidget(scrollArea);
	ui.horizontalLayout->setStretch(0, 2);
	ui.horizontalLayout->setStretch(1, 5);

	mModel = new QStandardItemModel;//tableview 列表头
	mModel->setHorizontalHeaderLabels(headerList);


	QWidget* _Widget = scrollArea;
	QGridLayout *m_gLayout;
	if (!_Widget->layout())
	{
		m_gLayout = new QGridLayout();
		m_gLayout->setSpacing(1);
		_Widget->setLayout(m_gLayout);
	}

	{
		//能谱显示
		QString var = "Value";
		QCustomPlot * mark = new QCustomPlot(this);
		QCPBars *bgBars = nullptr;

		mark->axisRect()->setupFullAxesBox();
		mark->yAxis->setRange(0, 4200);//纵坐标范围
		mark->xAxis->setRange(0, ui.lineEditChannelNum->text().toInt());//横坐标范围
		mark->xAxis->setSubTicks(false);
		mark->xAxis->ticker()->setTickCount(50);
		bgBars = new QCPBars(mark->axisRect()->axis(QCPAxis::atBottom), mark->axisRect()->axis(QCPAxis::atLeft));
		mark->installEventFilter(this);

		plotList[var] = mark;//坐标构成
		barsList[var] = bgBars;//柱形图
	}

	for each(auto it in headerList)
	{
		if (it == "Time")
			continue;
		m_gLayout->addWidget(plotList[it]);
		//curShowPlot.push_back(it);//显示的图表
	}

}

void newFastData::loadConfig()
{
	QString Path = QCoreApplication::applicationDirPath();
	QString allPath = Path + "/FastData.ini";
	QFileInfo in(allPath);
	if (!in.exists())
	{
		//默认文件不存在，则创建
		on_pBtSavaConfigure_clicked();
	}
	QSettings *settings = new QSettings(allPath, QSettings::IniFormat);
	//判断文件是否存在，存在则直接打开，不存在则创建
	settings->setIniCodec(QTextCodec::codecForName("GB2312")); //在此添加设置，即可读写ini文件中的中文
	settings->beginGroup("Port");
	mPortName = settings->value("PortData").toString();//串口号
	mBaud = settings->value("commBaud").toInt();		//波特率
	isFlag = settings->value("isFlag").toInt();      //标志位
	isOrder = settings->value("isOrder").toInt();    //是否排序
	settings->endGroup();

	//读取全部指令
	settings->beginGroup("CMDLIST");
	QStringList tempcmdkey = settings->allKeys();

	QStringList tempcmdvalue;
	for (int i = 0; i < tempcmdkey.size(); i++)
	{
		tempcmdvalue << settings->value(tempcmdkey[i]).toString();
	}
	settings->endGroup();

	//读取指令信息
	for each (auto var in tempcmdvalue)
	{
		settings->beginGroup(var);
		QStringList _cmdkey = settings->allKeys();

		QMap<QString, QString> tt;
		for each (auto _key in _cmdkey)
			tt.insert(_key, settings->value(_key).toString());

		settings->endGroup();
		this->mAllInstr.insert(var, tt);
	}

	//读取排序信息
	settings->beginGroup("Order");
	QStringList keyList = settings->allKeys();
	for (int i = 1; i <= keyList.size(); i++)
		Order.push_back(settings->value(QString("v%1").arg(i)).toInt());
	settings->endGroup();
	delete settings;

	if (!mAllInstr.isEmpty())
	{
		auto it = mAllInstr.begin();
		mTempCmd.first = mAllInstr.firstKey();
		mTempCmd.second = *it;
	}
}

//数据显示处理
void newFastData::calculateRange()
{
	if (mtempData.isEmpty()) return;

	int max = 0, avg = 0, tempcount = 0, sum = 0;
	for (auto &a : mtempData)
	{
		sum += a;
		max = a > max ? a : max;
	}
	avg = sum / mtempData.size();
	for (auto &a : mtempData)
	{
		if (a > avg)
			tempcount++;
	}
	//自适应纵坐标
	if (max < 15)
	{
		plotList["Value"]->yAxis->setRangeUpper(15);
	}
	if (max > 15 && max <= 25)
	{
		plotList["Value"]->yAxis->setRangeUpper(25);
	}
	if (max > 25 && max <= 50)
	{
		plotList["Value"]->yAxis->setRangeUpper(50);
	}
	else if (max > 50 && max <= 250)
	{
		plotList["Value"]->yAxis->setRangeUpper(300);
	}

	if (max > 250 && max <= 500)
	{
		//按照500显示
		plotList["Value"]->yAxis->setRangeUpper(800);
	}
	else if (max > 500 && max <= 1000)
	{
		//按照1000显示
		plotList["Value"]->yAxis->setRangeUpper(1200);
	}
	else if (max > 1000 && max <= 2000)
	{
		//按照2000显示
		plotList["Value"]->yAxis->setRangeUpper(2000);
	}
	else if (max > 2000)
	{
		//按照4k显示
		plotList["Value"]->yAxis->setRangeUpper(4500);
	}
}

void newFastData::tableClear()
{
	for (int i = 1; i <= 16; i += 2)
		for (int j = 1; j < 11; j++)
			ui.tableWidgetData->setItem(j, i, new QTableWidgetItem(0, 10));
}

void newFastData::plotClear()
{
	QVector<qreal>tempks; QVector<qreal>tempda(ui.lineEditChannelNum->text().toInt(nullptr, 10), 0);
	for (int i = 0; i < tempks.size(); i++)
		tempks.push_back(i);
	barsList["Value"]->setData(tempks, tempda);
	plotList["Value"]->replot();
}

void newFastData::slotStableDisplay()
{
	SDisplay->stop();
	connect(mPortCtrl, &PortCtrl::OnCtrlEvent, this, &newFastData::slotRcevData);
	connect(tonTimer, &QTimer::timeout, this, &newFastData::slotUpdataTon);//能谱刷新
}

void newFastData::refreshTable()
{
	keys.clear();
	for (int i = 0; i < vecData[0].size(); i++)
		keys.push_back(i);
	auto it = mtempData.cbegin();
	int numOfColumns = mtempData.size() / 10;
	for (int i = 1; i <= numOfColumns * 2; i += 2)
		for (int j = 1; j < 11; j++)
			ui.tableWidgetData->setItem(j, i, new QTableWidgetItem(QString::number((int)*it++, 10)));
}

void newFastData::slotUpdataTon()
{

	if (vecData.isEmpty()) return;
	{
		switch (choce)
		{
		case 0:
			mtempData = vecData[0];
			break;
		case 1:
			BGData->SetData(vecData[0]);
			BGData->GetData(mtempData);
			vecBGData.push_back(mtempData);
			break;
		case 2:
			BGData->GetData(mtempData);
			if (mtempData.size() != vecData[0].size())
				mtempData.resize(vecData[0].size());
			for (int i = 0; i < mtempData.size(); i++)
			{
				mtempData[i] = vecData[0][i] - mtempData[i];
			}
			vecENData.push_back(mtempData);
			break;
		default:
			break;
		}
		calculateRange();
		refreshTable();

		plotList["Value"]->xAxis->setRangeUpper(vecData[0].size());
		barsList["Value"]->setData(keys, mtempData);
		plotList["Value"]->replot();
		vecData.removeFirst();
		if (vecData.size() > 10) vecData.clear();
	}
	vecData.clear();
	return;
}

//指令处理
void newFastData::writeUiToConfig()
{
	if (!ui.cmBoxAllCmd->currentText().isEmpty())
	{
		if (!ui.lineEditChannelNum->text().isEmpty())
		{
			if (!ui.lineEditStartFlag->text().isEmpty())
			{
				if (!ui.lineEditEndFlag->text().isEmpty())
				{
					if (mAllInstr.contains(ui.cmBoxAllCmd->currentText()))
					{
						mAllInstr[ui.cmBoxAllCmd->currentText()] = mTempCmd.second;
					}
					else
					{
						mAllInstr.insert(mTempCmd.first, mTempCmd.second);
						ui.cmBoxAllCmd->addItem(mTempCmd.first);
					}
				}
			}
		}
	}
}

void newFastData::slotUpdataTempCmd()
{
	if (!ui.cmBoxAllCmd->currentText().isEmpty())
		mTempCmd.first = ui.cmBoxAllCmd->currentText().toUpper();
	if (!ui.lineEditChannelNum->text().isEmpty())
		mTempCmd.second["num"] = ui.lineEditChannelNum->text();
	if (!ui.lineEditStartFlag->text().isEmpty())
		mTempCmd.second["datastartflag"] = ui.lineEditStartFlag->text();
	if (!ui.lineEditEndFlag->text().isEmpty())
		mTempCmd.second["dataendflag"] = ui.lineEditEndFlag->text();

	mTempCmd.second["endcmd"] = ui.lineEEndCmd->text().toUpper();

	mTempCmd.second["ishaveorder"] = (ui.checkBoxFormat->isChecked() ? "1" : "0");
	mTempCmd.second["isonce"] = (ui.checkBoxSignalData->isChecked() ? "1" : "0");
}

//数据处理
double newFastData::CalcBK(QVector<double>& CalcData)
{
	//无浓度探头
	double _sum = 0;
	for (int i = 0; i < CalcData.size(); i++) {
		_sum += CalcData[i];
	}
	return _sum / 115;
}

void newFastData::setBKData(double _BK)
{
	int pos = 0;
	//曲线
	int showLengths = 10; //定义图表显示最大长度 msec
	double key = QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;

	QString itKey = "BK";
	//for (auto it = _valList.begin(); it != _valList.end(); it++)
	{
		//if (itKey == "Time") continue;
		int nY = plotList[itKey]->graph(pos)->data().data()->size();  //截取固定长度的实时曲线
		if (nY > showLengths)
		{
			plotList[itKey]->graph(pos)->data()->removeBefore(key - showLengths);
		}

		plotList[itKey]->graph(pos)->addData(key, _BK);
		plotList[itKey]->xAxis->setRange(key, showLengths, Qt::AlignRight);
		//plotList[itKey]->yAxis->setRange(0, (int)(_BK * 5));
		//plotList[itKey]->yAxis2->setRange(0, (int)(_BK * 5));

		plotList[itKey]->replot();
	}
}

bool newFastData::slotRcevData(QVector<double> Data)
{
	if (isOrder == 1)//是否排序
	{
		if (Order.isEmpty() && Data.count() != Order.count())  return false;

		QVector<double>listData(storeNum + 1, 0);
		//对中环为第0环 补0, _storeNum = 计算探头数量 + 1
		for (auto it = Order.cbegin(); it != Order.cend(); ++it)
		{
			if (*it != -1 && *it < storeNum)
				listData[*it] = Data[*it];
		}
		listData.removeFirst();
		Data = listData;
	}
	vecData.append(Data);
	return true;
}

//ui槽
void newFastData::on_pBtReflash_clicked()
{
	ui.cmBoxChoseCom->clear();
	searchUserfulPort();
}

void newFastData::on_pBtOpenPort_clicked()
{
	ui.pBtClosePort->setEnabled(true);
	this->mPortName = ui.cmBoxChoseCom->currentText();
	mBaud = ui.cmBoxChoseBaud->currentText().toInt(nullptr, 10);
	if (mPortCtrl == nullptr)//初次使用端口
	{
		this->isOrder = ui.checkBoxIsOrder->isChecked();//是否排序
		this->mPortCtrl = new PortCtrl(mBaud, storeNum, isFlag, mPortName);
		//串口初始化
		this->mPortCtrl->initPort();
	}
	mPortCtrl->openPort();
	//开启刷新能谱定时器
	tonTimer->start();
	connect(mPortCtrl, &PortCtrl::OnCtrlEvent, this, &newFastData::slotRcevData);

	ui.cmBoxChoseCom->setDisabled(true);
	ui.cmBoxChoseBaud->setDisabled(true);
	ui.pBtSendCmd->setEnabled(true);
}

void newFastData::on_pBtClosePort_clicked()
{
	disconnect(mPortCtrl, &PortCtrl::OnCtrlEvent, this, &newFastData::slotRcevData);
	this->mPortCtrl->closePort();//关闭端口
	vecData.clear();
	ui.cmBoxChoseCom->setDisabled(false);
	ui.cmBoxChoseBaud->setDisabled(false);
	ui.checkBoxIsOrder->setDisabled(false);
	ui.pBtSendCmd->setEnabled(false);
}

void newFastData::on_pBtSendCmd_clicked()
{
	if (isPause == false)
	{
		writeUiToConfig();
		slotUpdataTempCmd();
		tableClear();
		plotClear();
		mPortCtrl->clearCache();
		disconnect(mPortCtrl, &PortCtrl::OnCtrlEvent, this, &newFastData::slotRcevData);
		disconnect(tonTimer, &QTimer::timeout, this, &newFastData::slotUpdataTon);//能谱刷新
		SDisplay->start();
		mPortCtrl->startSendCmd(mTempCmd);
		ui.pBtSendCmd->setText(QStringLiteral("暂停"));
		isPause = true;
	}
	else if (isPause == true)
	{
		ui.pBtSendCmd->setText(QStringLiteral("发送"));
		mPortCtrl->stopSendCmd();
		isPause = false;
	}
	ui.lineEEndCmd->setEnabled(!isPause);
	ui.cmBoxAllCmd->setEnabled(!isPause);
	ui.pBtDelete->setEnabled(!isPause);
	ui.checkBoxSignalData->setEnabled(!isPause);
	ui.checkBoxFormat->setEnabled(!isPause);
	ui.lineEditChannelNum->setEnabled(!isPause);
	ui.lineEditStartFlag->setEnabled(!isPause);
	ui.lineEditEndFlag->setEnabled(!isPause);
	ui.pBtOpenPort->setEnabled(!isPause);
	ui.pBtClosePort->setEnabled(!isPause);
	ui.checkBoxIsOrder->setEnabled(!isPause);
}


void newFastData::on_pBtSaveBG_clicked()
{
	QString fileName = QCoreApplication::applicationDirPath() + "/BG.txt";
	QFile file(fileName);
	QTextStream out(&file);
	file.open(QIODevice::Append | QIODevice::Text);

	QString sDateTimes = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
	out << sDateTimes << "\n";

	QVector<double>_tempData = QVector<double>(storeNum, 0);
	BGData->GetData(_tempData);
	for each(auto var in _tempData)
	{
		out << var << "\t";
	}
	out << "\n";

	file.close();
}

void newFastData::on_pBtSaveEN_clicked()
{
	QString fileName = QCoreApplication::applicationDirPath() + "/EN.txt";
	QFile file(fileName);
	QTextStream out(&file);
	file.open(QIODevice::Append | QIODevice::Text);

	QString sDateTimes = QDateTime::currentDateTime().toString("yyyy-MM-dd hh-mm-ss");
	out << sDateTimes << "\n";

	for each (auto _ENData in vecENData)
	{
		for each(auto var in _ENData)
		{
			out << var << "\t";
		}
		out << "\n";
	}
	file.close();
}

void newFastData::on_pBtDelete_clicked()
{
	if (mAllInstr.contains(ui.cmBoxAllCmd->currentText().toUpper()))
	{
		mAllInstr.remove(ui.cmBoxAllCmd->currentText().toUpper());
		mAllInstr.remove(ui.cmBoxAllCmd->currentText().toUpper());
	}
	ui.cmBoxAllCmd->clear();
	ui.lineEEndCmd->clear();
	ui.cmBoxAllCmd->clearEditText();
	for each(auto car in mAllInstr.keys())
	{
		ui.cmBoxAllCmd->addItem(car);
	}
	if (!mAllInstr.isEmpty())
		ui.cmBoxAllCmd->setEditText(mAllInstr.firstKey());
	else
	{
		//ui全部清空
	}
}

void newFastData::on_pBtSavaConfigure_clicked()
{
	slotUpdataTempCmd();
	QString Path = QCoreApplication::applicationDirPath();
	QSettings *settings = new QSettings(Path + "/FastData.ini", QSettings::IniFormat);
	settings->setIniCodec(QTextCodec::codecForName("GB2312")); //在此添加设置，即可读写ini文件中的中文

	settings->clear();
	settings->beginGroup("Port");
	QString bauds = QString::number(mBaud, 10);
	QString isflag = QString::number(isFlag == true ? 1 : 0, 10);
	QString Order = isOrder == true ? "1" : "0";
	settings->setValue("PortData", mPortName);//串口号
	settings->setValue("commBaud", bauds);
	settings->setValue("isFlag", isflag);      //标志位
	settings->setValue("isOrder", Order);    //是否排序
	settings->endGroup();

	settings->beginGroup("CMDLIST");
	int i = 0;
	auto allkey = mAllInstr.keys();
	for (auto it = allkey.cbegin(); it != allkey.cend(); ++it)
	{
		settings->setValue(QString("cmd%1").arg(i + 1), *it);
		++i;
	}
	settings->endGroup();
	//将所有的指令写道ini文件
	auto it = mAllInstr.cbegin();
	for (auto &it = allkey.begin(); it != allkey.end(); ++it)
	{
		settings->beginGroup(*it);

		auto allk = mAllInstr[*it].keys();
		for (auto a : allk)
			settings->setValue(a, mAllInstr[*it][a]);
		settings->endGroup();

	}
	delete settings;
}

void newFastData::on_pBtBGtest_clicked()
{
	choce = 1;
}

void newFastData::on_pBtENtest_clicked()
{
	choce = 2;
}

void newFastData::on_cmBoxChoseBaud_currentIndexChanged(int index)
{
	this->mBaud = ui.cmBoxChoseBaud->currentText().toInt(nullptr, 10);
}

void newFastData::on_cmBoxAllCmd_currentTextChanged(const QString &text)
{
	//slotUpdataTempCmd();
	auto cmd = ui.cmBoxAllCmd->currentText().toUpper();
	if ( mAllInstr.contains(cmd))
	{
		ui.lineEEndCmd->setText(mAllInstr[cmd]["endcmd"]);
		ui.lineEditChannelNum->setText(mAllInstr[cmd]["num"]);
		mAllInstr[cmd]["isonce"].toInt() ? ui.checkBoxSignalData->setCheckState(Qt::Checked) : ui.checkBoxSignalData->setCheckState(Qt::Unchecked);
		mAllInstr[cmd]["ishaveorder"].toInt() ? ui.checkBoxFormat->setCheckState(Qt::Checked) : ui.checkBoxFormat->setCheckState(Qt::Unchecked);
		ui.lineEditStartFlag->setText(mAllInstr[cmd]["datastartflag"]);
		ui.lineEditEndFlag->setText(mAllInstr[cmd]["dataendflag"]);
	}
}

void newFastData::on_checkBoxIsOrder_stateChanged(int state)
{
	this->isOrder = this->ui.checkBoxIsOrder->isChecked();
}


