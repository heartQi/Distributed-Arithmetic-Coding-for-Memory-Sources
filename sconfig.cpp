//文件: sconfig.c & sconfig.h
//功能: 简单配置文件读写函数集(for Windows)
//时间: 2009-02-27

#include "string.h"
#include "process.h"
#include "sconfig.h"

static char newline[4] = {'\r','\n',0,0};

#define SCR_TMP_DESDIR "."

//函数: sc_copy
//功能: 复制文件
//参数: src  源文件路径和文件名
//      des  目的文件路径和文件名
//返回: 成功返回0, 失败返回-1。
int sc_copy(const char * src, const char * des)
{
   int rest = -1;
   size_t iz;
   FILE * fs = 0;
   FILE * fd = 0;

   char gcvar[SCR_VAR_MAXLEN + 1];

   if(src == 0) return -1;
   if(des == 0) return -1;

   fs = fopen(src, "rb");
   if(fs == 0) goto fun_end;

   fd = fopen(des, "wb");
   if(fd == 0) goto fun_end;

   for(;;)
   {
      iz = fread(gcvar, 1, sizeof(gcvar), fs);
      if(iz <= 0) break;
      if(fwrite(gcvar, 1, iz, fd) != iz) goto fun_end;
   }

   rest = 0;

   fun_end:
   {
      if(fs != 0) fclose(fs);
      if(fd != 0) fclose(fd);
      return rest;
   }
}

//函数: sc_read
//功能: 从配置文件读取字符串数据
//参数: name  配置文件路径和名字
//      pkey  键名(\0结尾的字串)
//      pdes  接收数据的内存地址
//      inum  接收数据的内存大小
//返回: 读取的字符个数(不包括\0结束符)值大于等于零, 失败则返回负值。
//备注: 函数读取并保存到参数pdes指向的内存中的数据为\0结尾的字符串；
//      参数pdes指向的内存块应足够存放包括\0结尾符在内的整个字符串；
//      若接收数据的内存大小小于字符串数据大小(包括\0)则函数返回-1；
//      若参数pdes或inum为零, 则该函数只返回数据的长度, 不读取数据。
//错误: -1=参数不正确, -2=无法打开文件, -3=键不存在, -4=未知的错误。
int sc_read(const char * name, const char * pkey, void * pdes, int inum)
{
   int iv, i, rst = -4;
   unsigned char ucvar;
   unsigned char * cp;
   FILE * fp = 0;

   //检查参数
   if(name == 0) return -1;
   if(pkey == 0) return -1;
   if(inum < 0) return -1;

   cp = (unsigned char *)pkey;
   iv = 0;

   for(i=0; i<=SCR_KEY_MAXLEN+1; i++,cp++)
   {
      if(*cp == 0)
      {
         iv = i;
         break;
      }

      if(!((*cp>='0' && *cp<='9') || *cp==' ' || *cp=='-' || *cp=='_'
         || (*cp>='a' && *cp<='z') || *cp=='/' || *cp=='\\' || *cp=='.'
         || (*cp>='A' && *cp<='Z') || *cp==':' || *cp>127)) return -1;
   }

   if(iv > SCR_KEY_MAXLEN || iv < 1) return -1;

   //打开配置文件
   fp = fopen(name, "rb");
   if(fp == 0)
   {
      rst = -2;
      goto read_exit;
   }

   //搜索键名
   search_key:
   {
      find_keyjp:

      //查找键起点标记
      for(;;)
      {
         iv = (int)fread(&ucvar, 1, sizeof(ucvar), fp);
         if(iv != sizeof(ucvar))
         {
            rst = -3;
            goto read_exit;
         }
         if(ucvar == '<') break;
         if(ucvar!=' ' && ucvar!='\t' && ucvar!=0xD && ucvar!=0xA) goto find_lend;
      }

      //比较键名
      for(cp=(unsigned char *)pkey ;; cp++)
      {
         iv = (int)fread(&ucvar, 1, sizeof(ucvar), fp);
         if(iv != sizeof(ucvar))
         {
            rst = -3;
            goto read_exit;
         }
         if(ucvar=='>' && *cp==0) goto search_equal;
         if(ucvar==0xD || ucvar==0xA) goto find_keyjp;
         if(*cp != ucvar) break;
      }

      find_lend:

      //查找行结束
      for(;;)
      {
         iv = (int)fread(&ucvar, 1, sizeof(ucvar), fp);
         if(iv != sizeof(ucvar))
         {
            rst = -3;
            goto read_exit;
         }
         if(ucvar==0xD || ucvar==0xA) break;
      }

      goto search_key;
   }

   search_equal:

   //搜索等号
   for(;;)
   {
      iv = (int)fread(&ucvar, 1, sizeof(ucvar), fp);
      if(iv != sizeof(ucvar))
      {
         rst = -4;
         goto read_exit;
      }
      if(ucvar == '=') break;
      if(ucvar!=' ' && ucvar!='\t')
      {
         rst = -4;
         goto read_exit;
      }
   }

   //搜索值起点标记
   for(;;)
   {
      iv = (int)fread(&ucvar, 1, sizeof(ucvar), fp);
      if(iv != sizeof(ucvar))
      {
         rst = -4;
         goto read_exit;
      }
      if(ucvar == '"') break;
      if(ucvar!=' ' && ucvar!='\t')
      {
         rst = -4;
         goto read_exit;
      }
   }

   //读取值字符串
   for(i=0,cp=(unsigned char*)pdes ;; i++,cp++)
   {
      if(inum!=0 && i>=inum)
      {
         rst = -1;
         goto read_exit;
      }
      if(i > SCR_VAR_MAXLEN)
      {
         rst = -4;
         goto read_exit;
      }
      iv = (int)fread(&ucvar, 1, sizeof(ucvar), fp);
      if(iv != sizeof(ucvar))
      {
         rst = -4;
         goto read_exit;
      }
      if(ucvar == '"')
      {
         if(pdes!=0 && inum!=0) *cp = 0;
         rst = i;
         goto read_exit;
      }
      if(ucvar < 32)
      {
         rst = -4;
         goto read_exit;
      }
      if(pdes!=0 && inum!=0) *cp = ucvar;
   }

   read_exit:
   if(fp != 0) fclose(fp);
   return rst;
}
//函数: sc_read_int
//功能: 读取int类型参数
//参数: 与sc_read 类似
int sc_read_int(const char * name, const char * pkey, int * pdes)
{
	char value[50];
	sc_read(name, pkey, value, 50);
	*pdes=atoi(value);
	return 1;
}
//函数: sc_read_int
//功能: 读取int类型参数
//参数: 与sc_read 类似
unsigned long sc_read_long(const char * name, const char * pkey, unsigned long * pdes)
{
	char value[50];
	sc_read(name, pkey, value, 50);
	*pdes=(unsigned long)atoi(value);
	return 1;
}
//函数: sc_read_double
//功能: 读取double类型参数
//参数: 与sc_read 类似
int sc_read_double(const char * name, const char * pkey, double * pdes)
{
	char value[50];
	sc_read(name, pkey,value,50);
	*pdes=atof(value);
	return 1;
}
//函数: sc_newline
//功能: 设置文件换行格式
//参数: SCL_PC或SCL_UNIX
void sc_newline(int iv)
{
   if(iv == SCL_PC) _snprintf(newline, sizeof(newline), "\r\n");
   else _snprintf(newline, sizeof(newline), "\n");
   return;
}

//函数: sc_open
//功能: 为读取而打开配置文件
//参数: 配置文件路径和文件名
//返回: 配置文件句柄, 失败返回0。
FILE * sc_open(const char * name)
{
   if(name == 0) return 0;
   return fopen(name, "rb");
}

//函数: sc_close
//功能: 关闭配置文件句柄
//参数: 配置文件句柄
void sc_close(FILE * vp)
{
   if(vp != 0) fclose(vp);
   return;
}

//函数: sc_reset
//功能: 重新设置配置读指针位置
//参数: 配置文件句柄
//返回: 成功返回0, 失败返回非0。
int sc_reset(FILE * vp)
{
   if(vp == 0) return -1;
   return fseek(vp, 0, SEEK_SET);
}

//函数: sc_readline
//功能: 从配置文件中读取一行配置
//参数: hcfg  配置文件句柄
//      pkey  键的接收地址(大小SCR_KEY_MAXLEN+1字节)
//      pvar  值的接收地址(大小SCR_VAR_MAXLEN+1字节)
//返回: 成功返回1, 空行无数据返回0, 结束返回-1。
//备注: 返回的键和值均为NULL结尾的字符串数据。
int sc_readline(FILE * hcfg, char * pkey, char * pvar)
{
   int iv, i, irst = 0;
   unsigned char ucvar;

   if(hcfg == 0) return -1;
   if(pkey == 0) return -1;
   if(pvar == 0) return -1;

   //查找键起点标记
   for(;;)
   {
      iv = (int)fread(&ucvar, 1, sizeof(ucvar), hcfg);
      if(iv != sizeof(ucvar)) return -1;
      if(ucvar == '<') break;
      if(ucvar == 0xD || ucvar == 0xA) return 0;
      if(ucvar != ' ' && ucvar != '\t') goto find_lend;
   }

   //读取键字符串
   for(i=0 ;; i++)
   {
      if(i > SCR_KEY_MAXLEN) goto find_lend;

      iv = (int)fread(&ucvar, 1, sizeof(ucvar), hcfg);
      if(iv != sizeof(ucvar)) return -1;

      if(ucvar == '>')
      {
         if(i == 0) goto find_lend;
         else{ pkey[i] = 0; break; }
      }

      if(ucvar == 0xD || ucvar == 0xA) return 0;

      if(!((ucvar>='0' && ucvar<='9') || ucvar==' ' || ucvar=='-' || ucvar=='_'
      || (ucvar>='a' && ucvar<='z') || ucvar=='/' || ucvar=='\\' || ucvar=='.'
      || (ucvar>='A' && ucvar<='Z') || ucvar==':' || ucvar>127)) goto find_lend;

      pkey[i] = (char)ucvar;
   }

   //搜索等号
   for(;;)
   {
      iv = (int)fread(&ucvar, 1, sizeof(ucvar), hcfg);
      if(iv != sizeof(ucvar)) return -1;
      if(ucvar == '=') break;
      if(ucvar == 0xD || ucvar == 0xA) return 0;
      if(ucvar != ' ' && ucvar != '\t') goto find_lend;
   }

   //搜索值起点标记
   for(;;)
   {
      iv = (int)fread(&ucvar, 1, sizeof(ucvar), hcfg);
      if(iv != sizeof(ucvar)) return -1;
      if(ucvar == '"') break;
      if(ucvar == 0xD || ucvar == 0xA) return 0;
      if(ucvar != ' ' && ucvar != '\t') goto find_lend;
   }

   //读取值字符串
   for(i=0 ;; i++)
   {
      if(i > SCR_VAR_MAXLEN) goto find_lend;

      iv = (int)fread(&ucvar, 1, sizeof(ucvar), hcfg);
      if(iv != sizeof(ucvar)) return -1;

      if(ucvar == '"')
      {
         pvar[i] = 0;
         break;
      }

      if(ucvar == 0xD || ucvar == 0xA) return 0;
      if(ucvar < 32) goto find_lend;

      pvar[i] = (char)ucvar;
   }

   irst = 1;

   find_lend:

   //查找行结束
   for(;;)
   {
      iv = (int)fread(&ucvar, 1, sizeof(ucvar), hcfg);
      if(iv != sizeof(ucvar)) break;
      if(ucvar == 0xD || ucvar == 0xA) break;
   }

   return irst;
}

//函数: sc_write
//功能: 添加或改写键值到配置文件
//参数: name  配置文件路径和名字
//      pkey  键名(\0结尾的字串)
//      pvar  键值(\0结尾的字串)
//      mode  模式(0:添加或替换,SCW_APPEND:添加,SCW_REPLACE:替换)
//返回: 成功返回0, 失败则返回-1。
int sc_write(const char * name, const char * pkey, const char * pvar, int mode)
{
   int iv, i;
   int rst = -1;
   int iwr = -1;
   unsigned char * cp;
   FILE * hs;
   FILE * fd;

   char gckey[SCR_KEY_MAXLEN + 1];
   char gcvar[SCR_VAR_MAXLEN + 1];

   if(name == 0) return -1;
   if(pkey == 0) return -1;
   if(pvar == 0) return -1;

   cp = (unsigned char *)pkey;
   iv = 0;

   for(i=0; i<=SCR_KEY_MAXLEN+1; i++,cp++)
   {
      if(*cp==0)
      {
         iv = i;
         break;
      }

      if(!((*cp>='0' && *cp<='9') || *cp==' ' || *cp=='-' || *cp=='_'
         || (*cp>='a' && *cp<='z') || *cp=='/' || *cp=='\\' || *cp=='.'
         || (*cp>='A' && *cp<='Z') || *cp==':' || *cp>127)) return -1;
   }

   if(iv>SCR_KEY_MAXLEN || iv<1) return -1;

   cp = (unsigned char *)pvar;
   iv = 0;

   for(i=0; i<=SCR_VAR_MAXLEN+1; i++,cp++)
   {
      if(*cp == 0)
      {
         iv = i;
         break;
      }

      if(*cp < 32 || *cp == '"') return -1;
   }

   if(iv>SCR_VAR_MAXLEN || iv<0) return -1;

   _snprintf(gcvar, sizeof(gcvar) - 1,
   "%s\\~scw_%x_%x", SCR_TMP_DESDIR, _getpid(), (int)&hs);
   gcvar[sizeof(gcvar) - 1] = 0;
   fd = fopen(gcvar, "wb");
   if(fd == NULL) return -1;

   hs = sc_open(name);
   if(hs != 0) for(;;)
   {
      iv = sc_readline(hs, gckey, gcvar);
      if(iv < 0) break;
      if(iv == 0) continue;

      cp = (unsigned char *)gcvar;
      iv = strcmp(pkey, gckey);
      if(iv == 0 && mode == SCW_APPEND) goto fun_end;
      if(iv == 0){ cp = (unsigned char *)pvar; iwr = 0; }

      if(fputs("<", fd) == EOF) goto fun_end;
      if(fputs(gckey, fd) == EOF) goto fun_end;
      if(fputs("> = \"", fd) == EOF) goto fun_end;
      if(fputs((char *)cp, fd) == EOF) goto fun_end;
      if(fputs("\"", fd) == EOF) goto fun_end;
      if(fputs(newline, fd) == EOF) goto fun_end;
   }

   if(iwr != 0)
   {
      if(mode == SCW_REPLACE) goto fun_end;
      if(fputs("<", fd) == EOF) goto fun_end;
      if(fputs(pkey, fd) == EOF) goto fun_end;
      if(fputs("> = \"", fd) == EOF) goto fun_end;
      if(fputs(pvar, fd) == EOF) goto fun_end;
      if(fputs("\"", fd) == EOF) goto fun_end;
      if(fputs(newline, fd) == EOF) goto fun_end;
   }

   rst = 0;

   fun_end:
   {
      fclose(fd);
      sc_close(hs);

      if(rst == 0)
      {
         _snprintf(gcvar, sizeof(gcvar) - 1,
         "%s\\~scw_%x_%x", SCR_TMP_DESDIR, _getpid(), (int)&hs);
         gcvar[sizeof(gcvar) - 1] = 0;
         rst = sc_copy(gcvar, name);

         _snprintf(gcvar, sizeof(gcvar) - 1,
         "%s\\~scw_%x_%x", SCR_TMP_DESDIR, _getpid(), (int)&hs);
         gcvar[sizeof(gcvar) - 1] = 0;
         _unlink(gcvar);
      }
      else
      {
         _snprintf(gcvar, sizeof(gcvar) - 1,
         "%s\\~scw_%x_%x", SCR_TMP_DESDIR, _getpid(), (int)&hs);
         gcvar[sizeof(gcvar) - 1] = 0;
         _unlink(gcvar);
      }
   }
   return rst;
}
