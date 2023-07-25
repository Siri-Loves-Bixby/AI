#include <QtCore/QCoreApplication>
#include <complex>
#include <QDebug>
#include "gdal.h"
#include "gdal_priv.h"
#include "cpl_conv.h"
#include <algorithm>
#include <QFile>
#include <QRegularExpression>

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    GDALAllRegister();

    //打开影像文件，获取GDALDatasetH对象和GDALRasterBand对象
    const char* inImgPath = "D:\\2\\Image.tiff";
    const char* outImgPath = "D:\\2\\DOM2.tif";
    const char* outImgPath1 = "D:\\2\\DOM3.tif";
    const char* midImgPath = "D:\\2\\DOMmid.tif";
    const char* demImgPath = "D:\\2\\dem1.tif";
    GDALDataset* demDataset = (GDALDataset*)GDALOpen(demImgPath, GA_ReadOnly);
    GDALDataset* inDataset = (GDALDataset*)GDALOpen(inImgPath, GA_ReadOnly);
    GDALRasterBandH inBand1 = GDALGetRasterBand(inDataset, 1);
    GDALRasterBandH inBand2 = GDALGetRasterBand(inDataset, 2);

    //获取影像的宽度和高度。
    int nXSize = inDataset->GetRasterXSize();
    int nYSize = inDataset->GetRasterYSize();
    int nRasterCount = inDataset->GetRasterCount();
    GDALDataType data_type = inDataset->GetRasterBand(1)->GetRasterDataType();
    int demXSize = demDataset->GetRasterXSize();//列数
    int demYSize = demDataset->GetRasterYSize();//行数
    float domGSD = 0.00005;
    //int nRasterCount = demDataset->GetRasterCount();
    //GDALDataType data_type = demDataset->GetRasterBand(1)->GetRasterDataType();

    //dem范围
    double demGeoTransform[6];
    demDataset->GetGeoTransform(demGeoTransform);//读出来的是第一个像素左上角的坐标
    double demMaxLat = demGeoTransform[3] + demGeoTransform[5] / 2;//维度最大
    double demMinLot = demGeoTransform[0] + demGeoTransform[1] / 2;//经度最小
    double demMinLat = demMaxLat + (demYSize - 1) * demGeoTransform[5];//维度最小
    double demMaxLot = demMinLot + (demXSize - 1) * demGeoTransform[1];//经度最大
    double demGSDx = demGeoTransform[1];
    double demGSDy = 0 - demGeoTransform[5];

    //DOM范围
    double domMaxLat = demMaxLat;//维度最大
    double domMinLot = demMinLot;//经度最小
    double domMinLat = demMinLat;//维度最小
    double domMaxLot = demMaxLot;//经度最大
    //double domGSD = demGSDx;
    int domXSize = (domMaxLot - domMinLot) / domGSD + 1;
    int domYSize = (domMaxLat - domMinLat) / domGSD + 1;

    //读RPC
    //QString rpcFilePath = "D:\\Code\\environment\\gdal\\gdal\\autotest\\gcore\\data\\md_dg.RPB";
    QString rpcFilePath = "D:\\2\\Image.rpb";
    GDALRPCInfoV2 rpcInfo{};//rpc结构体
    QStringList rpcContent;//文件内容字符串列表
    QFile aFile(rpcFilePath);  //以文件方式读出
    if (aFile.open(QIODevice::ReadOnly | QIODevice::Text)) //以只读文本方式打开文件
    {
        QTextStream aStream(&aFile); //用文本流读取文件
        while (!aStream.atEnd())
        {
            QString str = aStream.readLine();//读取文件的一行
            rpcContent.append(str); //添加到 StringList
        }
        aFile.close();//关闭文件
        for (size_t i = 0; i < 101; i++)//遍历文件每一行
        {
            QString aLineText = rpcContent.at(i);
            if (i == 0)
            {
                QRegularExpression re("\"([^\"]+)\"");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                if ((aLineText != "3-1") && (aLineText != "3-2"))
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            else if (i == 1)
            {
                QRegularExpression re("\"([^\"]+)\"");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                if (aLineText != "SAR")
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            else if (i == 2)
            {
                QRegularExpression re("\"([^\"]+)\"");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                if ((aLineText != "RPC"))
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            else if (i == 3)
            {
                if ((aLineText != "BEGIN_GROUP = IMAGE"))
                {
                    break;
                }
                else
                {
                    continue;
                }
            }
            else if (i == 4)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfERR_BIAS = TempRPC;
                    continue;
                }
            }
            else if (i == 5)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfERR_RAND = TempRPC;
                    continue;
                }
            }
            else if (i == 6)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfLINE_OFF = TempRPC;
                    continue;
                }
            }
            else if (i == 7)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfSAMP_OFF = TempRPC;
                    continue;
                }
            }
            else if (i == 8)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < -90 || TempRPC > 90)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfLAT_OFF = TempRPC;
                    continue;
                }
            }
            else if (i == 9)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < -180 || TempRPC > 180)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfLONG_OFF = TempRPC;
                    continue;
                }
            }
            else if (i == 10)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                rpcInfo.dfHEIGHT_OFF = TempRPC;
                continue;
            }
            else if (i == 11)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfLINE_SCALE = TempRPC;
                    continue;
                }
            }
            else if (i == 12)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0)
                {
                    break;
                }
                else
                {
                    rpcInfo.dfSAMP_SCALE = TempRPC;
                    continue;
                }
            }
            else if (i == 13)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0 || TempRPC > 90)
                {
                    //qDebug() << "RPC is wrong";
                    break;
                }
                else
                {
                    rpcInfo.dfLAT_SCALE = TempRPC;
                    //qDebug() << rpcInfo.dfLAT_SCALE;
                    continue;
                }
            }
            else if (i == 14)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0 || TempRPC > 180)
                {
                    //qDebug() << "RPC is wrong";
                    break;
                }
                else
                {
                    rpcInfo.dfLONG_SCALE = TempRPC;
                    //qDebug() << rpcInfo.dfLONG_SCALE;
                    continue;
                }
            }
            else if (i == 15)
            {
                QRegularExpression re("=\\s*([\\d\\.]+)\\s*;");
                QRegularExpressionMatch match = re.match(aLineText);
                if (match.hasMatch()) {
                    aLineText = match.captured(1);
                }
                double TempRPC = aLineText.trimmed().toDouble();
                if (TempRPC < 0)
                {
                    //qDebug() << "RPC is wrong";
                    break;
                }
                else
                {
                    rpcInfo.dfHEIGHT_SCALE = TempRPC;
                    //qDebug() << rpcInfo.dfHEIGHT_SCALE;
                    continue;
                }
            }
            else if (i == 16)
            {
                continue;
            }
            else if (i > 16 && i < 37)
            {
                double TempRPC = aLineText.replace(",", "").replace(");", "").toDouble();
                rpcInfo.adfLINE_NUM_COEFF[i - 17] = TempRPC;
                //qDebug() << rpcInfo.adfLINE_NUM_COEFF[i - 17];
                continue;
            }
            else if (i == 37)
            {
                continue;
            }
            else if (i > 37 && i < 58)
            {
                double TempRPC = aLineText.replace(",", "").replace(");", "").toDouble();
                rpcInfo.adfLINE_DEN_COEFF[i - 38] = TempRPC;
                //qDebug() << rpcInfo.adfLINE_DEN_COEFF[i - 38];
                continue;
            }
            else if (i == 58)
            {
                continue;
            }
            else if (i > 58 && i < 79)
            {
                double TempRPC = aLineText.replace(",", "").replace(");", "").toDouble();
                rpcInfo.adfSAMP_NUM_COEFF[i - 59] = TempRPC;
                //qDebug() << rpcInfo.adfSAMP_NUM_COEFF[i - 59];
                continue;
            }
            else if (i == 79)
            {
                continue;
            }
            else if (i > 79 && i < 100)
            {
                double TempRPC = aLineText.replace(",", "").replace(");", "").toDouble();
                rpcInfo.adfSAMP_DEN_COEFF[i - 80] = TempRPC;
                //qDebug() << rpcInfo.adfSAMP_DEN_COEFF[i - 80];
                continue;
            }
            else {
                break;
            }
        }
    }
    qDebug() << "read RPC over" << "\n";

    //创建输出影像
    GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName("GTiff");;
    GDALDataset* outDataset = hDriver->Create(outImgPath, domXSize, domYSize, 1, GDT_Byte, NULL);

    //幅度影像
    GDALDriver* mDriver = GetGDALDriverManager()->GetDriverByName("GTiff");;
    GDALDataset* outMidDataset = mDriver->Create(midImgPath, nXSize, nYSize, 1, GDT_Byte, NULL);
    unsigned char* fudu = new unsigned char[nXSize * nYSize];

    //未拉伸的影像
    GDALDriver* m1Driver = GetGDALDriverManager()->GetDriverByName("GTiff");;
    GDALDataset* out1Dataset = m1Driver->Create(outImgPath1, domXSize, domYSize, 1, GDT_Int16, NULL);
    short* zhichu = new short[domXSize * domYSize];

    //读取影像数据到内存中。
    short* inData1 = new short[nXSize * nYSize];
    short* inData2 = new short[nXSize * nYSize];
    float* outDataTemp = new float[nXSize * nYSize];//存储取模后的浮点数

    inDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, nXSize, nYSize, inData1, nXSize, nYSize, GDT_Int16, 0, 0);
    inDataset->GetRasterBand(2)->RasterIO(GF_Read, 0, 0, nXSize, nYSize, inData2, nXSize, nYSize, GDT_Int16, 0, 0);

    //复数取模
    for (int i = 0; i < nXSize * nYSize; i++)
    {
        //qDebug() << i;
        float absValue = std::sqrt(inData1[i] * inData1[i] + inData2[i] * inData2[i]);
        outDataTemp[i] = absValue;
        fudu[i] = (unsigned char)absValue;
    }
    outMidDataset->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, nXSize, nYSize, fudu, nXSize, nYSize, GDT_Byte, 0, 0); 
    delete[] inData1;
    delete[] inData2;
    delete[] fudu;
    GDALClose(outMidDataset);
    GDALClose(inDataset);
    qDebug() << "complex image compute over" << "\n";

    //构建RFM模型，从DOM反算，获取像素值
    float* outData = new float[domXSize * domYSize];//插值后DOM的像素值
    short* demData = new short[demXSize * demYSize];
    demDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, demXSize, demYSize, demData, demXSize, demYSize, GDT_Int16, 0, 0);
    double pixelB;//DOM像素点对应的维度
    double pixelL;//DOM像素点对应的经度
    double pixelH;//DOM像素点对应的高程
    int errNum1 = 0;//超出影像范围区
    int errNum2 = 0;//无高程值
    int higErrNum = 0;
    for (size_t i = 0; i < domYSize; i++)//逐行循环,行数
    {
        for (size_t j = 0; j < domXSize; j++)//每行逐像素循环，列数
        {
            pixelB = domMaxLat - domGSD * i;
            pixelL = domMinLot + domGSD * j;

            //双线性内插高程值
            int row = (pixelB - demMaxLat) / demGeoTransform[5];//DEM中行数
            int col = (pixelL - demMinLot) / demGeoTransform[1];//DEM中列数
            int p1 = demXSize * row + col;
            //普通情况
            if (((pixelB <= demMaxLat) && (pixelB >= (demMinLat + demGeoTransform[1]))) && ((pixelL <= (demMaxLot - demGeoTransform[1])) && (pixelL >= demMinLot)))
            {
                short r1 = demData[p1];
                short r2 = demData[p1 + 1];
                short r3 = demData[p1 + demXSize];
                short r4 = demData[p1 + demXSize + 1];
                float xscale = (pixelB - demMaxLat) / demGeoTransform[5] - row;
                float yscale = (pixelL - demMinLot) / demGSDx - col;
                float r12 = r1 * xscale + r2 * (1 - xscale);
                float r34 = r3 * xscale + r4 * (1 - xscale);
                pixelH = r12 * yscale + r34 * (1 - yscale);
            }
            //最底下一行的特例，单线性插值
            else if (((pixelB >= demMinLat) && (pixelB < (demMinLat + demGeoTransform[1]))) && ((pixelL < (demMaxLot - demGeoTransform[1])) && (pixelL >= demMinLot)))
            {
                short r1 = demData[p1];
                short r2 = demData[p1 + 1];
                float xscale = (pixelB - demMaxLat) / demGeoTransform[5] - row;
                float r12 = r1 * xscale + r2 * (1 - xscale);
                pixelH = r12;
            }
            //最右侧一列的特例
            else if (((pixelB < demMaxLat) && (pixelB >= (demMinLat + demGeoTransform[1]))) && ((pixelL >= (demMaxLot - demGeoTransform[1])) && (pixelL < demMaxLot)))
            {
                short r1 = demData[p1];
                short r3 = demData[p1 + demXSize];
                float yscale = (pixelL - demMinLot) / demGSDx - col;
                pixelH = r1 * yscale + r3 * (1 - yscale);
            }//最右下角的特例，最临近
            else if (((pixelB >= demMinLat) && (pixelB < (demMinLat + demGeoTransform[1]))) && ((pixelL >= (demMaxLot - demGeoTransform[1])) && (pixelL < demMaxLot)))
            {
                pixelH = static_cast<float>(demData[p1]);
            }
            else
            {
                pixelH = -9999;
                higErrNum++;
            }
            
            //RFM算行列号
            if (pixelH != -9999)
            {
                //归一化
                pixelB = (pixelB - rpcInfo.dfLAT_OFF) / rpcInfo.dfLAT_SCALE;
                pixelL = (pixelL - rpcInfo.dfLONG_OFF) / rpcInfo.dfLONG_SCALE;
                pixelH = (pixelH - rpcInfo.dfHEIGHT_OFF) / rpcInfo.dfHEIGHT_SCALE;
                double line_NUM = rpcInfo.adfLINE_NUM_COEFF[0]
                    + rpcInfo.adfLINE_NUM_COEFF[1] * pixelL + rpcInfo.adfLINE_NUM_COEFF[2] * pixelB + rpcInfo.adfLINE_NUM_COEFF[3] * pixelH
                    + rpcInfo.adfLINE_NUM_COEFF[4] * pixelL * pixelB + rpcInfo.adfLINE_NUM_COEFF[5] * pixelL * pixelH + rpcInfo.adfLINE_NUM_COEFF[6] * pixelH * pixelB
                    + rpcInfo.adfLINE_NUM_COEFF[7] * pixelL * pixelL+ rpcInfo.adfLINE_NUM_COEFF[8] * pixelB * pixelB + rpcInfo.adfLINE_NUM_COEFF[9] * pixelH * pixelH
                    + rpcInfo.adfLINE_NUM_COEFF[10] * pixelL * pixelB * pixelH+ rpcInfo.adfLINE_NUM_COEFF[11] * pixelL * pixelL * pixelL + rpcInfo.adfLINE_NUM_COEFF[12]  * pixelL * pixelB * pixelB
                    + rpcInfo.adfLINE_NUM_COEFF[13] * pixelL * pixelH * pixelH + rpcInfo.adfLINE_NUM_COEFF[14] * pixelL * pixelL * pixelB + rpcInfo.adfLINE_NUM_COEFF[15] * pixelB * pixelB * pixelB
                    + rpcInfo.adfLINE_NUM_COEFF[16] * pixelB * pixelH * pixelH + rpcInfo.adfLINE_NUM_COEFF[17] * pixelL * pixelL * pixelH + rpcInfo.adfLINE_NUM_COEFF[18] * pixelB * pixelB * pixelH
                    + rpcInfo.adfLINE_NUM_COEFF[19] * pixelH * pixelH * pixelH;
                double line_DEN = rpcInfo.adfLINE_DEN_COEFF[0]
                    + rpcInfo.adfLINE_DEN_COEFF[1] * pixelL + rpcInfo.adfLINE_DEN_COEFF[2] * pixelB + rpcInfo.adfLINE_DEN_COEFF[3] * pixelH
                    + rpcInfo.adfLINE_DEN_COEFF[4] * pixelL * pixelB + rpcInfo.adfLINE_DEN_COEFF[5] * pixelL * pixelH + rpcInfo.adfLINE_DEN_COEFF[6] * pixelH * pixelB
                    + rpcInfo.adfLINE_DEN_COEFF[7] * pixelL * pixelL + rpcInfo.adfLINE_DEN_COEFF[8] * pixelB * pixelB + rpcInfo.adfLINE_DEN_COEFF[9] * pixelH * pixelH
                    + rpcInfo.adfLINE_DEN_COEFF[10] * pixelL * pixelB * pixelH + rpcInfo.adfLINE_DEN_COEFF[11] * pixelL * pixelL * pixelL + rpcInfo.adfLINE_DEN_COEFF[12] * pixelL * pixelB * pixelB
                    + rpcInfo.adfLINE_DEN_COEFF[13] * pixelL * pixelH * pixelH + rpcInfo.adfLINE_DEN_COEFF[14] * pixelL * pixelL * pixelB + rpcInfo.adfLINE_DEN_COEFF[15] * pixelB * pixelB * pixelB
                    + rpcInfo.adfLINE_DEN_COEFF[16] * pixelB * pixelH * pixelH + rpcInfo.adfLINE_DEN_COEFF[17] * pixelL * pixelL * pixelH + rpcInfo.adfLINE_DEN_COEFF[18] * pixelB * pixelB * pixelH
                    + rpcInfo.adfLINE_DEN_COEFF[19] * pixelH * pixelH * pixelH;
                double samp_NUM = rpcInfo.adfSAMP_NUM_COEFF[0]
                    + rpcInfo.adfSAMP_NUM_COEFF[1] * pixelL + rpcInfo.adfSAMP_NUM_COEFF[2] * pixelB + rpcInfo.adfSAMP_NUM_COEFF[3] * pixelH
                    + rpcInfo.adfSAMP_NUM_COEFF[4] * pixelL * pixelB + rpcInfo.adfSAMP_NUM_COEFF[5] * pixelL * pixelH + rpcInfo.adfSAMP_NUM_COEFF[6] * pixelH * pixelB
                    + rpcInfo.adfSAMP_NUM_COEFF[7] * pixelL * pixelL + rpcInfo.adfSAMP_NUM_COEFF[8] * pixelB * pixelB + rpcInfo.adfSAMP_NUM_COEFF[9] * pixelH * pixelH
                    + rpcInfo.adfSAMP_NUM_COEFF[10] * pixelL * pixelB * pixelH + rpcInfo.adfSAMP_NUM_COEFF[11] * pixelL * pixelL * pixelL + rpcInfo.adfSAMP_NUM_COEFF[12] * pixelL * pixelB * pixelB
                    + rpcInfo.adfSAMP_NUM_COEFF[13] * pixelL * pixelH * pixelH + rpcInfo.adfSAMP_NUM_COEFF[14] * pixelL * pixelL * pixelB + rpcInfo.adfSAMP_NUM_COEFF[15] * pixelB * pixelB * pixelB
                    + rpcInfo.adfSAMP_NUM_COEFF[16] * pixelB * pixelH * pixelH + rpcInfo.adfSAMP_NUM_COEFF[17] * pixelL * pixelL * pixelH + rpcInfo.adfSAMP_NUM_COEFF[18] * pixelB * pixelB * pixelH
                    + rpcInfo.adfSAMP_NUM_COEFF[19] * pixelH * pixelH * pixelH;
                double samp_DEN = rpcInfo.adfSAMP_DEN_COEFF[0]
                    + rpcInfo.adfSAMP_DEN_COEFF[1] * pixelL + rpcInfo.adfSAMP_DEN_COEFF[2] * pixelB + rpcInfo.adfSAMP_DEN_COEFF[3] * pixelH
                    + rpcInfo.adfSAMP_DEN_COEFF[4] * pixelL * pixelB + rpcInfo.adfSAMP_DEN_COEFF[5] * pixelL * pixelH + rpcInfo.adfSAMP_DEN_COEFF[6] * pixelH * pixelB
                    + rpcInfo.adfSAMP_DEN_COEFF[7] * pixelL * pixelL + rpcInfo.adfSAMP_DEN_COEFF[8] * pixelB * pixelB + rpcInfo.adfSAMP_DEN_COEFF[9] * pixelH * pixelH
                    + rpcInfo.adfSAMP_DEN_COEFF[10] * pixelL * pixelB * pixelH + rpcInfo.adfSAMP_DEN_COEFF[11] * pixelL * pixelL * pixelL + rpcInfo.adfSAMP_DEN_COEFF[12] * pixelL * pixelB * pixelB
                    + rpcInfo.adfSAMP_DEN_COEFF[13] * pixelL * pixelH * pixelH + rpcInfo.adfSAMP_DEN_COEFF[14] * pixelL * pixelL * pixelB + rpcInfo.adfSAMP_DEN_COEFF[15] * pixelB * pixelB * pixelB
                    + rpcInfo.adfSAMP_DEN_COEFF[16] * pixelB * pixelH * pixelH + rpcInfo.adfSAMP_DEN_COEFF[17] * pixelL * pixelL * pixelH + rpcInfo.adfSAMP_DEN_COEFF[18] * pixelB * pixelB * pixelH
                    + rpcInfo.adfSAMP_DEN_COEFF[19] * pixelH * pixelH * pixelH;
                double inImgRow = line_NUM / line_DEN * rpcInfo.dfLINE_SCALE + rpcInfo.dfLINE_OFF;
                double inImgCol = samp_NUM / samp_DEN * rpcInfo.dfSAMP_SCALE + rpcInfo.dfSAMP_OFF;

                //双线性内插获取像素值
                if (((inImgRow > 0) && (inImgRow < (nYSize - 1))) && (((inImgCol > 0) && inImgCol < (nXSize - 1))))
                {
                    int leftTop = ((int)inImgRow * nXSize) + ((int)inImgCol);//在原图像中左上角的像素值的位置
                    double xscale = inImgRow - static_cast<int>(inImgRow);
                    double yscale = inImgCol - static_cast<int>(inImgCol);
                    double pixVal1 = outDataTemp[leftTop];
                    double pixVal2 = outDataTemp[leftTop + 1];
                    double pixVal3 = outDataTemp[leftTop + nXSize];
                    double pixVal4 = outDataTemp[leftTop + nXSize + 1];
                    double pixVal12 = xscale * pixVal1 + (1 - xscale) * pixVal2;
                    double pixVal34 = xscale * pixVal3 + (1 - xscale) * pixVal4;
                    double pixVal = yscale * pixVal12 + (1 - yscale) * pixVal34;
                    outData[i * domYSize + j] = (float)pixVal;
                }
                else
                {
                    outData[i * domXSize + j] = 0;//超出影像范围区
                    errNum1++;
                }
                
            }
            else
            {
                outData[i * domXSize + j] = 0;//DEM无值区
                errNum2++;
            }
        }
    }
    delete[] demData;
    delete[] outDataTemp;
    GDALClose(demDataset);
    qDebug() << "RPC compute over" << "\n";
    qDebug() << higErrNum << "\n";
    qDebug() << errNum1 << "\n" << errNum2;

    //线性拉伸
    float str_per = 1;//拉伸百分比
    int minThreshold = static_cast<int>(str_per/100* domXSize* domYSize);//最低阈值像素的位置
    int maxThreshold = domXSize * domYSize- minThreshold;//最大阈值像素的位置
    float* outDataSort = new float[domXSize * domYSize];//排序用的数组
    float minValue = *std::min_element(outData, outData + domXSize * domYSize);
    float maxValue = *std::max_element(outData, outData + domXSize * domYSize);
    std::copy(outData, outData + domXSize * domYSize, outDataSort);
    std::sort(outDataSort, outDataSort + domXSize * domYSize);
    unsigned short minPixThreshold = static_cast<unsigned short>(outDataSort[minThreshold]);//最低阈值
    unsigned short maxPixThreshold = static_cast<unsigned short>(outDataSort[maxThreshold]);//最大阈值
    delete[] outDataSort;

    int min = 0;
    int max = 0;
    //迭代，将超出阈值的像素全部卡掉
    for (size_t i = 0; i < domXSize * domYSize; i++)
    {
        zhichu[i] = (short)outData[i];
        if (outData[i] < minPixThreshold)
        {
            outData[i] = minPixThreshold;
            min++;
            //continue;
        }
        else if (outData[i] > maxThreshold)
        {
            outData[i] = maxThreshold;
            max++;
            //continue;
        }
        continue;
    }

    out1Dataset->GetRasterBand(1)->RasterIO(GF_Write, 0, 0, domXSize, domYSize, zhichu, domXSize, domYSize, GDT_Byte, 0, 0);
    delete zhichu;
    GDALClose(out1Dataset);

    //迭代，线性拉伸
    double stretchFactor = 255 / (maxThreshold - minPixThreshold) * 0.8;
    unsigned char* outDataWrite = new unsigned char[domXSize * domYSize];//最终写入影像的数组
    for (size_t i = 0; i < domXSize * domYSize; i++)
    {
        outDataWrite[i] = static_cast<unsigned char>((outData[i] - minPixThreshold) * stretchFactor);
    }

    delete[] outData;
    //将修改后的数据写回到影像文件中。


    qDebug() << "liner compute over" << "\n";

    //释放内存并关闭文件。      
    delete[] outDataWrite;
    GDALClose(outDataset);

    qDebug() << "compute over" << "\n";

    return a.exec();
}