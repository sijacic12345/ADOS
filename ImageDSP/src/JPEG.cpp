#include "JPEG.h"
#include "NxNDCT.h"
#include <math.h>

#include "JPEGBitStreamWriter.h"


#define DEBUG(x) do{ qDebug() << #x << " = " << x;}while(0)



// quantization tables from JPEG Standard, Annex K
uint8_t QuantLuminance[8*8] =
    { 16, 11, 10, 16, 24, 40, 51, 61,
      12, 12, 14, 19, 26, 58, 60, 55,
      14, 13, 16, 24, 40, 57, 69, 56,
      14, 17, 22, 29, 51, 87, 80, 62,
      18, 22, 37, 56, 68,109,103, 77,
      24, 35, 55, 64, 81,104,113, 92,
      49, 64, 78, 87,103,121,120,101,
      72, 92, 95, 98,112,100,103, 99 };
uint8_t QuantChrominance[8*8] =
    { 17, 18, 24, 47, 99, 99, 99, 99,
      18, 21, 26, 66, 99, 99, 99, 99,
      24, 26, 56, 99, 99, 99, 99, 99,
      47, 66, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99,
      99, 99, 99, 99, 99, 99, 99, 99 };

static char quantizationMatrix[64] =
{
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

static const int zigZagOrder[64]={
    0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};


struct imageProperties{
    int width;
    int height;
    int16_t* coeffs;
};


void DCTUandV(const char input[], int16_t output[], int N, double* DCTKernel)
{
    double* temp = new double[N*N];
    double* DCTCoefficients = new double[N*N];

    double sum;
    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + DCTKernel[i*N+k] * (input[k*N+j]);
            }
            temp[i*N + j] = sum;
        }
    }

    for (int i = 0; i <= N - 1; i++)
    {
        for (int j = 0; j <= N - 1; j++)
        {
            sum = 0;
            for (int k = 0; k <= N - 1; k++)
            {
                sum = sum + temp[i*N+k] * DCTKernel[j*N+k];
            }
            DCTCoefficients[i*N+j] = sum;
        }
    }

    for(int i = 0; i < N*N; i++)
    {
        output[i] = floor(DCTCoefficients[i]+0.5);
    }

    delete[] temp;
    delete[] DCTCoefficients;

    return;
}

uint8_t quantQuality(uint8_t quant, uint8_t quality) {
    // Convert to an internal JPEG quality factor, formula taken from libjpeg
    int16_t q = quality < 50 ? 5000 / quality : 200 - quality * 2;
    return clamp((quant * q + 50) / 100, 1, 255);
}

static void doZigZag(int16_t block[], uint8_t quantizationBlock[], int N, int DCTorQuantization)
{
    /* TO DO */

    int16_t temp[64];
    for(int i=0;i<64;i++){
        //kvantizacija: koeficijent/faktor kvantizacije
        temp[i]=(int16_t)round((double)block[zigZagOrder[i]]/quantizationBlock[zigZagOrder[i]]);
    }
    memcpy(block,temp,64*sizeof (int16_t));
}

/* perform DCT */
imageProperties performDCT(char input[], int xSize, int ySize, int N, uint8_t quality, bool quantType)
{
	// TO DO
}

//JPEGBitStreamWriter streamer("example.jpg");
void performJPEGEncoding(uchar Y_buff[], char U_buff[], char V_buff[], int xSize, int ySize, int quality)
{
	DEBUG(quality);
	
	
    auto s = new JPEGBitStreamWriter("example.jpg");
	// TO DO

    s->writeHeader();

    //priprema kvantizacionih tabela prema kvalitetu
    uint8_t qLuma[64],qChroma[64];
    for(int i=0;i<64;i++){
        qLuma[i]=quantQuality(QuantLuminance[i],quality);
        qChroma[i]=quantQuality(QuantChrominance[i],quality);
    }

    uint8_t qLumaZigZag[64],qChromaZigZag[64];
    for(int i=0;i<64;i++){
        qLumaZigZag[i]=qLuma[zigZagOrder[i]];
        qChromaZigZag[i]=qChroma[zigZagOrder[i]];
    }


    s->writeQuantizationTables(qLumaZigZag,qChromaZigZag);
    s->writeImageInfo(xSize,ySize);
    s->writeHuffmanTables();

    double kernel[64];
    GenerateDCTmatrix(kernel,8);

    //iteracija po MCU(Minimum Coded Unit) blokovima
    for(int j=0;j<ySize;j+=16){
        for(int i=0;i<xSize;i+=16){
            //Y komponente: 4 bloka 8x8
            int yOffsets[4][2]={{0,0},{8,0},{0,8},{8,8}};
            for(int b=0;b<4;b++){
                char blockData[64];
                int16_t dctData[64];
                //izdvajanje 8x8 bloka
                for(int row=0;row<8;row++){
                    for(int col=0;col<8;col++){
                        blockData[row*8+col]=(char)((int)Y_buff[(j+yOffsets[b][1]+row)*xSize+(i+yOffsets[b][0]+col)]-128);
                    }
                }
                DCT(blockData,dctData,8,kernel);
                doZigZag(dctData,qLuma,8,1);
                s->writeBlockY(dctData);
            }

            //U komponenta (jedan 8x8 blok za 16x16 Y blokova)
            char blockU[64];
            int16_t dctU[64];
            for(int row=0;row<8;row++){
                for(int col=0;col<8;col++){
                    blockU[row*8+col]=U_buff[(j/2+row)*(xSize/2)+(i/2+col)];
                }
            }
            DCTUandV(blockU,dctU,8,kernel);
            doZigZag(dctU,qChroma,8,1);
            s->writeBlockU(dctU);

            //V komponenta
            char blockV[64];
            int16_t dctV[64];
            for(int row=0;row<8;row++){
                for(int col=0;col<8;col++){
                    blockV[row*8+col]=V_buff[(j/2+row)*(xSize/2)+(i/2+col)];
                }
            }
            DCTUandV(blockV,dctV,8,kernel);
            doZigZag(dctV,qChroma,8,1);
            s->writeBlockV(dctV);
        }
    }
    s->finishStream();
    delete s;

}








