
#include "ImageProcessing.h"
#include "ColorSpaces.h"
#include "JPEG.h"

#include <cmath>

#include <QDebug>
#include <QString>
#include <QImage>

void imageProcessingFun(const QString& progName, QImage& outImgs, const QImage& inImgs, const QVector<double>& params)
{
	// TO DO
    int width=inImgs.width();
    int height=inImgs.height();

    //prosirivanje na deljivost sa 16
    int X_SIZE=width+(width%16==0 ? 0 : 16-(width%16));
    int Y_SIZE=height+(height%16==0 ? 0 : 16-(height%16));

	/* Create buffers for YUV image */
    uchar* Y_buff=new uchar[X_SIZE*Y_SIZE];
    char* U_buff=new char[(X_SIZE/2)*(Y_SIZE/2)];
    char* V_buff=new char[(X_SIZE/2)*(Y_SIZE/2)];


	/* Create empty output image */
    outImgs = QImage(inImgs.width(), inImgs.height(), inImgs.format());

	/* Convert input image to YUV420 image */
    QImage paddedImg=inImgs.scaled(X_SIZE,Y_SIZE);
    RGBtoYUV420(paddedImg.bits(),X_SIZE,Y_SIZE,Y_buff,U_buff,V_buff);

    if(progName == QString("JPEG Encoder"))
	{	
        qDebug()<<"Pre pozivanja";
		/* Perform NxN DCT */
        //double quality=params.isEmpty() ? 50.0 : params[0];
        performJPEGEncoding(Y_buff, U_buff, V_buff, X_SIZE, Y_SIZE, params[0]);

        outImgs.load("example.jpg");
    }
    else{
        /* Convert YUV image back to RGB */
        YUV420toRGB(Y_buff, U_buff, V_buff, X_SIZE, Y_SIZE, paddedImg.bits());
        outImgs=paddedImg.copy(0,0,width,height);
    }


	/* Delete used memory buffers */
    delete[] Y_buff;
    delete[] U_buff;
    delete[] V_buff;

}

