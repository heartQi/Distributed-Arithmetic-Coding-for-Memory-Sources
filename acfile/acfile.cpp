// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Simple file compression using arithmetic coding                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Version 1.00  -  April 25, 2004                                           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                                  WARNING                                  -
//                                 =========                                 -
//                                                                           -
// The only purpose of this program is to demonstrate the basic principles   -
// of arithmetic coding. It is provided as is, without any express or        -
// implied warranty, without even the warranty of fitness for any particular -
// purpose, or that the implementations are correct.                         -
//                                                                           -
// Permission to copy and redistribute this code is hereby granted, provided -
// that this warning and copyright notices are not removed or altered.       -
//                                                                           -
// Copyright (c) 2004 by Amir Said (said@ieee.org) &                         -
//                       William A. Pearlman (pearlw@ecse.rpi.edu)           -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <stdlib.h>
#include "tools.h"
#include "sconfig.h"
#include "Profiler.h"
#include "math.h"
#include "report.h"
#include <string.h>
#include "arithmetic_codec.h"
#include "time.h"
extern "C" {
#include "rand.h"
	double rand_uniform ();
}

// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

const char * W_MSG = "cannot write to file";
const char * R_MSG = "cannot read from file";

const unsigned NumModels  = 16;                          // MUST be power of 2

const unsigned FILE_ID    = 0xB8AA3B29U;

unsigned TERMINATION=15;
unsigned BufferSize = 1000;
unsigned BufferCount=10;
unsigned nodecount=2048;
unsigned STEP=10;
int block_size=0;
int overlap_count=0;
int max_node=0;

char reports[25];
char  sourcefile[20];
char  compressed[20];
char  decompressed[20];

char* inifilename="DAC.ini";
double overlap_input=0.43;
double max_cross=0.1;
double min_cross=0.0;
//probability of 0
double probability=0.5;

const int MarOrder = MarOrderone;//设置马尔科夫信源的阶数
double Marpro[4] = { 0.8,0.3,0.5,0.3 };//Markov 信源符号之间的转移概率,应传入pow(2,MarOrder)个概率值
unsigned Marbit[MarOrder] = { 0 };//Markov信源前MarOrder个元素的bit值
void GenerateSource();
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Prototypes  - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_File(char * data_file_name,
				 char * code_file_name);

void Decode_File(char * code_file_name,
				 char * data_file_name);
void GenerateSide(char* side,char* source,double cross_probability);
void iniconfig(char model); 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Main function - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//原函数没有修改
int main(int numb_arg, char * arg[])
{
	
	iniconfig(arg[1][1]);
	printf("MultiMarkovePycx%sN%dT%d\n", sourcefile, BufferSize, TERMINATION);
	if (arg[1][1] == 'c')
	{
		GenerateSource();
		Encode_File(sourcefile, compressed);
	}
	else
	{
		Decode_File(compressed, decompressed);
	}
	report_stop(reports);
	return 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Implementations - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GenerateSource()
{
	const int N = 999;
	unsigned char zero = 0;
	unsigned char one = 1;
	double temprobability = 0;
	FILE * new_file = fopen(sourcefile, "wb");
	if (new_file == 0)
		printf("cannot open input file");
	double tempnum;
	srand((unsigned)time(NULL));
	unsigned char tempachar = 0;
	for (int j = 0; j < BufferCount; j++)
	{
		for (int i = 0; i < BufferSize; i++)
		{
			int tempMarproindex = 0;

			//统计需要选取马尔可夫信源的第几个概率
			for (int k = 0; k <MarOrder; k++)
			{
				tempMarproindex += Marbit[k] * (int)pow(2, k);
			}
			temprobability = Marpro[tempMarproindex];//设定概率

			tempnum = rand() % (N + 1) / (double)(N + 1);	//生成随机数

															//改变后MarOrder-1的Marbit
			for (int w = MarOrder - 1; w >0; w--)
			{
				Marbit[w] = Marbit[w - 1];
			}

			///////写入文件
			if (tempnum > temprobability)
			{
				fwrite(&one, sizeof(zero), 1, new_file);
				Marbit[0] = 1;//改变第1个的Marbit
			}
			else
			{
				fwrite(&zero, sizeof(zero), 1, new_file);
				Marbit[0] = 0;//改变第1个的Marbit
			}
		}
		for (int i = 0; i < MarOrder; i++)
		{
			Marbit[i] = 0;
		}
	}
	fclose(new_file);
}
void Error(const char * s)
{
	fprintf(stderr, "\n Error: %s.\n\n", s);
	exit(1);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Buffer_CRC(unsigned bytes,
					unsigned char * buffer)
{
	static const unsigned CRC_Gen[8] = {        // data for generating CRC table
		0xEC1A5A3EU, 0x5975F5D7U, 0xB2EBEBAEU, 0xE49696F7U,
		0x486C6C45U, 0x90D8D88AU, 0xA0F0F0BFU, 0xC0A0A0D5U };

		static unsigned CRC_Table[256];            // table for fast CRC computation

		if (CRC_Table[1] == 0)                                      // compute table
			for (unsigned k = CRC_Table[0] = 0; k < 8; k++) {
				unsigned s = 1 << k, g = CRC_Gen[k];
				for (unsigned n = 0; n < s; n++) CRC_Table[n+s] = CRC_Table[n] ^ g;
			}

			// computes buffer's cyclic redundancy check
			unsigned crc = 0;
			if (bytes)
				do {
					crc = (crc >> 8) ^ CRC_Table[(crc&0xFFU)^unsigned(*buffer++)];
				} while (--bytes);
			return crc;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FILE * Open_Input_File(char * file_name)
{
	FILE * new_file = fopen(file_name, "rb");
	if (new_file == 0) Error("cannot open input file");
	return new_file;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

FILE * Open_Output_File(char * file_name)
{
	FILE * new_file = fopen(file_name, "wb");
	if (new_file == 0) Error("cannot open output file");
	return new_file;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Save_Number(unsigned n, unsigned char * b)
{                                                   // decompose 4-byte number
	b[0] = (unsigned char)( n        & 0xFFU);
	b[1] = (unsigned char)((n >>  8) & 0xFFU);
	b[2] = (unsigned char)((n >> 16) & 0xFFU);
	b[3] = (unsigned char)( n >> 24         );
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Recover_Number(unsigned char * b)
{                                                    // recover 4-byte integer
	return unsigned(b[0]) + (unsigned(b[1]) << 8) + 
		(unsigned(b[2]) << 16) + (unsigned(b[3]) << 24);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Encode_File(char * data_file_name,
				 char * code_file_name)
{
	// open files
	FILE * data_file = Open_Input_File(data_file_name);
	FILE * code_file = Open_Output_File(code_file_name);
	// buffer for data file data
	unsigned char * data = new unsigned char[BufferSize];
	unsigned nb, bytes = 0, crc = 0;       // compute CRC (cyclic check) of file
	bytes=BufferSize*BufferCount;
	for(int icount=0;icount<BufferCount;icount++) 
	{
		nb = fread(data, 1, BufferSize, data_file);
		crc ^= Buffer_CRC(nb, data);
	}
	// define 12-byte header
	unsigned char header[12];
	Save_Number(FILE_ID, header);
	Save_Number(crc,      header + 4);
	Save_Number(bytes,    header + 8);
	if (fwrite(header, 1, 12, code_file) != 12) Error(W_MSG);
	// set data models
	Static_Bit_Model dm;
	//dm.set_probability_0(probability);
	Arithmetic_Codec encoder(BufferSize);                  // set encoder buffer
	rewind(data_file);                               // second pass to code file
	CProfiler Timer;
	Timer.Start();
	do {
		nb = (bytes < BufferSize ? bytes : BufferSize);
		if (fread(data, 1, nb, data_file) != nb) Error(R_MSG);   // read file data
		encoder.start_encoder();
		//encoder.setoverlap(overlap_input,dm);
		for (unsigned p = 0; p < nb; p++)
		{
			int tempMarproindex = 0;
			//统计需要选取马尔可夫信源的第几个概率
			for (int k = 0; k <MarOrder; k++)
			{
				tempMarproindex += Marbit[k] * (int)pow(2, k);
			}
			probability = Marpro[tempMarproindex];//设定概率

			dm.set_probability_0(probability);
			// compress data
			if (p>=(nb-TERMINATION))
			{
				encoder.setoverlap(0.0,dm);
			}
			else
			{
				encoder.setoverlap(overlap_input, dm);
			}
			encoder.encode(data[p], dm);
			for (int w = MarOrder - 1; w >0; w--)
			{
				Marbit[w] = Marbit[w - 1];
			}
			Marbit[0] = data[p];
		}
		for (int index = 0; index < MarOrder; index++)
		{
			Marbit[index] = 0;
		}
		encoder.write_to_file(code_file);  // stop encoder & write compressed data
	} while (bytes -= nb);
	// done: close files
	double encoder_time=Timer.GetElapsedTime();
	report(encoder_time);

	fflush(code_file);
	unsigned data_bytes = ftell(data_file), code_bytes = ftell(code_file);
	printf(" Compressed file size = %d bytes (%6.6f:1 compression)\n",
		(code_bytes-12), double((code_bytes-12)*8)/double (data_bytes) );
	fclose(data_file);
	fclose(code_file);

	report(double((code_bytes-12)*8)/double (data_bytes));
	report(int(code_bytes-12)*8);
	report((int)data_bytes);

	report_newline();
	delete [] data;
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Decode_File(char * code_file_name,
				 char * data_file_name)
{
	FILE * send_file =Open_Input_File(sourcefile);
	char* source=(char*)malloc(sizeof(char)*BufferCount*BufferSize);
	if (fread(source, 1, BufferSize*BufferCount, send_file) != BufferSize*BufferCount) Error(R_MSG);
	fclose(send_file);
	char* tempsource=source;
	//	genarate nblock
	char* nblock=(char*)malloc(sizeof(char)*BufferSize*BufferCount);
	//malloc node
	Node** tree=(Node**)malloc(sizeof(Node*)*(BufferSize+1));
	for(int ilen=0;ilen<(BufferSize+1);ilen++)
	{
		tree[ilen]=(Node*)malloc(sizeof(Node)*max_node);
	}
	double cross_probability=0.10;
	// buffer for data file data
	char * data = new char[BufferSize];
	
	for(int icross=0;icross<STEP;icross++)
	{
		tempsource=source;
		// open files
		FILE * code_file = Open_Input_File(code_file_name);
		FILE * data_file  = Open_Output_File(data_file_name);

		cross_probability=double(icross)*(max_cross-min_cross)/STEP+min_cross;
		int decode_error=0;
		// read file information from 12-byte header
		unsigned char header[12];
		if (fread(header, 1, 12, code_file) != 12) Error(R_MSG);
		unsigned fid   = Recover_Number(header);
		unsigned crc   = Recover_Number(header + 4);
		unsigned bytes = Recover_Number(header + 8);

		if (fid != FILE_ID) Error("invalid compressed file");
		// set data models
		GenerateSide(nblock,tempsource,cross_probability);
		Static_Bit_Model dm; 
		//dm.set_probability_0(probability);
		Arithmetic_Codec decoder(BufferSize);// set encoder buffer
		//decoder.setdecoder(nodecount,cross_probability,probability,nblock,TERMINATION);
		decoder.setdecoder(nodecount, cross_probability, nblock, TERMINATION, Marpro);
		decoder.setblock(block_size, max_node);

		CProfiler Timer;
		Timer.Start();

		unsigned nb=0; // decompress file
		for(int icount=0;icount<BufferCount;icount++)
		{
			decoder.read_from_file(code_file); // read compressed data & start decoder
			nb = (bytes < BufferSize ? bytes : BufferSize);

			int tempMarproindex = 0;
			for (int k = 0; k <MarOrder; k++)
			{
				tempMarproindex += Marbit[k] * (int)pow(2, k);
			}
			probability = Marpro[tempMarproindex];//设定概率
			//probability = Marpro[0];
			dm.set_probability_0(probability);
			decoder.setoverlap(overlap_input,dm);//主要为了初始ovelap

			// decompress data
			decoder.decode(tree,dm,Marpro);
			decoder.stop_decoder();

			//save the decode data
			Node temp=tree[BufferSize][0];
			data[BufferSize-1]=temp.bit;
			for(int ilen=BufferSize-1;ilen>0;ilen--)
			{
				temp=*temp.parent;
				data[ilen-1]=temp.bit;
			}
			decode_error=bitdiff(data,tempsource,nb)+decode_error;
			tempsource+=nb;
			decoder.nextnblock(nb);
		}
		double decode_time=Timer.GetElapsedTime();
		double hxy=-(cross_probability*log(cross_probability)+(1-cross_probability)*log(1-cross_probability)+probability*log(probability)+(1-probability)*log(1-probability))/log(2.0);
		report(decode_time);
		report(cross_probability);
		report(hxy);
		report(decode_error);
		report((double)decode_error/double(BufferSize*BufferCount));
		printf("Decoding error %lf\n",(double)decode_error/double(BufferSize*BufferCount));

		report_stop(reports);
		report_start(reports);

		fclose(data_file);                                     // done: close files
		fclose(code_file);
	}
	delete [] data;
	
}
void GenerateSide(char* side,char* source,double cross_probability)
{
	for(int icount=0;icount<BufferSize*BufferCount;icount++)
	{
		if(rand_uniform() < cross_probability)
		{
			if(source[icount]==0)
			{
				side[icount]=1;
			}
			else
			{
				side[icount]=0;
			}
		}
		else
		{
			side[icount]=source[icount];
		}
	}
}

void iniconfig(char model)
{
	sc_read_int(inifilename, "BufferSize", (int*)&BufferSize);
	sc_read_int(inifilename, "BufferCount", (int*)&BufferCount);
	sc_read_int(inifilename, "TERMINATION", (int*)&TERMINATION);
	sc_read_double(inifilename, "probability", &probability);
	sc_read_double(inifilename, "overlap_input", &overlap_input);
	sc_read(inifilename, "reports", reports,20);
	sc_read(inifilename, "sourcefile", sourcefile,20);
	//sc_read_double(inifilename, "Marproone",&Marpro[0]);
	//sc_read_double(inifilename, "Marprotwo",&Marpro[1]);
	strcpy(compressed, sourcefile);
	strcat(compressed,".en");
	//write reports
	report_start(reports);
	
	if(model=='c')
	{
		time_t t = time(0); 
		char tmp[128]; 
		strftime( tmp, sizeof(tmp), "DAC Encoder at %Y/%m/%d %X %A 本年第%j天 %z",localtime(&t)); 
		report("------------------------------------------------");
		report_newline();
		report(tmp);
		report_newline();
		report("------------------------------------------------");
		report_newline();

		report("BufferSize(bit)");
		report("BufferCount");
		report("probability");
		report("overlap_input");
		report("TERMINATION");
		report("Encode Time");
		report("Compression Ratio");
		report("Compressed Size(bit)");
		report("Source Size(bit)");
		report_newline();

		report((int)BufferSize);
		report((int)BufferCount);
		report(probability);
		report(overlap_input);
		report((int)TERMINATION);		
	}
	else
	{
		sc_read_int(inifilename, "nodecount", (int*)&nodecount);
		sc_read_int(inifilename, "STEP", (int*)&STEP);
		sc_read_int(inifilename, "block_size", (int*)&block_size);
		sc_read_int(inifilename, "overlap_count", (int*)&overlap_count);
		sc_read_double(inifilename, "max_cross", &max_cross);
		sc_read_double(inifilename, "min_cross", &min_cross);
		max_node=(int)pow(2.0,overlap_count)*nodecount*1.5;//此处可能对解码性能有影响

		strcpy(decompressed, sourcefile);
		strcat(decompressed,".de");
		
		time_t t = time(0); 
		char tmp[128]; 
		strftime( tmp, sizeof(tmp), "DAC Decoder at %Y/%m/%d %X %A 本年第%j天 %z",localtime(&t)); 
		report("------------------------------------------------");
		report_newline();
		report(tmp);
		report_newline();
		report("------------------------------------------------");
		report_newline();

		report("BufferSize(bit)");
		report("BufferCount");
		report("probability");
		report("overlap_input");
		report("TERMINATION");
		report("nodecount");
		report("STEP");
		report("max_cross");
		report("min_cross");
		report_newline();

		report((int)BufferSize);
		report((int)BufferCount);
		report(probability);
		report(overlap_input);
		report((int)TERMINATION);
		report((int)nodecount);
		report((int)STEP);
		report(max_cross);
		report(min_cross);
		report_newline();

		report("Decode Time");
		report("Source Error");
		report("H(X,Y)");
		report("Error Count");
		report("Error Ratio");
		report_newline();
	}
}
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
