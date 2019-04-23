// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
//                       ****************************                        -
//                        ARITHMETIC CODING EXAMPLES                         -
//                       ****************************                        -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//                                                                           -
// Fast arithmetic coding implementation                                     -
// -> double-precision floating-point arithmetic                             -
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
//                                                                           -
// A description of the arithmetic coding method used here is available in   -
//                                                                           -
// Lossless Compression Handbook, ed. K. Sayood                              -
// Chapter 5: Arithmetic Coding (A. Said), pp. 101-152, Academic Press, 2003 -
//                                                                           -
// A. Said, Introduction to Arithetic Coding Theory and Practice             -
// HP Labs report HPL-2004-76  -  http://www.hpl.hp.com/techreports/         -
//                                                                           -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -


// - - Inclusion - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

#include <math.h>
#include <stdlib.h>
#include "arithmetic_codec.h"



// - - Constants - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

//sort tree;
inline int sort(Node* decodetree,int maxcount);
inline int cmp( const void *a ,const void *b) ;
inline int sort(Node* decodetree,int maxcount)
{
	qsort(decodetree,maxcount,sizeof(decodetree[0]),cmp);
	return maxcount;
}
inline int cmp( const void *a ,const void *b) 
{ 
	return ((*(Node *)a).weight < (*(Node *)b).weight ? 1 : -1);
} 


// This encoder saves data 2 bytes a time: renormalization factor = 2^16
const double AC__OutputFactor   = double(0x10000);                 // D = 2^16
const double AC__MinLength      = 1.0 / double(0x10000);        // 1/D = 2^-16
const double AC__LeastSignifBit = 1.0 / double(0x1000000) / double(0x1000000);
const double AC__Leakage        = 2.0 * AC__LeastSignifBit;

const unsigned BM__MaxCount  = 1 << 14;     // Maximum bit count for Bit_Model
const unsigned DM__MaxCount  = 1 << 17; // Maximum symbol count for Data_Model

const double MIN_Nagtive=- double(0x1000000) * double(0x1000000)* double(0x1000000);

double cross_probability=0.1;
unsigned NODECOUNT=2048;
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static functions  - - - - - - - - - - - - - - - - - - - - - - - - - - -
// 堆化，保持堆的性质
// MaxHeapify让a[i]在最大堆中"下降"，
// 使以i为根的子树成为最大堆
void MaxHeapify(Node *a, int i, int size)
{
	int lt = 2*i, rt = 2*i+1;
	int largest;
	if(lt <= size-1 && a[lt].current_weight > a[i].current_weight)
		largest = lt;
	else
		largest = i;
	if(rt <= size-1 && a[rt].current_weight > a[largest].current_weight)
		largest = rt;
	if(largest != i)
	{
		Node temp = a[i];
		a[i] = a[largest];
		a[largest] = temp;
		MaxHeapify(a, largest, size);
	}
} 
// 堆排序
// 初始调用BuildMaxHeap将a[1..size]变成最大堆
// 因为数组最大元素在a[1]，则可以通过将a[1]与a[size]互换达到正确位置
// 现在新的根元素破坏了最大堆的性质，所以调用MaxHeapify调整，
// 使a[1..size-1]成为最大堆，a[1]又是a[1..size-1]中的最大元素，
// 将a[1]与a[size-1]互换达到正确位置。
// 反复调用Heapify，使整个数组成从小到大排序。
// 注意： 交换只是破坏了以a[1]为根的二叉树最大堆性质，它的左右子二叉树还是具备最大堆性质。
//        这也是为何在BuildMaxHeap时需要遍历size/2到1的结点才能构成最大堆，而这里只需要堆化a[1]即可。
void HeapSort(Node *a, int size)
{
	// 建堆
// 自底而上地调用MaxHeapify来将一个数组a[1..size]变成一个最大堆
//
	for(int i=size/2-1; i>=0; --i)
		MaxHeapify(a, i, size);
 
	int len = size;
	for(int i=size-1; i>=1; --i)
	{
		Node temp = a[0];
		a[0] = a[i];
		a[i] = temp;
		len--;
		MaxHeapify(a, 0, len);
	}
 
}
static void AC_Error(const char * msg)
{
	fprintf(stderr, "\n\n -> Arithmetic coding error: ");
	fputs(msg, stderr);
	fputs("\n Execution terminated!\n", stderr);
	exit(1);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Coding implementations  - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::propagate_carry(void)
{
	base -= 1.0;                  // carry propagation on compressed data buffer
	unsigned char * p;
	for (p = ac_pointer - 1; *p == 0xFFU; p--) *p = 0;
	++*p;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_enc_interval(void)
{
	// rescale arithmetic encoder interval, output data bytes
	do {
		unsigned a = unsigned(base *= AC__OutputFactor);
		*ac_pointer++ = (unsigned char) (a >> 8);
		*ac_pointer++ = (unsigned char) (a & 0xFF);
		base -= double(a);
	} while ((length *= AC__OutputFactor) <= AC__MinLength);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

inline void Arithmetic_Codec::renorm_dec_interval(void)
{
	// rescale arithmetic decoder interval, input data bytes
	do {
		unsigned a = unsigned(base *= AC__OutputFactor);
		base -= double(a);
		value = (AC__OutputFactor * value - double(a)) + AC__LeastSignifBit *
			(256.0 * double(ac_pointer[0]) + double(ac_pointer[1]));
		ac_pointer += 2;
	} while ((length *= AC__OutputFactor) <= AC__MinLength);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bit(unsigned bit)
{
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
#endif
	// encode assuming p0 = p1 = 1/2
	double x = 0.5 * length;                             // compute middle point

	if (bit == 0)                             // update interval base and length 
		length  = x;
	else {
		base   += x;
		length -= x;
		if (base >= 1.0) propagate_carry();                  // check if carry bit
	}

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bit(void)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
#endif

	double x = 0.5 * length;                    // compute interval middle point
	unsigned bit = (value + AC__LeastSignifBit >= base + x); // decode bit value

	if (bit == 0)                             // update interval base and length 
		length  = x;
	else {
		base   += x;
		length -= x;
		if (base >= 1.0) {                     // decoder does not propagate carry
			base   -= 1.0;                                         // shift interval
			value  -= 1.0;
		}
	}

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_dec_interval();

	return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::put_bits(unsigned data,
								unsigned bits)
{
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
	if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
	if (data >= (1U << bits)) AC_Error("invalid data");
#endif

	unsigned symbols = 1U << bits;                              // alphabet size
	double y, d = length / symbols;               // assume uniform distribution
	// compute products
	if (data == symbols - 1)
		y = base + length;                            // avoid multiplication by 1
	else
		y = base + d * (data + 1);
	// set new interval
	base  += d * data;
	length = y - base;

	if (base >= 1.0) propagate_carry();                    // check if carry bit

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::get_bits(unsigned bits)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
	if ((bits < 1) || (bits > 20)) AC_Error("invalid number of bits");
#endif

	unsigned s = 0, n = 1U << bits, m = n >> 1;
	double x = base, y = base + length, d = length / n;
	double shifted_value = value + AC__LeastSignifBit;

	// bissection search of index in arithmetic coding interval
	do {
		double z = base + d * m;
		if (z > shifted_value) {
			y = z;                                          // code value is smaller
			n = m;
		}
		else {
			x = z;                                  // code value is larger or equal
			s = m;
		}
	} while ((m = (s + n) >> 1) != s);
	// set new interval
	base   = x;
	length = y - x;

	if (base >= 1.0) {                       // decoder does not propagate carry
		base   -= 1.0;                                           // shift interval
		value  -= 1.0;
	}

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_dec_interval();

	return s;
}

void Arithmetic_Codec::setoverlap(double overlap,Static_Bit_Model & M)
{
		Arithmetic_Codec::overlap=overlap;
		Arithmetic_Codec::pow_0=pow(M.bit_0_prob,1-overlap);
		Arithmetic_Codec::pow_1=pow(1-M.bit_0_prob,1-overlap);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
							  Static_Bit_Model & M)
{
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
#endif

	double x = length *pow_0;// compute product l x p0

	if (bit == 0)                             // update interval base and length 
		length  = x;
	else {
		base   += (length*(1-pow_1));
		length -= (length*(1-pow_1));
		if (base >= 1.0) propagate_carry();                  // check if carry bit
	}
	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Bit_Model & M)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
#endif

	double x = length *( M.bit_0_prob+overlap*(1-M.bit_0_prob));         // compute interval-division point
	unsigned bit ;
	if(value + AC__LeastSignifBit <=base + x); // decode bit value
	{
		bit=0;
	}
	if(value + AC__LeastSignifBit >=base + x+length*overlap)
	{
		bit=1;
	}
	if(((value + AC__LeastSignifBit) <(base + x+length*overlap))&&((value + AC__LeastSignifBit )>(base + x)))
	{
		bit=3;//ambiguous
	}

	if (bit == 0)                             // update interval base and length 
	{
		length  = x;
	}
	else if(bit==1)
	{
		base   += (x-length*overlap);
		length -= (x-length*overlap);
		if (base >= 1.0) 
		{                     // decoder does not proplagate carry
			base   -= 1.0;                                         // shift interval
			value  -= 1.0;
		}
	}
	else if(bit==3)
	{
		//new node
		//add 0 node
	}
	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_dec_interval();
	return bit;                                         // return data bit value
}

//decode block data
void Arithmetic_Codec::setdecoder(unsigned nodecount,double error,char* nblock,unsigned termination,const double *Marpro)
{
	TERMINATION=termination;
	NODECOUNT=nodecount;
	Arithmetic_Codec::nblock=nblock;
	double weight=0;
	weight_0to1=log(error);//+log(p0);
	weight_0to0=log(1-error);//+log(p0);
	weight_1to1=log(1-error);//+log(1-p0);
	weight_1to0=log(error);//+log(1-p0);

	//weight=log(0.5);//log(p0)+(1-overlap)*log(1-p0)-log(p0*pow(1-p0,1-overlap)+(1-p0)*pow(p0,1-overlap));
	weight_ab_0to1=log(error);//+(weight);//+log(p0);
	weight_ab_0to0=log(1-error);//+(weight);//+log(p0);

	//weight=log(0.5);//log(1-p0)+(1-overlap)*log(p0)-log(p0*pow(1-p0,1-overlap)+(1-p0)*pow(p0,1-overlap));
	weight_ab_1to1=log(1-error);//+(weight);//+log(1-p0);
	weight_ab_1to0=log(error);//+(weight);//+log(1-p0);


	//weight_ab_0to1=log(error);
	//weight_ab_0to0=log(1-error);
	//weight_ab_1to1=log(1-error);
	//weight_ab_1to0=log(error);
	Arithmetic_Codec::error=error;
	Arithmetic_Codec::p0= Marpro[0];
}
void Arithmetic_Codec::setweight()
{}
void Arithmetic_Codec::setblock( int block_size,int max_node)
{
	Arithmetic_Codec::block_size=block_size;
	Arithmetic_Codec::max_node=max_node;
}
void Arithmetic_Codec::nextnblock(unsigned count)
{
	nblock+=count;
}
void Arithmetic_Codec::decode(Node** node, Static_Bit_Model &M, const double *Marpro)
{
	int icol=0,irow=0;
	
	//标记未处理的
	for(irow=0;irow<buffer_size+1;irow++)
	{
		for(icol=0;icol<max_node;icol++)
		{
			node[irow][icol].ac_pointer=NULL;
			node[irow][icol].length=-1.0;
			node[irow][icol].base=-1.0;
			node[irow][icol].bit=0;
			node[irow][icol].parent=NULL;
			node[irow][icol].value=-1.0;
			node[irow][icol].weight=0;
			node[irow][icol].current_weight=0;
			for (int index = 0; index < MarOrderone; index++)
			{
				node[irow][icol].Marbit[index] = { 0 };
			}	
		}
	}
	int newnodecounter=0;
	double probability;//新增加代码
	//initial first node
	node[0][0].base=base;
	node[0][0].ac_pointer=ac_pointer;
	node[0][0].length=length;
	node[0][0].parent=NULL;
	node[0][0].value=value;
	node[0][0].weight=0;
	Node* tempnode=(Node*)malloc(sizeof(Node)*(max_node));
	int tailbit=block_size-1;
	int isave=0;
	
	for(irow=0;irow<buffer_size;irow++)
	{
		//initial all temp node
		if((irow)==(buffer_size-TERMINATION))
		{
			overlap=0.0;
			//Arithmetic_Codec::setoverlap(overlap,M);
		}
		newnodecounter=0;
		for(icol=0;icol<max_node;icol++)
		{
			if(node[irow][icol].value==-1.0)
			{
				break;
			}
			//统计需要选取马尔可夫信源的第几个转移概率
			int tempMarproindex = 0;
			for (int k = 0; k <MarOrderone; k++)
			{
				tempMarproindex += node[irow][icol].Marbit[k] * (int)pow(2, k);
			}
			probability = Marpro[tempMarproindex];//设定概率
			M.set_probability_0(probability);//新增加
			Arithmetic_Codec::setoverlap(overlap, M);//新增加
													 
			double x =node[irow][icol].length * pow_0;         // compute interval-division point
			unsigned bit ;
			if((node[irow][icol].value + AC__LeastSignifBit) <=(node[irow][icol].base + node[irow][icol].length*(1-pow_1))) // decode bit value
			{
				bit=0;
			}
			else if((node[irow][icol].value + AC__LeastSignifBit) >=(node[irow][icol].base + x))
			{
				bit=1;
			}
			else if(((node[irow][icol].value + AC__LeastSignifBit) >(node[irow][icol].base + node[irow][icol].length*(1-pow_1))
				&&((node[irow][icol].value + AC__LeastSignifBit) <(node[irow][icol].base + x))))
			{
				bit=3;//ambiguous
			}

			if (bit == 0)                             // update interval base and length 
			{
				if(nblock[irow]==0)
				{
					tempnode[newnodecounter].weight = node[irow][icol].weight + weight_0to0 ;
				}
				else
				{
					tempnode[newnodecounter].weight = node[irow][icol].weight + weight_0to1 ;
				}
				tempnode[newnodecounter].current_weight=tempnode[newnodecounter].weight;
				tempnode[newnodecounter].ac_pointer=node[irow][icol].ac_pointer;
				tempnode[newnodecounter].base=node[irow][icol].base;
				tempnode[newnodecounter].length = x;
				tempnode[newnodecounter].bit=bit;
				tempnode[newnodecounter].parent=&node[irow][icol];
				tempnode[newnodecounter].value=node[irow][icol].value;

				////设置Marbit(新增加代码）
				for (int index = MarOrderone - 1; index >0; index--)
				{
					tempnode[newnodecounter].Marbit[index] = node[irow][icol].Marbit[index - 1];
				}
				tempnode[newnodecounter].Marbit[0] = tempnode[newnodecounter].bit;

				// if renormalization time
				if ((tempnode[newnodecounter].length -= AC__Leakage) <= AC__MinLength)     
				{
					// rescale arithmetic decoder interval, input data bytes
					do {
						unsigned a = unsigned(tempnode[newnodecounter].base *= AC__OutputFactor);
						tempnode[newnodecounter].base -= double(a);
						tempnode[newnodecounter].value = (AC__OutputFactor * tempnode[newnodecounter].value - double(a)) + AC__LeastSignifBit *
							(256.0 * double(tempnode[newnodecounter].ac_pointer[0]) + double(tempnode[newnodecounter].ac_pointer[1]));
						tempnode[newnodecounter].ac_pointer += 2;
					} while ((tempnode[newnodecounter].length *= AC__OutputFactor) <= AC__MinLength);
				}
				newnodecounter++;
			}
			else if(bit==1)
			{
				//must initial 
				tempnode[newnodecounter].base=node[irow][icol].base;
				tempnode[newnodecounter].length=node[irow][icol].length;
				tempnode[newnodecounter].base+= (node[irow][icol].length*(1-pow_1));
				tempnode[newnodecounter].length -= (node[irow][icol].length*(1-pow_1));

				if (nblock[irow] == 1)
				{
						tempnode[newnodecounter].weight = node[irow][icol].weight + weight_1to1;
				}
				else
				{
						tempnode[newnodecounter].weight = node[irow][icol].weight + weight_1to0;
				}
				tempnode[newnodecounter].value = node[irow][icol].value;
				tempnode[newnodecounter].ac_pointer = node[irow][icol].ac_pointer;
				tempnode[newnodecounter].bit = bit;
				tempnode[newnodecounter].parent = &node[irow][icol];
				tempnode[newnodecounter].current_weight=tempnode[newnodecounter].weight;

				////设置Marbit(新增加代码）
				for (int index = MarOrderone - 1; index >0; index--)
				{
					tempnode[newnodecounter].Marbit[index] = node[irow][icol].Marbit[index - 1];
				}
				tempnode[newnodecounter].Marbit[0] = tempnode[newnodecounter].bit;

				if (tempnode[newnodecounter].base >= 1.0) 
				{  
					// decoder does not proplagate carry
					tempnode[newnodecounter].base   -= 1.0;                                         // shift interval
					tempnode[newnodecounter].value  -= 1.0;
				}
				// if renormalization time
				if ((tempnode[newnodecounter].length -= AC__Leakage) <= AC__MinLength)     
				{
					// rescale arithmetic decoder interval, input data bytes
					do {
						unsigned a = unsigned(tempnode[newnodecounter].base *= AC__OutputFactor);
						tempnode[newnodecounter].base -= double(a);
						tempnode[newnodecounter].value = (AC__OutputFactor * tempnode[newnodecounter].value - double(a)) + AC__LeastSignifBit *
							(256.0 * double(tempnode[newnodecounter].ac_pointer[0]) + double(tempnode[newnodecounter].ac_pointer[1]));
						tempnode[newnodecounter].ac_pointer += 2;
					} while ((tempnode[newnodecounter].length *= AC__OutputFactor) <= AC__MinLength);
				}
				newnodecounter++;
			}
			else if(bit==3)
			{
				//two new node
				//add 0 node                                       
				bit=0;
				//must initial 
				tempnode[newnodecounter].base=node[irow][icol].base;
				//tempnode[newnodecounter].length=node[irow][icol].length;
				tempnode[newnodecounter].length  = x;

				if (nblock[irow] == 0)
				{
						tempnode[newnodecounter].weight = node[irow][icol].weight + weight_ab_0to0;
				}
				else
				{
						tempnode[newnodecounter].weight = node[irow][icol].weight + weight_ab_0to1;
				}
				tempnode[newnodecounter].current_weight=tempnode[newnodecounter].weight;
				tempnode[newnodecounter].ac_pointer=node[irow][icol].ac_pointer;
				tempnode[newnodecounter].base=node[irow][icol].base;
				tempnode[newnodecounter].bit=bit;
				tempnode[newnodecounter].parent=&node[irow][icol];
				tempnode[newnodecounter].value=node[irow][icol].value;

				////设置Marbit(新增加代码）
				for (int index = MarOrderone - 1; index >0; index--)
				{
					tempnode[newnodecounter].Marbit[index] = node[irow][icol].Marbit[index - 1];
				}
				tempnode[newnodecounter].Marbit[0] = tempnode[newnodecounter].bit;

				// if renormalization time
				if ((tempnode[newnodecounter].length -= AC__Leakage) <= AC__MinLength)     
				{
					// rescale arithmetic decoder interval, input data bytes
					do {
						unsigned a = unsigned(tempnode[newnodecounter].base *= AC__OutputFactor);
						tempnode[newnodecounter].base -= double(a);
						tempnode[newnodecounter].value = (AC__OutputFactor * tempnode[newnodecounter].value - double(a)) + AC__LeastSignifBit *
							(256.0 * double(tempnode[newnodecounter].ac_pointer[0]) + double(tempnode[newnodecounter].ac_pointer[1]));
						tempnode[newnodecounter].ac_pointer += 2;
					} while ((tempnode[newnodecounter].length *= AC__OutputFactor) <= AC__MinLength);
				}
				newnodecounter++;

				bit=1;
				//must initial 
				tempnode[newnodecounter].base=node[irow][icol].base;
				tempnode[newnodecounter].length=node[irow][icol].length;

				if (nblock[irow] == 1)
				{
					tempnode[newnodecounter].weight = node[irow][icol].weight + weight_ab_1to1;
				}
				else
				{
					tempnode[newnodecounter].weight = node[irow][icol].weight + weight_ab_1to0;
				}
				//add 1 node
				tempnode[newnodecounter].base += (node[irow][icol].length*(1 - pow_1));
				tempnode[newnodecounter].length -= (node[irow][icol].length*(1 - pow_1));
				tempnode[newnodecounter].value = node[irow][icol].value;
				tempnode[newnodecounter].ac_pointer = node[irow][icol].ac_pointer;
				tempnode[newnodecounter].bit = bit;
				tempnode[newnodecounter].parent = &node[irow][icol];
				tempnode[newnodecounter].current_weight=tempnode[newnodecounter].weight;

				////设置Marbit(新增加代码）
				for (int index = MarOrderone - 1; index >0; index--)
				{
					tempnode[newnodecounter].Marbit[index] = node[irow][icol].Marbit[index - 1];
				}
				tempnode[newnodecounter].Marbit[0] = tempnode[newnodecounter].bit;

					if (tempnode[newnodecounter].base >= 1.0) 
				{	// decoder does not proplagate carry
					tempnode[newnodecounter].base   -= 1.0;                                         // shift interval
					tempnode[newnodecounter].value  -= 1.0;
				}
				// if renormalization time
				if ((tempnode[newnodecounter].length -= AC__Leakage) <= AC__MinLength)     
				{
					// rescale arithmetic decoder interval, input data bytes
					do {
						unsigned a = unsigned(tempnode[newnodecounter].base *= AC__OutputFactor);
						tempnode[newnodecounter].base -= double(a);
						tempnode[newnodecounter].value = (AC__OutputFactor * tempnode[newnodecounter].value - double(a)) + AC__LeastSignifBit *
							(256.0 * double(tempnode[newnodecounter].ac_pointer[0]) + double(tempnode[newnodecounter].ac_pointer[1]));
						tempnode[newnodecounter].ac_pointer += 2;
					} while ((tempnode[newnodecounter].length *= AC__OutputFactor) <= AC__MinLength);
				}
				newnodecounter++;
			}
			if(newnodecounter>(max_node-2))
				break;
		}
		if(tailbit==(irow%block_size)||(irow)>=(buffer_size-TERMINATION))
		{
			isave=NODECOUNT;
		}
		else
		{
			isave=max_node;
			int nexrow=irow+1;
			for(int icount=0;icount<newnodecounter&&icount<isave;icount++)
			{
				node[nexrow][icount] =tempnode[icount];
			}
			continue;
		}
		for(int itempnode=newnodecounter;itempnode<max_node;itempnode++)
		{
			if(tempnode[itempnode].weight==MIN_Nagtive)
				break;
			tempnode[itempnode].weight=MIN_Nagtive;
		}
		icol=0;
		//堆排序，只取最大的NODECOUNT个
		if(newnodecounter>1)
		{
			for(int i=newnodecounter/2-1; i>=0; --i)
				MaxHeapify(tempnode, i, newnodecounter);
			int len = newnodecounter;
			int least=newnodecounter-isave;
			if(least<1)
			{
				least=1;
			}
			for(int i=newnodecounter-1; i>=least; --i)
			{
				Node temp = tempnode[0];
				tempnode[0] = tempnode[i];
				tempnode[i] = temp;

				//save most weight node
				if(temp.weight==MIN_Nagtive)
				{
					break;
				}
				node[irow+1][icol++] =temp;
				len--;
				MaxHeapify(tempnode, 0, len);
			}
			if(tempnode[0].weight!=MIN_Nagtive&&icol<isave)
			{
				node[irow+1][icol++] =tempnode[0];
			}
		}
		else
		{
			node[irow+1][icol++] =tempnode[0];
		}

	}
	free(tempnode);
}
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned bit,
							  Adaptive_Bit_Model & M)
{
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
#endif

	double x = length * M.bit_0_prob;                  // compute product l x p0

	if (bit == 0) {                           // update interval length and base
		length  = x;
		++M.bit_0_count;
	}
	else {
		base   += x;
		length -= x;
		if (base >= 1.0) propagate_carry();                  // check if carry bit
	}

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_enc_interval();

	if (--M.bits_until_update == 0) M.update();         // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Bit_Model & M)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
#endif

	double x = length * M.bit_0_prob;         // compute interval-division point
	unsigned bit = (value + AC__LeastSignifBit >= base + x); // decode bit value

	if (bit == 0) {                           // update interval length and base
		length  = x;
		++M.bit_0_count;
	}
	else {
		base   += x;
		length -= x;
		if (base >= 1.0) {                     // decoder does not propagate carry
			base   -= 1.0;                                         // shift interval
			value  -= 1.0;
		}
	}

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_dec_interval();

	if (--M.bits_until_update == 0) M.update();         // periodic model update

	return bit;                                         // return data bit value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
							  Static_Data_Model & M)
{
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
	if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

	double y;
	// compute products
	if (data == M.data_symbols - 1)
		y = base + length;                            // avoid multiplication by 1
	else
		y = base + length * M.distribution[data+1];
	// set new interval
	base  += length * M.distribution[data];
	length = y - base;

	if (base >= 1.0) propagate_carry();                    // check if carry bit

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_enc_interval();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Static_Data_Model & M)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
#endif

	unsigned s = 0, n = M.data_symbols, m = n >> 1;
	double x = base, y = base + length;
	double shifted_value = value + AC__LeastSignifBit;

	// bissection search of index in arithmetic coding interval
	do {
		double z = base + length * M.distribution[m];
		if (z > shifted_value) {
			y = z;                                          // code value is smaller
			n = m;
		}
		else {
			x = z;                                  // code value is larger or equal
			s = m;
		}
	} while ((m = (s + n) >> 1) != s);
	// set new interval
	base   = x;
	length = y - x;

	if (base >= 1.0) {                       // decoder does not propagate carry
		base   -= 1.0;                                           // shift interval
		value  -= 1.0;
	}

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_dec_interval();

	return s;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::encode(unsigned data,
							  Adaptive_Data_Model & M)
{
#ifdef _DEBUG
	if (mode != 1) AC_Error("encoder not initialized");
	if (data >= M.data_symbols) AC_Error("invalid data symbol");
#endif

	double y;
	// compute products
	if (data == M.data_symbols - 1)
		y = base + length;                            // avoid multiplication by 1
	else
		y = base + length * M.distribution[data+1];

	base  += length * M.distribution[data];  // set new interval base and length
	length = y - base;

	if (base >= 1.0) propagate_carry();                    // check if carry bit

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_enc_interval();

	++M.symbol_count[data];
	if (--M.symbols_until_update == 0) M.update();      // periodic model update
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::decode(Adaptive_Data_Model & M)
{
#ifdef _DEBUG
	if (mode != 2) AC_Error("decoder not initialized");
#endif

	unsigned s = 0, n = M.data_symbols, m = n >> 1;
	double x = base, y = base + length;
	double shifted_value = value + AC__LeastSignifBit;

	// bissection search of index in arithmetic coding interval
	do {
		double z = base + length * M.distribution[m];
		if (z > shifted_value) {
			y = z;                                          // code value is smaller
			n = m;
		}
		else {
			x = z;                                  // code value is larger or equal
			s = m;
		}
	} while ((m = (s + n) >> 1) != s);
	// set new interval
	base   = x;
	length = y - x;

	if (base >= 1.0) {                       // decoder does not propagate carry
		base   -= 1.0;                                           // shift interval
		value  -= 1.0;
	}

	if ((length -= AC__Leakage) <= AC__MinLength)     // if renormalization time
		renorm_dec_interval();

	++M.symbol_count[s];
	if (--M.symbols_until_update == 0) M.update();      // periodic model update

	return s;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Other Arithmetic_Codec implementations  - - - - - - - - - - - - - - - -

Arithmetic_Codec::Arithmetic_Codec(void)                       // constructors
{
	mode = buffer_size = 0;
	new_buffer = code_buffer = 0;
}

Arithmetic_Codec::Arithmetic_Codec(unsigned max_code_bytes,
								   unsigned char * user_buffer)
{
	mode = buffer_size = 0;
	new_buffer = code_buffer = 0;
	set_buffer(max_code_bytes, user_buffer);
}

Arithmetic_Codec::~Arithmetic_Codec(void)                        // destructor
{
	delete [] new_buffer;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::set_buffer(unsigned max_code_bytes,
								  unsigned char * user_buffer)
{
	// test for reasonable sizes
	if ((max_code_bytes < 16) || (max_code_bytes > 0x1000000U))
		AC_Error("invalid codec buffer size");
	if (mode != 0) AC_Error("cannot set buffer while encoding or decoding");

	if (user_buffer != 0) {                       // user provides memory buffer
		buffer_size = max_code_bytes;
		code_buffer = user_buffer;               // set buffer for compressed data
		delete [] new_buffer;                 // free anything previously assigned
		new_buffer = 0;
		return;
	}

	if (max_code_bytes <= buffer_size) return;               // enough available

	buffer_size = max_code_bytes;                           // assign new memory
	delete [] new_buffer;                   // free anything previously assigned
	if ((new_buffer = new unsigned char[buffer_size+16]) == 0) // 16 extra bytes
		AC_Error("cannot assign memory for compressed data buffer");
	code_buffer = new_buffer;                  // set buffer for compressed data
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::start_encoder(void)
{
	if (mode != 0) AC_Error("cannot start encoder");
	if (buffer_size == 0) AC_Error("no code buffer set");

	mode   =   1;
	base   = 0.0;          // initialize encoder variables: interval and pointer
	length = 1.0;
	ac_pointer = code_buffer;                       // pointer to next data byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::start_decoder(void)
{
	if (mode != 0) AC_Error("cannot start decoder");
	if (buffer_size == 0) AC_Error("no code buffer set");

	mode   =   2;
	length = 1.0;                       // initialize decoder: interval, pointer
	value  = base = 0.0;
	// set initial code value: 48 bits
	for (unsigned k = 0; k < 6; k++)
		value = 256.0 * value + AC__LeastSignifBit * code_buffer[k];
	ac_pointer = code_buffer + 6;                   // pointer to next data byte
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::read_from_file(FILE * code_file)
{
	unsigned shift = 0, code_bytes = 0;
	int file_byte;
	// read variable-length header with number of code bytes
	do {
		if ((file_byte = getc(code_file)) == EOF)
			AC_Error("cannot read code from file");
		code_bytes |= unsigned(file_byte & 0x7F) << shift;
		shift += 7;
	} while (file_byte & 0x80);
	// read compressed data
	if (code_bytes > buffer_size) AC_Error("code buffer overflow");
	if (fread(code_buffer, 1, code_bytes, code_file) != code_bytes)
		AC_Error("cannot read code from file");

	start_decoder();                                       // initialize decoder
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::stop_encoder(void)
{
	if (mode != 1) AC_Error("invalid to stop encoder");
	mode = 0;
	// done encoding: set final data bytes
	unsigned last_bytes;
	unsigned a = unsigned(AC__OutputFactor * base);
	unsigned b = unsigned(AC__OutputFactor * (base + length));

	if (b - a < 2) {     // decide number of final byes based on interval length
		base += 0.5 * AC__MinLength;
		last_bytes = 3;                                          // output 3 bytes
	}
	else
		if ((b >> 8) - (a >> 8) < 2) {
			base += AC__MinLength;
			last_bytes = 2;                                        // output 2 bytes
		}
		else {
			base += 256.0 * AC__MinLength;
			last_bytes = 1;                                         // output 1 byte
		}

		if (base >= 1.0) propagate_carry();

		do {
			base *= 256.0;
			unsigned a = unsigned(base);               // save 8 most-significant bits
			*ac_pointer++ = (unsigned char) a;
			base -= double(a);                       // rescale interval by factor 256
		} while (--last_bytes);

		unsigned code_bytes = unsigned(ac_pointer - code_buffer);
		if (code_bytes > buffer_size) AC_Error("code buffer overflow");

		return code_bytes;                                   // number of bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

unsigned Arithmetic_Codec::write_to_file(FILE * code_file)
{
	unsigned header_bytes = 0, code_bytes = stop_encoder(), nb = code_bytes;

	// write variable-length header with number of code bytes
	do {
		int file_byte = int(nb & 0x7FU);
		if ((nb >>= 7) > 0) file_byte |= 0x80;
		if (putc(file_byte, code_file) == EOF)
			AC_Error("cannot write compressed data to file");
		header_bytes++;
	} while (nb);
	// write compressed data

	if (fwrite(code_buffer, 1, code_bytes, code_file) != code_bytes)
		AC_Error("cannot write compressed data to file");

	return code_bytes + header_bytes;                              // bytes used
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Arithmetic_Codec::stop_decoder(void)
{
	if (mode != 2) AC_Error("invalid to stop decoder");
	mode = 0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - Static bit model implementation - - - - - - - - - - - - - - - - - - - - -

Static_Bit_Model::Static_Bit_Model(void)
{
	bit_0_prob = 0.5;                           // set initial probability value
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Bit_Model::set_probability_0(double p0)
{
	if ((p0 < 0.00001)||(p0 > 0.99999))           // check for reasonable values
		AC_Error("invalid Static_Bit_Model probability");

	bit_0_prob = p0;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - Adaptive bit model implementation - - - - - - - - - - - - - - - - - - - -

Adaptive_Bit_Model::Adaptive_Bit_Model(void)
{
	reset();                                                 // initialize model
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::reset(void)
{
	bit_0_count = 1;                     // initialization to equiprobable model
	bit_count   = 2;
	bit_0_prob  = 0.5;
	update_cycle = bits_until_update = 4;         // start with frequent updates
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Bit_Model::update(void)
{
	// halve counts when a threshold is reached

	if ((bit_count += update_cycle) >= BM__MaxCount) {
		bit_count = (bit_count + 1) >> 1;
		bit_0_count = (bit_0_count + 1) >> 1;
		if (bit_0_count == bit_count) ++bit_count;
	}
	// compute scaled bit 0 probability

	bit_0_prob = double(bit_0_count) / double(bit_count);

	// set frequency of model updates
	update_cycle = (5 * update_cycle) >> 2;
	if (update_cycle > 64) update_cycle = 64;
	bits_until_update = update_cycle;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Static data model implementation  - - - - - - - - - - - - - - - - - - -

Static_Data_Model::Static_Data_Model(void)                                   // constructors
{
	data_symbols = 0;
	distribution = 0;
}

Static_Data_Model::~Static_Data_Model(void)                                    // destructor
{
	delete [] distribution;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Static_Data_Model::set_distribution(unsigned number_of_symbols,
										 const double probability[])
{
	if ((number_of_symbols < 2) || (number_of_symbols > (1 << 14)))
		AC_Error("invalid number of data symbols");

	if (data_symbols != number_of_symbols) {     // assign memory for data model
		data_symbols = number_of_symbols;
		delete [] distribution;
		distribution = new double[data_symbols];
		if (distribution == 0) AC_Error("cannot assign model memory");
	}
	// compute cumulative distribution
	double sum = 0.0, p = 1.0 / double(data_symbols);
	for (unsigned k = 0; k < data_symbols; k++) {
		if (probability) p = probability[k];
		if ((p < 0.00001) || (p > 0.99999)) AC_Error("invalid symbol probability");
		distribution[k] = sum;
		sum += p;
	}
	if ((sum < 0.99999) || (sum > 1.00001))
		AC_Error("invalid set of probabilities");
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - Adaptive data model implementation  - - - - - - - - - - - - - - - - - -

Adaptive_Data_Model::Adaptive_Data_Model(void)                 // constructors
{
	data_symbols = 0;
	distribution = 0;
	symbol_count = 0;
}

Adaptive_Data_Model::Adaptive_Data_Model(unsigned number_of_symbols)
{
	data_symbols = 0;
	distribution = 0;
	symbol_count = 0;
	set_alphabet(number_of_symbols);
}

Adaptive_Data_Model::~Adaptive_Data_Model(void)                  // destructor
{
	delete [] distribution;
	delete [] symbol_count;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::set_alphabet(unsigned number_of_symbols)
{
	if ((number_of_symbols < 2) || (number_of_symbols > (1 << 14)))
		AC_Error("invalid number of data symbols");

	if (data_symbols != number_of_symbols) {     // assign memory for data model
		data_symbols = number_of_symbols;
		delete [] distribution;
		delete [] symbol_count;
		symbol_count = new unsigned[data_symbols];
		distribution = new double[data_symbols];
		if ((symbol_count == 0) || (distribution == 0))
			AC_Error("cannot assign model memory");
	}

	reset();                                                 // initialize model
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::update(void)
{
	// halve counts when a threshold is reached

	if ((total_count += update_cycle) > DM__MaxCount) {
		total_count = 0;
		for (unsigned n = 0; n < data_symbols; n++)
			total_count += (symbol_count[n] = (symbol_count[n] + 1) >> 1);
	}

	unsigned sum = 0;
	double scale = 1.0 / double(total_count);
	// compute cumulative distribution
	for (unsigned k = 0; k < data_symbols; k++) {
		distribution[k] = scale * sum;
		sum += symbol_count[k];
	}
	// set frequency of model updates
	update_cycle = (5 * update_cycle) >> 2;
	unsigned max_cycle = (data_symbols + 6) << 3;
	if (update_cycle > max_cycle) update_cycle = max_cycle;
	symbols_until_update = update_cycle;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

void Adaptive_Data_Model::reset(void)
{
	if (data_symbols == 0) return;

	// restore probability estimates to uniform distribution
	total_count = 0;
	update_cycle = data_symbols;
	for (unsigned k = 0; k < data_symbols; k++) symbol_count[k] = 1;
	update();
	symbols_until_update = update_cycle = (data_symbols + 6) >> 1;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
