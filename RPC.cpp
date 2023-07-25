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

    //��Ӱ���ļ�����ȡGDALDatasetH�����GDALRasterBand����
    const char* inImgPath = "D:\\2\\Image.tiff";
    const char* outImgPath = "D:\\2\\DOM2.tif";
    const char* outImgPath1 = "D:\\2\\DOM3.tif";
    const char* midImgPath = "D:\\2\\DOMmid.tif";
    const char* demImgPath = "D:\\2\\dem1.tif";
    GDALDataset* demDataset = (GDALDataset*)GDALOpen(demImgPath, GA_ReadOnly);
    GDALDataset* inDataset = (GDALDataset*)GDALOpen(inImgPath, GA_ReadOnly);
    GDALRasterBandH inBand1 = GDALGetRasterBand(inDataset, 1);
    GDALRasterBandH inBand2 = GDALGetRasterBand(inDataset, 2);

    //��ȡӰ��Ŀ�Ⱥ͸߶ȡ�
    int nXSize = inDataset->GetRasterXSize();
    int nYSize = inDataset->GetRasterYSize();
    int nRasterCount = inDataset->GetRasterCount();
    GDALDataType data_type = inDataset->GetRasterBand(1)->GetRasterDataType();
    int demXSize = demDataset->GetRasterXSize();//����
    int demYSize = demDataset->GetRasterYSize();//����
    float domGSD = 0.00005;
    //int nRasterCount = demDataset->GetRasterCount();
    //GDALDataType data_type = demDataset->GetRasterBand(1)->GetRasterDataType();

    //dem��Χ
    double demGeoTransform[6];
    demDataset->GetGeoTransform(demGeoTransform);//���������ǵ�һ���������Ͻǵ�����
    double demMaxLat = demGeoTransform[3] + demGeoTransform[5] / 2;//ά�����
    double demMinLot = demGeoTransform[0] + demGeoTransform[1] / 2;//������С
    double demMinLat = demMaxLat + (demYSize - 1) * demGeoTransform[5];//ά����С
    double demMaxLot = demMinLot + (demXSize - 1) * demGeoTransform[1];//�������
    double demGSDx = demGeoTransform[1];
    double demGSDy = 0 - demGeoTransform[5];

    //DOM��Χ
    double domMaxLat = demMaxLat;//ά�����
    double domMinLot = demMinLot;//������С
    double domMinLat = demMinLat;//ά����С
    double domMaxLot = demMaxLot;//�������
    //double domGSD = demGSDx;
    int domXSize = (domMaxLot - domMinLot) / domGSD + 1;
    int domYSize = (domMaxLat - domMinLat) / domGSD + 1;

    //��RPC
    //QString rpcFilePath = "D:\\Code\\environment\\gdal\\gdal\\autotest\\gcore\\data\\md_dg.RPB";
    QString rpcFilePath = "D:\\2\\Image.rpb";
    GDALRPCInfoV2 rpcInfo{};//rpc�ṹ��
    QStringList rpcContent;//�ļ������ַ����б�
    QFile aFile(rpcFilePath);  //���ļ���ʽ����
    if (aFile.open(QIODevice::ReadOnly | QIODevice::Text)) //��ֻ���ı���ʽ���ļ�
    {
        QTextStream aStream(&aFile); //���ı�����ȡ�ļ�
        while (!aStream.atEnd())
        {
            QString str = aStream.readLine();//��ȡ�ļ���һ��
            rpcContent.append(str); //��ӵ� StringList
        }
        aFile.close();//�ر��ļ�
        for (size_t i = 0; i < 101; i++)//�����ļ�ÿһ��
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

    //�������Ӱ��
    GDALDriver* hDriver = GetGDALDriverManager()->GetDriverByName("GTiff");;
    GDALDataset* outDataset = hDriver->Create(outImgPath, domXSize, domYSize, 1, GDT_Byte, NULL);

    //����Ӱ��
    GDALDriver* mDriver = GetGDALDriverManager()->GetDriverByName("GTiff");;
    GDALDataset* outMidDataset = mDriver->Create(midImgPath, nXSize, nYSize, 1, GDT_Byte, NULL);
    unsigned char* fudu = new unsigned char[nXSize * nYSize];

    //δ�����Ӱ��
    GDALDriver* m1Driver = GetGDALDriverManager()->GetDriverByName("GTiff");;
    GDALDataset* out1Dataset = m1Driver->Create(outImgPath1, domXSize, domYSize, 1, GDT_Int16, NULL);
    short* zhichu = new short[domXSize * domYSize];

    //��ȡӰ�����ݵ��ڴ��С�
    short* inData1 = new short[nXSize * nYSize];
    short* inData2 = new short[nXSize * nYSize];
    float* outDataTemp = new float[nXSize * nYSize];//�洢ȡģ��ĸ�����

    inDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, nXSize, nYSize, inData1, nXSize, nYSize, GDT_Int16, 0, 0);
    inDataset->GetRasterBand(2)->RasterIO(GF_Read, 0, 0, nXSize, nYSize, inData2, nXSize, nYSize, GDT_Int16, 0, 0);

    //����ȡģ
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

    //����RFMģ�ͣ���DOM���㣬��ȡ����ֵ
    float* outData = new float[domXSize * domYSize];//��ֵ��DOM������ֵ
    short* demData = new short[demXSize * demYSize];
    demDataset->GetRasterBand(1)->RasterIO(GF_Read, 0, 0, demXSize, demYSize, demData, demXSize, demYSize, GDT_Int16, 0, 0);
    double pixelB;//DOM���ص��Ӧ��ά��
    double pixelL;//DOM���ص��Ӧ�ľ���
    double pixelH;//DOM���ص��Ӧ�ĸ߳�
    int errNum1 = 0;//����Ӱ��Χ��
    int errNum2 = 0;//�޸߳�ֵ
    int higErrNum = 0;
    for (size_t i = 0; i < domYSize; i++)//����ѭ��,����
    {
        for (size_t j = 0; j < domXSize; j++)//ÿ��������ѭ��������
        {
            pixelB = domMaxLat - domGSD * i;
            pixelL = domMinLot + domGSD * j;

            //˫�����ڲ�߳�ֵ
            int row = (pixelB - demMaxLat) / demGeoTransform[5];//DEM������
            int col = (pixelL - demMinLot) / demGeoTransform[1];//DEM������
            int p1 = demXSize * row + col;
            //��ͨ���
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
            //�����һ�е������������Բ�ֵ
            else if (((pixelB >= demMinLat) && (pixelB < (demMinLat + demGeoTransform[1]))) && ((pixelL < (demMaxLot - demGeoTransform[1])) && (pixelL >= demMinLot)))
            {
                short r1 = demData[p1];
                short r2 = demData[p1 + 1];
                float xscale = (pixelB - demMaxLat) / demGeoTransform[5] - row;
                float r12 = r1 * xscale + r2 * (1 - xscale);
                pixelH = r12;
            }
            //���Ҳ�һ�е�����
            else if (((pixelB < demMaxLat) && (pixelB >= (demMinLat + demGeoTransform[1]))) && ((pixelL >= (demMaxLot - demGeoTransform[1])) && (pixelL < demMaxLot)))
            {
                short r1 = demData[p1];
                short r3 = demData[p1 + demXSize];
                float yscale = (pixelL - demMinLot) / demGSDx - col;
                pixelH = r1 * yscale + r3 * (1 - yscale);
            }//�����½ǵ����������ٽ�
            else if (((pixelB >= demMinLat) && (pixelB < (demMinLat + demGeoTransform[1]))) && ((pixelL >= (demMaxLot - demGeoTransform[1])) && (pixelL < demMaxLot)))
            {
                pixelH = static_cast<float>(demData[p1]);
            }
            else
            {
                pixelH = -9999;
                higErrNum++;
            }
            
            //RFM�����к�
            if (pixelH != -9999)
            {
                //��һ��
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

                //˫�����ڲ��ȡ����ֵ
                if (((inImgRow > 0) && (inImgRow < (nYSize - 1))) && (((inImgCol > 0) && inImgCol < (nXSize - 1))))
                {
                    int leftTop = ((int)inImgRow * nXSize) + ((int)inImgCol);//��ԭͼ�������Ͻǵ�����ֵ��λ��
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
                    outData[i * domXSize + j] = 0;//����Ӱ��Χ��
                    errNum1++;
                }
                
            }
            else
            {
                outData[i * domXSize + j] = 0;//DEM��ֵ��
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

    //��������
    float str_per = 1;//����ٷֱ�
    int minThreshold = static_cast<int>(str_per/100* domXSize* domYSize);//�����ֵ���ص�λ��
    int maxThreshold = domXSize * domYSize- minThreshold;//�����ֵ���ص�λ��
    float* outDataSort = new float[domXSize * domYSize];//�����õ�����
    float minValue = *std::min_element(outData, outData + domXSize * domYSize);
    float maxValue = *std::max_element(outData, outData + domXSize * domYSize);
    std::copy(outData, outData + domXSize * domYSize, outDataSort);
    std::sort(outDataSort, outDataSort + domXSize * domYSize);
    unsigned short minPixThreshold = static_cast<unsigned short>(outDataSort[minThreshold]);//�����ֵ
    unsigned short maxPixThreshold = static_cast<unsigned short>(outDataSort[maxThreshold]);//�����ֵ
    delete[] outDataSort;

    int min = 0;
    int max = 0;
    //��������������ֵ������ȫ������
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

    //��������������
    double stretchFactor = 255 / (maxThreshold - minPixThreshold) * 0.8;
    unsigned char* outDataWrite = new unsigned char[domXSize * domYSize];//����д��Ӱ�������
    for (size_t i = 0; i < domXSize * domYSize; i++)
    {
        outDataWrite[i] = static_cast<unsigned char>((outData[i] - minPixThreshold) * stretchFactor);
    }

    delete[] outData;
    //���޸ĺ������д�ص�Ӱ���ļ��С�


    qDebug() << "liner compute over" << "\n";

    //�ͷ��ڴ沢�ر��ļ���      
    delete[] outDataWrite;
    GDALClose(outDataset);

    qDebug() << "compute over" << "\n";

    return a.exec();
}