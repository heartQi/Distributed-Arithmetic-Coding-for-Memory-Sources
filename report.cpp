#include "report.h"
#include "string.h"
FILE* fpreport=NULL;
int report_start(char* reportfile)
{
	fpreport=fopen(reportfile,"at");
	if(fpreport==NULL)
	{
		fpreport=fopen(reportfile,"w+");
	}
	report_newline();
	return 0;
}
int report_stop(char* reportfile)
{
	return fclose(fpreport);
}
int report( const int ivalue)
{
	char buffer[20];
	itoa(ivalue,buffer,10);
	return report(buffer);
}
int report(const double fvalue)
{
	char buffer[40];
	sprintf(buffer,"%15.14f",fvalue);
	return report(buffer);
}
int report(const char* strvalue)
{
	//¿Õ¸ñ
	int ilen=strlen(strvalue);
	fwrite(strvalue,ilen,1,fpreport);
	fwrite("	",3,1,fpreport);
	return 0;
}
int report_newline()
{
	fwrite("\n",4,1,fpreport);
	return 0;
}