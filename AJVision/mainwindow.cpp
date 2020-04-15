﻿#include "mainwindow.h"
#include "ui_mainwindow.h"

bool x_step_finish = false;//X轴步进电机是否完成
bool y_step_finish = false;//Y轴步进电机是否完成
unsigned short coarse_x = 0;//x轴电机位置
unsigned short coarse_y = 0;//y轴电机位置
unsigned short coarse_z = 0;//z轴电机位置
bool bUseEnableShutdown = false;//使能停机标志
bool bDistanceDone = false;     //距离完成标志
bool bTimeUseup    = false;     //时间耗尽标志
bool bToAppiontPos = false;     //到达指定位置标志
bool bToNegativeLimit = false;  //到达负限位标志
bool bDirectionError  = false;  //方向错误标志
bool bToLimit = false;          //到达限位标志


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_bSaveImage = false;

     ui->listWidget->setViewMode(QListView::ListMode);   //设置显示模式为列表模式

    //udp初始化
    mSocket = new QUdpSocket(this);
    connect(mSocket,SIGNAL(readyRead()),this,SLOT(read_data()),Qt::DirectConnection);
    qDebug()<< mSocket->bind(QHostAddress::Any, 8888);

    //初始化定时器
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    timer->start(100);

    InitialCom();//初始化串口


//    ReadParameters();
//    cout<<cameraMatrix_L<<endl;
//    cout<<distCoeffs_L<<endl;
//    cout<<cameraMatrix_R<<endl;
//    cout<<distCoeffs_R<<endl;
//    cout<<R<<endl;
//    cout<<T<<endl;

//    //创建图像重投影映射表
//    stereoRectification(cameraMatrix_L, distCoeffs_L, cameraMatrix_R, distCoeffs_R,
//        imageSize, R, T, R1, R2, P1, P2, Q, mapl1, mapl2, mapr1, mapr2);
//    cout<<cameraMatrix_L<<endl;
//    cout<<distCoeffs_L<<endl;
//    cout<<cameraMatrix_R<<endl;
//    cout<<distCoeffs_R<<endl;
//    cout<<R<<endl;
//    cout<<T<<endl;

//    Mat_<float> invec = (Mat_<float>(3, 1) << 0.04345, -0.05236, -0.01810);
//    Mat  outMat;
//    Rodrigues(invec, outMat);
//    cout << outMat << endl;//打印矩阵

//    Mat outvec;
//    Rodrigues(outMat, outvec);
//    cout << outvec << endl;

}

MainWindow::~MainWindow()
{
    //关闭相机
    if(capture.isOpened())
    {
        capture.release();
    }

    //关闭定时器
    if(timer)
    {
       if(timer->isActive())
       {
           timer->stop();
       }

       delete timer;
    }

    //关闭udp
    if(mSocket)
        delete mSocket;

    //关闭串口
    if(serial)
    {
        if(serial->isOpen())
        {
            serial->close();//关闭串口
        }

        delete serial;
    }


    delete ui;
}

Vec3f  MainWindow::xy2XYZ(const char* imageName_L,const char* imageName_R,int x,int y)
{
    //视差图建立
    computeDisparityImage(imageName_L, imageName_R, img1_rectified, img2_rectified, mapl1, mapl2, mapr1, mapr2, validRoi, disparity);
    // 从三维投影获得深度映射
    reprojectImageTo3D(disparity, result3DImage, Q);

    Point point;
    point.x = x;
    point.y = y;
    Vec3f xyz = result3DImage.at<Vec3f>(point);

    return xyz;
}

void MainWindow::InitialCom()
{
    serial = new QSerialPort;
    serial->setPortName("COM1");      //设置串口名
    serial->open(QIODevice::ReadWrite);      //打开串口
    serial->setBaudRate(9600);      //设置波特率
    serial->setDataBits(QSerialPort::Data8);      //设置数据位数
    serial->setParity(QSerialPort::NoParity);        //设置奇偶校验
    serial->setStopBits(QSerialPort::OneStop);      //设置停止位
    serial->setFlowControl(QSerialPort::NoFlowControl);    //设置流控制
    connect(serial,SIGNAL(readyRead()),this,SLOT(Read_Data()));
}

void MainWindow::addToList(QString str)
{
    QListWidgetItem *item = new QListWidgetItem;
    item->setText(str);                     //设置列表项的文本
    ui->listWidget->addItem(item);          //加载列表项到列表框
}

void MainWindow::read_data()
{
    QByteArray array;
    QHostAddress address;
    quint16 port;
    array.resize(mSocket->bytesAvailable());//根据可读数据来设置空间大小
    mSocket->readDatagram(array.data(),array.size(),&address,&port); //读取数据
    ui->listWidget->addItem(array);//显示数据

}


void MainWindow::handleTimeout()
{
    if(!m_bShowImage)
        return;
    if(capture.isOpened())
    {
        capture >> frame;
//        cvtColor(frame, frame, CV_BGR2RGB);//only RGB of Qt
//        QImage srcQImage = QImage((uchar*)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888);

        Rect rectLeft(0, 0, frame.cols / 2, frame.rows);
        Rect rectRight(frame.cols / 2, 0, frame.cols / 2, frame.rows);
        Mat image_l = Mat(frame, rectLeft);
        Mat image_r = Mat(frame, rectRight);

        Mat distortion = image_l.clone();
        Mat camera_matrix = Mat(3, 3, CV_32FC1);
        Mat distortion_coefficients;

        //矫正
        cv::undistort(image_l, distortion, cameraMatrix_L, distCoeffs_L);


        imshow("left",image_l);


       imshow("left2",distortion);
       //imshow("right",image_r);



       if(m_bSaveImage)
       {
           static int cnt = 0;
           cnt++;

           m_bSaveImage = false;

           string path1 = std::to_string(cnt) + ".jpg";
           imwrite(path1, image_l);

           string path2 = std::to_string(cnt+25) + ".jpg";
           imwrite(path2, image_r);


       }

//        cvtColor(image_l, image_l, CV_BGR2RGB);//only RGB of Qt
//        QImage srcQImage = QImage((uchar*)(image_l.data), image_l.cols, image_l.rows, QImage::Format_RGB888);

//        ui->label->setPixmap(QPixmap::fromImage(srcQImage));
//        ui->label->resize(srcQImage.size());
//        ui->label->show();
    }
    else
    {
        capture.open(0);
        capture.set(CV_CAP_PROP_FRAME_WIDTH, 1280);
        capture.set(CV_CAP_PROP_FRAME_HEIGHT, 960);
    }

}

void MainWindow::Read_Data()//串口读取函数
{
    QByteArray buf;
    buf = serial->readAll();
    //QByteArray 转换为 char *
    char *rxData;//不要定义成ch[n];
    rxData = buf.data();
    int size = buf.size();

    if(rxData[0] == 0x5a && rxData[size-1] == jiaoyan((unsigned char*)rxData))
    {
        char id  = rxData[1];//应答帧ID
        char cmd = rxData[2];//应答帧命令字
        if(cmd == 0xc)//应答帧
        {
            switch (id) {
            case 0:
                 addToList("横轴步进电机应答");
                 break;
            case 1:
                 addToList("纵轴步进电机应答");
                 break;
            case 2:
                addToList("垂向步进电机应答");
                break;
            case 5:
                addToList("温度控制应答");
                break;
            case 6:
                addToList("动作控制应答");
                break;
            case 7:
                addToList("故障帧应答");
                break;
            case 8:
                addToList("手持仪应答");
                break;
            default:
                break;
            }
        }
        else if(cmd == 0xf){//完成帧
            //完成帧解析
        }

    }

//    int size = buf.size();//
//    //QByteArray<->char数组
//    char recv[100];//数组
//    int len_array = buf.size();
//    int len_buf = sizeof(buf);
//    int len = qMin( len_array, len_buf );
//    // 转化
//    memcpy( recv, buf,  len );
}

//顺时针旋转90度
Mat MainWindow::imageRotate90(Mat src)
{
    Mat temp,dst;
    transpose(src, temp);
    flip(temp, dst, 1); //旋转90度

    return dst;
}

//逆时针旋转90度
Mat MainWindow::imageRatateNegative90(Mat src)
{
    Mat temp, dst;
    transpose(src, temp);
    flip(temp, dst, 0); //旋转-90度

    return dst;
}

//读取相机标定参数，包括内参、外参
void MainWindow::ReadParameters()
{
    cv::FileStorage fs("calibParams.yml", cv::FileStorage::READ);
    if (fs.isOpened())
    {
        fs["cameraMatrix_L"] >> cameraMatrix_L;
        fs["distCoeffs_L"] >> distCoeffs_L;
        fs["cameraMatrix_R"] >> cameraMatrix_R;
        fs["distCoeffs_R"] >> distCoeffs_R;
        fs["R"] >> R;
        fs["T"] >> T;
        fs["imageSize"] >> imageSize;
        fs.release();
    }
}

/*
立体校正
参数：
    cameraMatrix			相机内参数
    distCoeffs				相机畸变系数
    imageSize				图像尺寸
    R						左右相机相对的旋转矩阵
    T						左右相机相对的平移向量
    R1, R2					行对齐旋转校正
    P1, P2					左右投影矩阵
    Q						重投影矩阵
    map1, map2				重投影映射表
*/
void MainWindow::stereoRectification(Mat& cameraMatrix1, Mat& distCoeffs1, Mat& cameraMatrix2, Mat& distCoeffs2,
    Size& imageSize, Mat& R, Mat& T, Mat& R1, Mat& R2, Mat& P1, Mat& P2, Mat& Q, Mat& mapl1, Mat& mapl2, Mat& mapr1, Mat& mapr2)
{
    Rect validRoi[2];

    stereoRectify(cameraMatrix1, distCoeffs1, cameraMatrix2, distCoeffs2, imageSize,
        R, T, R1, R2, P1, P2, Q, 0, -1, imageSize, &validRoi[0], &validRoi[1]);

    initUndistortRectifyMap(cameraMatrix1, distCoeffs1, R1, P1, imageSize, CV_32FC1, mapl1, mapl2);
    initUndistortRectifyMap(cameraMatrix2, distCoeffs2, R2, P2, imageSize, CV_32FC1, mapr1, mapr2);
  //  return validRoi[0], validRoi[1];
}

/*
计算视差图
参数：
    imageName1	左相机拍摄的图像
    imageName2	右相机拍摄的图像
    img1_rectified	重映射后的左侧相机图像
    img2_rectified	重映射后的右侧相机图像
    map	重投影映射表
*/
bool MainWindow::computeDisparityImage(const char* imageName1, const char* imageName2, Mat& img1_rectified,
    Mat& img2_rectified, Mat& mapl1, Mat& mapl2, Mat& mapr1, Mat& mapr2, Rect validRoi[2], Mat& disparity)
{
    // 首先，对左右相机的两张图片进行重构
    Mat img1 = imread(imageName1);
    Mat img2 = imread(imageName2);
    if (img1.empty() | img2.empty())
    {
        qDebug() << "图像为空" << endl;
    }
    Mat gray_img1, gray_img2;
    cvtColor(img1, gray_img1, COLOR_BGR2GRAY);
    cvtColor(img2, gray_img2, COLOR_BGR2GRAY);
    Mat canvas(imageSize.height, imageSize.width * 2, CV_8UC1); // 注意数据类型
    Mat canLeft = canvas(Rect(0, 0, imageSize.width, imageSize.height));
    Mat canRight = canvas(Rect(imageSize.width, 0, imageSize.width, imageSize.height));
    gray_img1.copyTo(canLeft);
    gray_img2.copyTo(canRight);
    imwrite("校正前左右相机图像.jpg", canvas);
    remap(gray_img1, img1_rectified, mapl1, mapl2, INTER_LINEAR);
    remap(gray_img2, img2_rectified, mapr1, mapr2, INTER_LINEAR);
    imwrite("左相机校正图像.jpg", img1_rectified);
    imwrite("右相机校正图像.jpg", img2_rectified);
    img1_rectified.copyTo(canLeft);
    img2_rectified.copyTo(canRight);
    rectangle(canLeft, validRoi[0], Scalar(255, 255, 255), 5, 8);
    rectangle(canRight, validRoi[1], Scalar(255, 255, 255), 5, 8);
    for (int j = 0; j <= canvas.rows; j += 16)
        line(canvas, Point(0, j), Point(canvas.cols, j), Scalar(0, 255, 0), 1, 8);
    imwrite("校正后左右相机图像.jpg", canvas);
    // 进行立体匹配
    Ptr<StereoBM> bm = StereoBM::create(16, 9); // Ptr<>是一个智能指针
    bm->compute(img1_rectified, img2_rectified, disparity); // 计算视差图
    disparity.convertTo(disparity, CV_32F, 1.0 / 16);
    // 归一化视差映射
    normalize(disparity, disparity, 0, 256, NORM_MINMAX, -1);
    return true;
}

void MainWindow::on_pushButton_clicked()
{
   // Mat dst = ImageStittchBySurf(imread("d:\\image02.jpg"),imread("d:\\image01.jpg"));
   //  Mat dst = StitchImageByOrb(imread("d:\\image02.jpg"),imread("d:\\image01.jpg"));
  //   Mat dst = StitchImageByOrb(imread("d:\\1.png"),imread("d:\\2.png"));
     Mat dst = StitchImageBySift(imread("d:\\1.png"),imread("d:\\2.png"));
//    medianBlur(dst, dst, 3);//第三个参数表示孔径的线性尺寸，它的值必须是大于1的奇数

   // Stitch(imread("d:\\1.jpg"),imread("d:\\2.jpg"));
  //  imshow("dst",dst);
  //  imshow("dst",StitchImageByFast(imread("d:\\1.png"),imread("d:\\2.png")));

}

void MainWindow::SendPictureByUdp(QString path,QString ip,qint16 port)
{
    struct ImageFrameHead {
        unsigned int funCode;               //功能码，用于区分数据包中的内容
        unsigned int uTransFrameHdrSize;    //sizeof(ImageFrameHead)，数据包头的大小
        unsigned int uTransFrameSize;       //Data Size，数据包中数据的大小
        //数据帧变量
        unsigned int uDataFrameSize;        //图片数据总大小
        unsigned int uDataFrameTotal;       //图片数据被分数据帧个数
        unsigned int uDataFrameCurr;        //数据帧当前的帧号
        unsigned int uDataInFrameOffset;    //数据帧在整帧的偏移
    };

    while(true) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly))
            return;
        char *m_sendBuf = new char[1024];

        int size = file.size();
        int num = 0;
        unsigned int count = 0;
        int endSize = size%996;//996=1024-sizeof(ImageFrameHead)
        if (endSize == 0) {
            num = size/996;
        }
        else {
            num = size/996+1;
        }


        while (count < num) {
            memset(m_sendBuf, 0, 1024);

            ImageFrameHead mes;
            mes.funCode = 24;
            mes.uTransFrameHdrSize = sizeof(ImageFrameHead);
            if ((count+1) != num) {
                mes.uTransFrameSize = 996;
            }
            else {
                mes.uTransFrameSize = endSize;
            }
            //qDebug()<<size;
            mes.uDataFrameSize = size;
            mes.uDataFrameTotal = num;
            mes.uDataFrameCurr = count+1;
            mes.uDataInFrameOffset = count*(1024 - sizeof(ImageFrameHead));

            file.read(m_sendBuf+sizeof(ImageFrameHead), 1024-sizeof(ImageFrameHead));

            memcpy(m_sendBuf, (char *)&mes, sizeof(ImageFrameHead));
            mSocket->writeDatagram(m_sendBuf, mes.uTransFrameSize+mes.uTransFrameHdrSize, QHostAddress(ip)/*QHostAddress("192.168.56.1")*/, port);
            QTime dieTime = QTime::currentTime().addMSecs(1);
            while( QTime::currentTime() < dieTime )
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            count++;
        }
        file.close();
        QTime dieTime = QTime::currentTime().addMSecs(10);
        while( QTime::currentTime() < dieTime )
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

void MainWindow::on_deleteAll_clicked()
{
    int num = ui->listWidget->count();  //获取列表项的总数目
     ui->listWidget->setFocus(); //将光标设置到列表框上，若注释该语句，则删除时，要手动将焦点设置到列表框，即点击列表项
     for(int i=0;i<num;i++)
     {   //逐个获取列表项的指针，并删除
         QListWidgetItem *item = ui->listWidget->takeItem(ui->listWidget->currentRow());
         delete item;
     }
}

//void MainWindow::on_deleteSingleItem_clicked()
//{
//    //获取列表项的指针
//    QListWidgetItem *item = ui->listWidget->takeItem(ui->listWidget->currentRow());
//    delete item;        //释放指针所指向的列表项
//}

//串口数据检查求和
unsigned char MainWindow::SerialCheckSum(unsigned char *buf, unsigned char len)
{
    unsigned char i, ret = 0;

    for(i=0; i<len; i++)
    {
        ret += buf[i];
    }
    return ret;
}





//处理串口缓冲区数据
void MainWindow::processSerialBuffer(const char* data)
{
    if (SerialCheckSum((unsigned char*)data, 10) != (unsigned char)data[10])
    {
        cout << "CRC校验和不符!" << endl;
        return;
    }

    unsigned char device_id, command;
    device_id = data[1];//设备id
    command   = data[2];//命令id

    //完成帧
    if (command == 0x0f)//完成帧
    {
        if (device_id == 0x00)//横轴完成
        {
            addToList("X轴位移完成");
          //  cout << "X轴位移完成" << endl;
            coarse_x = 0;
            unsigned short temp;
            temp = (unsigned char)data[5];
            coarse_x |= temp;
            temp = (unsigned char)data[6];
            coarse_x |= temp << 8;
            //printf("%x %x\n", udata[5], udata[6]);
            QString strPos("x轴当前位置:");
            strPos.append(QString::number(coarse_x));
            addToList(strPos);
            //cout << "当前x=" << coarse_x << endl;
            x_step_finish = true;
        }
       else if (device_id == 0x01)
        {
            addToList("Y轴位移完成");
            coarse_y = 0;
            unsigned short temp;
            temp = (unsigned char)data[5];
            coarse_y |= temp;
            temp = (unsigned char)data[6];
            coarse_y |= temp << 8;
            //printf("%x %x\n", udata[5], udata[6]);
            //cout << "当前y=" << coarse_y << endl;
            QString strPos("y轴当前位置:");
            strPos.append(QString::number(coarse_y));
            addToList(strPos);

            y_step_finish = true;
        }
        else if(device_id == 0x02)//直流电机完成帧
        {
            addToList("Z轴位移完成");

            coarse_z = 0;
            unsigned short temp;
            temp = (unsigned char)data[5];
            coarse_z |= temp;
            temp = (unsigned char)data[6];
            coarse_z |= temp << 8;
            //printf("%x %x\n", udata[5], udata[6]);
            //cout << "当前y=" << coarse_y << endl;
            QString strPos("z轴当前位置:");
            strPos.append(QString::number(coarse_z));
            addToList(strPos);
        }

        unsigned char status = (unsigned char)data[4];
        if(status&0x1 == 1) //bit0 使能停机标志
        {
            addToList("不是使用使能位停机");
            bUseEnableShutdown = true;
        }
        else
        {
            addToList("不是使用使能位停机");
            bUseEnableShutdown = false;
        }

        if(status&0x2 == 1) //bit1 距离完成标志
        {
            addToList("不是使用使能位停机");
            bDistanceDone = true;
        }
        else
        {
            addToList("不是使用使能位停机");
            bDistanceDone = false;
        }

        if(status&0x4 == 1)//时间耗尽标志
        {
            addToList("时间耗尽");
            bTimeUseup = true;
        }
        else
        {
            addToList("时间未耗尽");
            bTimeUseup = false;
        }

        if(status&0x8 == 1)//到达指定位置标志
        {
            addToList("到达指定位置");
            bToAppiontPos = true;
        }
        else
        {
            addToList("未到达指定位置");
            bToAppiontPos = false;
        }

        if(status&0x10 == 1)//到达负限位标志
        {
            addToList("到达负限位");
            bToNegativeLimit = true;
        }
        else
        {
            addToList("未到达负限位");
            bToNegativeLimit = false;
        }

        if(status&0x20 == 1)//方向错误标志
        {
            addToList("方向无误");
            bDirectionError = true;
        }
        else
        {
            addToList("方向错误");
            bDirectionError = false;
        }


        if(status&0x40 == 1)//到达限位标志
        {
            addToList("到达限位");
            bToLimit = true;
        }
        else
        {
            addToList("未到限位");
            bToLimit = false;
        }
    }
    else if (command == 0x0c)//应答帧
    {
        if(device_id == 0x0)
        {
            addToList("x轴电机应答");
        }
        else if(device_id == 0x1)
        {
            addToList("y轴电机应答");
        }
        else if(device_id == 0x2)
        {
            addToList("z轴电机应答");
        }
        else if(device_id == 0x5)
        {
            addToList("温度控制应答");
        }
        else if(device_id == 0x6)
        {
            addToList("动作控制应答");
        }
        else if(device_id == 0x7)
        {
            addToList("故障帧应答");
        }
    }

//    //应答帧
//    else if (command == 0x0c)//应答帧
//    {
//        //cout<<"收到设备"<<device_id<<"的应答帧"<<endl;
//        if (device_id == 0x04)//舵机复位完成
//        {
//            cout << "舵机复位完成" << endl;
//            status = WAIT_FOR_START;
//            sendMoveCommand(200, 5900, 15000);//走到床头
//        }

//        if (device_id == 0x06)//动作控制帧：以当前位置为圆心画圆
//        {
//            if (status == WAIT_FOR_TREATMENT)
//                status = IN_TREATMENT;//进入治疗中状态，开始计时
//            else if (status == IN_TREATMENT)//按键暂停的应答帧
//            {
//                cout << "method=" << acu_data_pack.actions[action_index].method << endl;
//                if (acu_data_pack.actions[action_index].method != 3)//若当前手法不是画圆，则收到应答帧即可认为电机停止
//                {
//                    sendDcmotorCommand(2500, 900, false, true);
//                    status = DC_MOTOR_UP;
//                    //status = PAUSE;
//                }
//            }
//            else if (status == PAUSE)
//            {
//                status = IN_TREATMENT;
//            }
//            else if (status == TIME_OUT)
//            {
//                if (acu_data_pack.actions[action_index - 1].method != 3)//若当前手法不是画圆，则收到应答帧即可认为电机停止
//                {
//                    if (action_index == acu_data_pack.action_num)
//                    {
//                        cout << "直流电机提起" << endl;
//                        sendDcmotorCommand(2500, 900, false, true);//直流电机提起
//                        status = DC_MOTOR_UP;
//                    }
//                    else
//                    {
//                        //发送指令进行下一个按摩手法
//                        cout << "开始执行第" << action_index + 1 << "个按摩手法" << endl;
//                        //开始对当前手法设定温度，等待治疗
//                        sendSetTemperature(acu_data_pack.actions[action_index].temperature * 10);
//                        status = WAIT_FOR_TREATMENT;
//                    }
//                }
//            }

//            cout << "治疗手法应答后,status=" << status << endl;
//        }
//    }

//    //按键帧
//    else if (command == 0x3f)//按键帧
//    {
////        if (device_id == 0x08)
////        {
////            unsigned char key = data[4];
////            cout << "收到按键，键值为" << (int)key << endl;
////            if (key == 0)//按键值为WORK,重新开启工作状态
////            {
////                cout << "收到WORK按键" << endl;

////                if (status == PAUSE)
////                {
////                    sendDcmotorCommand(1500, 400, true, true);
////                    status = DC_MOTOR_DOWN;
////                }
////            }
////            else if (key == 1)//按键值为PAUSE,暂停工作
////            {
////                cout << "收到PAUSE按键" << endl;
////                if (status == IN_TREATMENT)
////                {
////                    if (acu_data_pack.actions[action_index].method == 1)
////                        sendQuezhuoCommand(false);
////                    else if (acu_data_pack.actions[action_index].method == 2)
////                        sendDiananCommand(false);
////                    else if (acu_data_pack.actions[action_index].method == 3)
////                        sendHuayuanCommand(false);
////                }
////            }
////            else if (key == 2)//按键值为STOP，停止工作
////            {
////                cout << "收到STOP按键" << endl;
////                if (acu_data_pack.actions[action_index].method == 1)
////                    sendQuezhuoCommand(false);
////                else if (acu_data_pack.actions[action_index].method == 2)
////                    sendDiananCommand(false);
////                else if (acu_data_pack.actions[action_index].method == 3)
////                    sendHuayuanCommand(false);
////                stopFlag = true;
////                //sendDcmotorCommand(2500, 900, false, true);
////                //status = DC_MOTOR_UP;
////            }
////        }
//    }

//    //温度反馈
//    else if (command == 0x1f)//温度反馈
//    {
//        unsigned short temp;
//        unsigned char level;
//        temprature = 0;
//        temp = (unsigned char)data[3];
//        temprature |= temp;
//        temp = (unsigned char)data[4];
//        temprature |= temp << 8;
//        cout << "当前温度值为" << (float)temprature / 10 << endl;
//        level = temp2Level(temprature / 10);
//        sendTempLevel(level);

//        if (status == WAIT_FOR_TREATMENT)
//        {
//            cout << "检查温度是否符合要求" << endl;
//            if (abs(temprature / 10 - acu_data_pack.actions[action_index].temperature) < 10)//如果当前温度与设定温度差值小于一定值
//            {
//                /*if (acu_data_pack.actions[action_index].method == 1)
//                sendQuezhuoCommand(true);
//                else if (acu_data_pack.actions[action_index].method == 2)
//                sendDiananCommand(true);
//                else if (acu_data_pack.actions[action_index].method == 3)
//                sendHuayuanCommand(true);
//                //status = IN_TREATMENT;

//                timeRemaining = acu_data_pack.actions[action_index].time * 10;//开始计时，时间单位为0.1秒*/
//                cout << "温度满足要求" << endl;
//                if (action_index == 0)
//                {
//                    sendDcmotorCommand(1500, 400, true, true);
//                    status = DC_MOTOR_DOWN;
//                }
//                else
//                {
//                    if (acu_data_pack.actions[action_index].method == 1)
//                        sendQuezhuoCommand(true);
//                    else if (acu_data_pack.actions[action_index].method == 2)
//                        sendDiananCommand(true);
//                    else if (acu_data_pack.actions[action_index].method == 3)
//                        sendHuayuanCommand(true);
//                    //status = IN_TREATMENT;

//                    timeRemaining = acu_data_pack.actions[action_index].time * 10;
//                }

//            }
//        }
//    }

//    //设定温度级别
//    else if (command == 0x40)//设定温度级别
//    {
//        if (device_id == 0x08)
//        {
//            unsigned short temp;
//            temp = level2Temp(data[4]);
//            cout << "收到设定温度级别，设定温度为" << temp << "度" << endl;
//            sendSetTemperature(temp * 10);
//        }
//    }
}


Mat MainWindow::jiaozheng( Mat image )
{
    Size image_size = image.size();
    Mat R = Mat::eye(3,3,CV_32F);
    Mat mapx = Mat(image_size,CV_32FC1);
    Mat mapy = Mat(image_size,CV_32FC1);
    Mat intrinsic_matrix;
    initUndistortRectifyMap(cameraMatrix_L,distCoeffs_L,R,intrinsic_matrix,image_size,CV_32FC1,mapx,mapy);
    Mat t = image.clone();
    cv::remap( image, t, mapx, mapy, INTER_LINEAR);
    return t;
}





void MainWindow::on_saveButton_clicked()
{
    m_bSaveImage = true;//开始保存图像
}

//计算校验值
unsigned char MainWindow::jiaoyan(unsigned char* data)
{
    int sum = 0;
    for(int i=0;i<10;i++)
    {
        sum += data[i];
    }

    return sum&0xff;//取低8位
}

//电机复位指令帧
void MainWindow::SendMotorResetCmd(int id,int speed,int pos)
{
    unsigned char txData[20] = {0};

    txData[0] = 0x5a;
    txData[1] = id; //包括0:横轴电机、1:纵轴电机、2:垂向电机
    txData[2] = 0x21;//下行数据帧：复位指令
    txData[4] = speed&0xff;//低8位
    txData[5] = (speed>>8)&0xff;//高8位
    txData[6] = pos&0xff;
    txData[7] = (pos>>8)&0xff;
    txData[10] = jiaoyan(txData);

    if(serial)
        serial->write((const char*)txData,11);

}

//电机运动指令帧
void MainWindow::SendMotorRunCmd(int id,int speed,int pos)
{
    unsigned char txData[20] = {0};

    txData[0] = 0x5a;
    txData[1] = id; //包括0:横轴电机、1:纵轴电机、2:垂向电机
    txData[2] = 0x01;//下行数据帧：运动指令
    txData[4] = speed&0xff;//低8位
    txData[5] = (speed>>8)&0xff;//高8位
    txData[6] = pos&0xff;
    txData[7] = (pos>>8)&0xff;
    txData[10] = jiaoyan(txData);

    if(serial)
        serial->write((const char*)txData,11);
}


//温度控制指令帧
void MainWindow::SendTempretureControlCmd(int value)
{
    unsigned char txData[20] = {0};

    txData[0] = 0x5a;
    txData[1] = 5; //包括0:横轴电机、1:纵轴电机、2:垂向电机
    txData[2] = 0x11;//下行数据帧：温度控制
    txData[6] = value&0xff;
    txData[7] = (value>>8)&0xff;
    txData[10] = jiaoyan(txData);

    if(serial)
        serial->write((const char*)txData,11);
}
//动作控制指令帧，画圆
void MainWindow::SendHuayuanCmd(bool bEnable,int speed,int radius)
{
    unsigned char txData[20] = {0};

    txData[0] = 0x5a;
    txData[1] = 6; //动作控制
    txData[2] = 0x31;//下行数据帧：画圆
    txData[3] = bEnable?1:0;
    txData[4] = speed&0xff;//低8位
    txData[5] = (speed>>8)&0xff;//高8位
    txData[6] = radius&0xff;
    txData[7] = (radius>>8)&0xff;
    txData[10] = jiaoyan(txData);

    if(serial)
        serial->write((const char*)txData,11);
}

//动作控制指令帧，雀琢
void MainWindow::SendQuezuoCmd(bool bEnable,int speed,int pos,int maxResrved)
{
    unsigned char txData[20] = {0};

    txData[0] = 0x5a;
    txData[1] = 6; //动作控制
    txData[2] = 0x32;//下行数据帧：动作指令
    txData[3] = bEnable?1:0;
    txData[4] = speed&0xff;//低8位
    txData[5] = (speed>>8)&0xff;//高8位
    txData[6] = pos&0xff;
    txData[7] = (pos>>8)&0xff;
    txData[8] = maxResrved&0xff;
    txData[9] = (maxResrved>>8)&0xff;
    txData[10] = jiaoyan(txData);

    if(serial)
        serial->write((const char*)txData,11);
}

//下行故障帧，不定时，有故障即发 (有)
void MainWindow::SendFaultCmd(bool bFault)
{
    unsigned char txData[20] = {0};

    txData[0] = 0x5a;
    txData[1] = 7; //动作控制
    txData[2] = 0x2f;//下行数据帧：画圆
    txData[4] =bFault?0:1;//低8位

    txData[10] = jiaoyan(txData);

    if(serial)
        serial->write((const char*)txData,11);
}

//实时温度级别帧，1Hz
void MainWindow::SendTempretureLevelCmd(int level)
{
    unsigned char txData[20] = {0};

    txData[0] = 0x5a;
    txData[1] = 87; //动作控制
    txData[2] = 0x40;//下行数据帧：画圆
    txData[4] = level;//

    txData[10] = jiaoyan(txData);

    if(serial)
        serial->write((const char*)txData,11);
}

void MainWindow::on_checkBox_clicked(bool checked)
{
    m_bShowImage = checked;
}


