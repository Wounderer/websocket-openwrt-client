/*
 * libwebsockets-DTS2B-client - libwebsockets DTS2B implementation
 *
 * Copyright (C) 2015 mleaf_hexi <350983773@qq.com> ,<mleaf90@gmail.com>
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include </usr/local/include/json/json.h>//json��ͷ�ļ�
#include <uuid/uuid.h>//����UUID���ͷ�ļ�
#include<sqlite3.h>//sqlite3���ļ�
#include <libwebsockets.h>

//�������ͷ�ļ� 
#include <sys/types.h>  
#include <sys/stat.h>  
#include <fcntl.h> //�ļ����ƶ���  
#include <termios.h>//�ն˿��ƶ���  
#include <errno.h>  
#include <sys/signal.h>  
#include <pthread.h>
//ʱ����غ���ͷ�ļ�
#include <sys/time.h>
#include <time.h>

char *errmsg=0;

#ifdef CMAKE_BUILD
#include "lws_config.h"
#endif


static unsigned int opts;
static int was_closed;
static int deny_deflate;
static int deny_mux;
static struct libwebsocket *wsi_mirror;
static int mirror_lifetime = 0;
static volatile int force_exit = 0;
static int longlived = 0;
/**************
*������ض���
***************/
#define FALSE_SERIAL 0
#define TRUE_SERIAL 1

#define DEVICE "/dev/ttyS0" //�豸�ڵ� 
int serial_fd = 0; 
char wait_flag=0;
volatile int STOP=FALSE_SERIAL; 
char send_wait_flag=0;
#define use_signal 0
#define use_select 1

/******************
*���ݿ�����ļ�����
*******************/
sqlite3 *brxydatadb;

/************************
ͨ��IP��ַ��ض���
*************************/
char *get_ipaddr1;
char *get_ipaddr2;
char *get_ipindex1;
char *get_ipindex2;
char *use_ssl_set;

static struct libwebsocket_context *context;

//#define LOCAL_RESOURCE_PATH INSTALL_DATADIR"/libwebsockets-client"
#define LOCAL_RESOURCE_PATH "/libwebsockets-client"

char *resource_path = LOCAL_RESOURCE_PATH;
/*****************
ʱ����ض���
*****************/
long start_time_old;
long end_time_old;
char *timeused_old;
long end_time;
long start_time;
char *timeused_old_back=0;

int gettimeofday(struct timeval *tv, struct timezone *tz);

/*�豸״̬��غ궨��*/
#define ONLINE 1          //���ߣ���ʾ �豸������ ���ҹ�������
#define TROUBLE 2        // ��ʾ �豸�����ӣ����ǹ���������
#define SHUTDOWNING 3   //��ʾ �豸�����ӣ����ǹ���������
#define REBOOTING 4    //��ʾ �豸��������
#define BOOTING 5     //��ʾ �豸��������
#define SHUTDOWN 6   //��ʾ �豸�ѹر�
#define UPGRADING 7 //��ʾ �豸��������
#define DISABLED 8 //��ʾ �豸�Ѿ�������
#define UNKNOWN 9 //��ʾ �豸״̬����֪

/*�豸����-DeviceType*/
#define DTS_ROUTER	1 //���ֽ�ѧһ��� - ·����
#define DTS_EMBEDDED_COMPUTER 2	//���ֽ�ѧһ��� - ��Ƕ PC
#define DTS_PROJECTOR 3	//���� DTS ��ͶӰ��
#define DTS_DISPLAYER 4	//���� DTS ����ʾ�豸
#define DTS_SWITCH 5	//����
#define DTS_RFID_READER 6 //RFID������
#define DTS_SENSOR 7	//������
#define DTS_ALL_PERIPHERY_DEVICE 8	//DTS ��������
/*�豸������-ConfigOption*/

	/*������ƽ̨ ���� URL ������ {deviceId}��
	DTSͨ��URL �� schema �ж��Ƿ�ʹ�ð�ȫ���ӣ�
	�� wss ʹ�ð�ȫ���ӡ������ö������URL,
	��ʹ�÷ָ���Ӣ�ﶺ�ŷָ�, 
	DTS �豸 Ӧ���� ���� ��һ�� URL��
	���ʧ�ܲ����еڶ���URL ��Ӧ�������ӵڶ��� URL�� 
	�Դ�����.*/
#define CLOUD_PLATFORM_WEBSOCKET_URL 192.168.1.123
/*DTS �ϱ��豸״̬ Ƶ�ʣ���λ �롣 ��һ���ϱ�ʱ�� Ӧ�����*/
#define REPORT_DTS_DEVICE_STATUS_PERIOD 72
/*DTS �ϴ�����־ Ƶ��, ��λ �롣��һ���ϴ���־��ʱ�� �ں����ʱ����������*/
#define UPLOAD_DTS_LOG_DATA_PERIOD 12 
/*DTS ����־����, ���ܵ�ֵ�� ALL,DEBUG, INFO, WARN, ERROR,FATAL,OFF*/
#define DTS_LOG_LEVEL 1
/*DTS �ϱ��豸��ǰ������Ϣ Ƶ�ʣ���λ �롣 ��һ���ϱ�ʱ�� Ӧ�����*/
#define REPORT_DTS_DEVICE_RUNTIME_INFO_PERIOD 12
/*��ѧ���ڿ�ʼʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_IN_START_TIME 200
/*��ѧ���ڽ���ʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_IN_END_TIME 200
/*��ѧ���ڿ�ʼʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_OUT_START_TIME 200
/*��ѧ���ڽ���ʱ��,��ֵ��ʾ����һ�쿪ʼ(00:00)�ĺ�����*/
#define CHECK_OUT_END_TIME 200

/*sqlite3��غ���*/
/*
*�������ݵ�sqlite3 ���ݿ�
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/
void insert_data(sqlite3 *db)
{
    int number,age,score;
    char name[20];
 
    printf("input the number: ");
    scanf("%d",&number);
    getchar();
    printf("input the name: ");
    scanf("%s",name);
    getchar();
    printf("input the age: ");
    scanf("%d",&age);
    getchar();
    printf("input the score ");
    scanf("%d",&score);
    getchar();
    char *sql=sqlite3_mprintf("insert into RFID values('%d','%s','%d','%d')",number,name,age,score);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec");
        exit(-1);
    }
    else
        printf("insert success!!\n");
    return;
}
 
 /*
 *ɾ��sqlite3 ���ݿ�number λ�õ�����
 *Author  mleaf_hexi
 *Mail:350983773@qq.com
 */
void delete_data(sqlite3 *db)
{
    int num;
    printf("please input the number you want to delete\n");
    scanf("%d",&num);
    getchar();
	//ɾ����student���ݿ��number��ͷ��
    char *sql=sqlite3_mprintf("delete from RFID where number ='%d'",num);
 
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec_delete");
        exit(-1);
    }
    else
        printf("delete success!!\n");
    return;
 
}
 
 /*
 *����sqlite3 ���ݿ�����
 *Author  mleaf_hexi
 *Mail:350983773@qq.com
 */
void updata_ipaddr_data(char ipaddr1[40],char ipaddr2[40],char ipindex1[40],char ipindex2[40],char use_ssl[3])
{
    int num=1;
 
  	char *sql12=sqlite3_mprintf("update ipaddr set ipaddr1='%s',ipaddr2='%s',ipindex1='%s',ipindex2='%s',use_ssl='%s' where number='%d'",ipaddr1,ipaddr2,ipindex1,ipindex2,use_ssl,num);
  	printf("sqlite3_exec=%d\n",sqlite3_exec(brxydatadb,sql12,NULL,NULL,&errmsg));
	if(sqlite3_exec(brxydatadb,sql12,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec_update");
        exit(-1);
    }
    else
        printf("ipaddr update success!!\n");
    return;
 
}
/*
*��ʾsqlite3 ���ݿ�����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/
 
void get_sqlite3_ipaddr_data(void)
{
    char ** resultp;
    int nrow,ncolumn,i,j,index;
    char *sql="select * from ipaddr";
	/*
		int sqlite3_get_table(
	  	sqlite3 *db,          //An open database 
	  	const char *zSql,    // SQL to be evaluated 
	  	char ***pazResult,   // Results of the query
	  	
	  	int *pnRow,           //Number of result rows written here
	  	int *pnColumn,        //Number of result columns written here
	  	char **pzErrmsg      //Error msg written here 
	);
	*/

    if(sqlite3_get_table(brxydatadb,sql,&resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_get_table");
        exit(-1);
    }
 
    for( i=0 ; i<( nrow + 1 ) * ncolumn ; i++ )
	{
		printf( "resultp[%d] = %s\n", i , resultp[i] );
	}
	//��ȡ���ݿ���ipaddr����Ϣ
	get_ipaddr1=resultp[7];
	get_ipaddr2=resultp[8];
	get_ipindex1=resultp[9];
	get_ipindex2=resultp[10];
	use_ssl_set=resultp[11];
    return ;
}
/*
*�˳�sqlite3 ���ݿ�
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/

void quit_sqlite3(void)
{
    printf("BYBYE!!\n");
    sqlite3_close(brxydatadb);
    exit(0);
}
/*
*sqlite3 ���ݿ��ʼ������
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/

void sqlite3_timeused_init(void);

int sqlite3_init(void)
{
    int nu;
   	int  ret = 0;

	int number;
	char *ipaddr1;
	char *ipaddr2;
	char *ipindex1;
	char *ipindex2;
	char *use_ssl;
    if(sqlite3_open("brxydata.db",&brxydatadb) != SQLITE_OK)
    {
        perror("sqlite3_open");
        exit(-1);
    }
	//�����ȴ������ݿ�� ��Ȼ������ӳ�Ա
	//�������ݿ��
	const char *SQL1="create table if not exists ipaddr(number,ipaddr1 varchar(40),ipaddr2 varchar(40),ipindex1 varchar(40),ipindex2 varchar(40),use_ssl varchar(3));";
	/*int sqlite3_exec(
	*	  sqlite3* ppDb,                                          //An open database 
	*	  const char *sql,                                        //SQL to be evaluated 
	* 	  int (*callback)(void*,int,char**,char**), //Callback function 
	*	  void *,                                                    //1st argument to callback 
	*	  char **errmsg                                        // Error msg written here
	*	);
	*/

	//ִ�н��� 
    ret = sqlite3_exec(brxydatadb,SQL1,0,0,&errmsg);
    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"SQL Error:%s\n",errmsg);
        sqlite3_free(errmsg);
    }
	sqlite3_timeused_init();
#if 0
  //��������  ֻ��������һ�� ��Ȼ��ÿһ�����г����ʱ�����ݿ��е���ͬ���ݻ�����
  	number=1;
	ipaddr1="192.168.6.114";
	ipaddr2="192.168.6.176";
	ipindex1="/";
	ipindex2="/websocket";
	use_ssl="ws";
  char *sqlipaddr=sqlite3_mprintf("insert into ipaddr values('%d','%s','%s','%s','%s','%s')",number,ipaddr1,ipaddr2,ipindex1,ipindex2,use_ssl);
  //char *sqlipaddr=sqlite3_mprintf("update ipaddr set ipaddr1='%s',ipaddr2='%s',ipindex1='%s',ipindex2='%s',use_ssl='%s' where number='%d'",ipaddr1,ipaddr2,ipindex1,ipindex2,use_ssl,number);
  	
	if(sqlite3_exec(brxydatadb,sqlipaddr,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec");
        exit(-1);
    }
    else
        printf("insert success!!\n");
#endif
    return 0;
}
/**************************
ʱ����ص����ݿ��������
***************************/
/************************************************************************ 
*������:updata_timeused_data����ʹ��ʱ�䵽���ݿ�
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/

void updata_timeused_data(long start_time,long end_time,long timeused)
{
	int num=1;
 
	char *sql=sqlite3_mprintf("update timeused set start_time='%ld',end_time='%ld',timeused='%ld' where number='%d'",start_time,end_time,timeused,num);
	printf("sqlite3_exec=%d\n",sqlite3_exec(brxydatadb,sql,NULL,NULL,&errmsg));
	if(sqlite3_exec(brxydatadb,sql,NULL,NULL,&errmsg) != SQLITE_OK)
	{
		perror("sqlite3_exec_update");
		exit(-1);
	}
	else
		printf("ipaddr update success!!\n");

 
}
/************************************************************************ 
*������:sqlite3_timeused_init��ʼ������ʹ��ʱ�����ݿ��
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/

void sqlite3_timeused_init(void)
{

	int  ret = 0;
	int number;
	long  start_time;
	long  end_time;
	long  timeused;
	//�����ȴ������ݿ�� ��Ȼ������ӳ�Ա
	//�������ݿ��
    const char *SQL1="create table if not exists timeused(number,start_time,end_time,timeused );";
/*int sqlite3_exec(
*	  sqlite3* ppDb,                                          //An open database 
*	  const char *sql,                                        //SQL to be evaluated 
* 	  int (*callback)(void*,int,char**,char**), //Callback function 
*	  void *,                                                    //1st argument to callback 
*	  char **errmsg                                        // Error msg written here
*	);
*/
	//ִ�н���
    ret = sqlite3_exec(brxydatadb,SQL1,0,0,&errmsg);
    if(ret != SQLITE_OK)
    {
        fprintf(stderr,"SQL Error:%s\n",errmsg);
        sqlite3_free(errmsg);
    }
#if 0
	number=1;
	start_time=0;
	end_time=0;
	timeused=0;
  char *sqltimeused=sqlite3_mprintf("insert into timeused values('%d','%ld','%ld','%ld')",number,start_time,end_time,timeused);
   	
	if(sqlite3_exec(brxydatadb,sqltimeused,NULL,NULL,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_exec");
        exit(-1);
    }
    else
        printf("insert success!!\n");
#endif

}
/************************************************************************ 
*������:get_sqlite3_timeused_data ��ȡ���ݿ��б���ĳ����ʹ��ʱ��
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/

void get_sqlite3_timeused_data(void)
{
    char ** resultp;
    int nrow,ncolumn,i,j,index;
    char *sql="select * from timeused";
	/*
		int sqlite3_get_table(
	  	sqlite3 *db,          //An open database 
	  	const char *zSql,    // SQL to be evaluated 
	  	char ***pazResult,   // Results of the query
	  	
	  	int *pnRow,           //Number of result rows written here
	  	int *pnColumn,        //Number of result columns written here
	  	char **pzErrmsg      //Error msg written here 
	);
	*/
		

    if(sqlite3_get_table(brxydatadb,sql,&resultp,&nrow,&ncolumn,&errmsg) != SQLITE_OK)
    {
        perror("sqlite3_get_table");
        exit(-1);
    }
 
    for( i=0 ; i<( nrow + 1 ) * ncolumn ; i++ )
	{
		printf( "resultp[%d] = %s\n", i , resultp[i] );
	}
	timeused_old=resultp[7];

}
/************************************************************************ 
*������:get_start_time ��ȡ����ʼʱ��ʱ���Ժ���Ϊ��λ
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/

void get_start_time(void)
{
	struct timeval t_start;
	//get start time
	gettimeofday(&t_start, NULL);
	start_time = ((long)t_start.tv_sec)*1000+(long)t_start.tv_usec/1000;
	printf("Start time: %ld ms\n", start_time);

}
/************************************************************************ 
*������:get_end_time ��ȡ�������ʱ��ʱ���Ժ���Ϊ��λ
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/

void get_end_time(void)
{
		struct timeval t_end;
		//get end time
		gettimeofday(&t_end, NULL);
		end_time= ((long)t_end.tv_sec)*1000+(long)t_end.tv_usec/1000;
		printf("End time: %ld ms\n", end_time);

}
/************************************************************************ 
*������:time_get_init  ��ʼ��ʱ��洢
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/

void time_get_init()
{
	sqlite3_timeused_init();//��ʼ��ʱ��洢���ݿ�
	get_sqlite3_timeused_data();//��ȡ�ϴ����е�ʱ��
	if(strcmp(timeused_old,"0")==0)//����ǵ�һ������
	{
		timeused_old_back=0;
		printf("timeused_old_back= 0\n");

	}
	else
	{
		timeused_old_back=timeused_old;
		printf("timeused_old_back= %s ms\n", timeused_old_back);
	}

}
/************************************************************************ 
*������:time_get_end  ����������ʱ�Ϳ�ʼ��ʱ���
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/

void time_get_end()
{
	long cost_time = 0;

//calculate time slot
	if(timeused_old_back==0)
	{
		cost_time = end_time - start_time;
		printf("cost_time = end_time - start_time\n");

	}
	else
	{
		//strtol(timeused_old_back,NULL,10)  ���ַ���ת��Ϊ�����͵�10������
		cost_time = (end_time - start_time)+strtol(timeused_old_back,NULL,10);
		printf("cost_time = (end_time - start_time)+strtol(timeused_old_back,NULL,10)\n");
	}
	updata_timeused_data(start_time,end_time,cost_time);
	printf("Cost time: %ld ms\n", cost_time);
	get_sqlite3_timeused_data();
}

/*
*��ȡ�����UUID
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/
static unsigned char* uuidget(char str[36])
{
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, str);
    printf("%s\n", str);
    return str;
}


/*
*2.1.1	�ϱ�DTS״̬
*��������: unsigned char*
*����ֵ: json��ʽ����
*Author: mleaf_hexi
*Mail:350983773@qq.com
*/
static unsigned char* reportDeviceStatus(void)
{
	char struuidget[36];
	int L = 0;
	int n;
	unsigned char*reportDeviceStatus_string;
	unsigned char reportDeviceStatus_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
								  LWS_SEND_BUFFER_POST_PADDING];
	json_object *mainjson=json_object_new_object();
	json_object *header=json_object_new_object();
	json_object *action=json_object_new_object();

	
	//header
	json_object *action1=json_object_new_array();
	json_object *requestId=json_object_new_array();
	json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_STATUS"));
	uuidget(struuidget);//��ȡUUID��struuidget
	printf("uuidget = %s\n", struuidget);
	json_object_array_add(requestId,json_object_new_string(struuidget));
	json_object_object_add(action,"action",action1);
	json_object_object_add(action,"requestId",requestId);
	json_object_object_add(mainjson,"header",action);
	//data
       // "deviceType1": "<DeviceType1>", "status"
	json_object *status1=json_object_new_object();
	json_object_object_add(status1,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode11"));
	json_object_object_add(status1,
	   "deviceStatus",json_object_new_string("<DeviceStatus11>"));
	
	json_object *status11=json_object_new_object();
	json_object_object_add(status11,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode12"));
	json_object_object_add(status11,
	   "deviceStatus",json_object_new_string("<DeviceStatus12>"));

	// "deviceType2": "<DeviceType2>", "status"
	
	json_object *status2=json_object_new_object();
	json_object_object_add(status2,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode21"));
	json_object_object_add(status2,
	   "deviceStatus",json_object_new_string("<DeviceStatus21>"));

	json_object *status22=json_object_new_object();
	json_object_object_add(status22,
	   "deviceIdentify",json_object_new_string("DeviceIdentifyCode22"));
	json_object_object_add(status22,
	   "deviceStatus",json_object_new_string("<DeviceStatus22>"));


	
	json_object *status3=json_object_new_array();
	json_object_array_add(status3,status1);
	json_object_array_add(status3,status11);
	
	
	
	const char *status3_str=json_object_to_json_string(status3);
	
	printf("status3_str=%s\n",status3_str);
	/*
	[
	    {
	        "deviceIdentify": "DeviceIdentifyCode11",
	        "deviceStatus": "<DeviceStatus11>"
	    },
	    {
	        "deviceIdentify": "DeviceIdentifyCode12",
	        "deviceStatus": "<DeviceStatus12>"
	    }
	]

	*/

	json_object *status5=json_object_new_array();
		json_object_array_add(status5,status2);
	json_object_array_add(status5,status22);
	const char *status5_str=json_object_to_json_string(status5);
	
	printf("status5_str=%s\n",status5_str);
	
	/*
	[
	    {
	        "deviceIdentify": "DeviceIdentifyCode21",
	        "deviceStatus": "<DeviceStatus21>"
	    },
	    {
	        "deviceIdentify": "DeviceIdentifyCode22",
	        "deviceStatus": "<DeviceStatus22>"
	    }
	]
	*/

	json_object *status4=json_object_new_object();
	json_object_object_add(status4, "deviceType1",json_object_new_string("<DeviceType1>"));
	json_object_object_add(status4,"status",status3);
	
	
	const char *status4_str=json_object_to_json_string(status4);
	
	printf("status4_str=%s\n",status4_str);

	/*
		{
	    "deviceType1": "<DeviceType1>",
	    "status": [
	        {
	            "deviceIdentify": "DeviceIdentifyCode11",
	            "deviceStatus": "<DeviceStatus11>"
	        },
	        {
	            "deviceIdentify": "DeviceIdentifyCode12",
	            "deviceStatus": "<DeviceStatus12>"
	        }
		    ]
		}

	*/
	json_object *status6=json_object_new_object();
	json_object_object_add(status6, "deviceType2",json_object_new_string("<DeviceType2>"));
	json_object_object_add(status6,"status",status5);
	
	
	const char *status6_str=json_object_to_json_string(status6);
	
	printf("status6_str=%s\n",status6_str);
	/*
	{
	    "deviceType2": "<DeviceType2>",
	    "status": [
	        {
	            "deviceIdentify": "DeviceIdentifyCode21",
	            "deviceStatus": "<DeviceStatus21>"
	        },
	        {
	            "deviceIdentify": "DeviceIdentifyCode22",
	            "deviceStatus": "<DeviceStatus22>"
	        }
	    ]
	}
	*/
	

	json_object *data=json_object_new_object();
	json_object *data1=json_object_new_array();
	json_object_array_add(data1,status4);
	json_object_array_add(data1,status6);
	json_object_object_add(mainjson,"data",data1);

	const char *str=json_object_to_json_string(mainjson);
	printf("%s\n",str);
	/*
	{
    "header": {
        "action": [
            "REPORT_DTS_DEVICE_STATUS"
        ],
        "requestId": [
            "825b2533-e507-4708-8c0b-eb77f62fe596"
        ]
    },
    "data": [
        {
            "deviceType1": "<DeviceType1>",
            "status": [
                {
                    "deviceIdentify": "DeviceIdentifyCode11",
                    "deviceStatus": "<DeviceStatus11>"
                },
                {
                    "deviceIdentify": "DeviceIdentifyCode12",
                    "deviceStatus": "<DeviceStatus12>"
                }
            ]
        },
        {
            "deviceType2": "<DeviceType2>",
            "status": [
                {
                    "deviceIdentify": "DeviceIdentifyCode21",
                    "deviceStatus": "<DeviceStatus21>"
                },
                {
                    "deviceIdentify": "DeviceIdentifyCode22",
                    "deviceStatus": "<DeviceStatus22>"
                }
            ]
        }
    ]
}
	*/

	printf("%s\n",str);
	for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&reportDeviceStatus_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
	reportDeviceStatus_string=&reportDeviceStatus_buf[LWS_SEND_BUFFER_PRE_PADDING];
	
	json_object_put(mainjson);
	json_object_put(header);
	json_object_put(action);
	json_object_put(action1);
	json_object_put(requestId);
	json_object_put(status1);
	json_object_put(status11);
	json_object_put(status2);
	json_object_put(status22);
	json_object_put(status3);
	json_object_put(status4);
	json_object_put(status5);
	json_object_put(status6);
	json_object_put(data);
	json_object_put(data1);	
	return reportDeviceStatus_string;
	
}

/*
* 2.1.2	�ϴ�DTS��־
*��������: unsigned char*
*����ֵ: json��ʽ����
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*/
static unsigned char* reportDeviceLog(void) 
{
	char struuidget[36];
	int L = 0;
	int n;
	unsigned char*reportDeviceLog_string;
	unsigned char DeviceLog_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
								  LWS_SEND_BUFFER_POST_PADDING];
	json_object *mainjson=json_object_new_object();
	json_object *header=json_object_new_object();
	json_object *action=json_object_new_object();
	json_object *data=json_object_new_object();
	
	
	json_object *action1=json_object_new_array();
	json_object *requestId=json_object_new_array();
	json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_LOG"));
	uuidget(struuidget);//��ȡUUID��struuidget
	printf("uuidget = %s\n", struuidget);
	json_object_array_add(requestId,json_object_new_string(struuidget));//��ȡUUIDд��requestId
	json_object_object_add(action,"action",action1);
	json_object_object_add(action,"requestId",requestId);
	json_object_object_add(mainjson,"header",action);


	json_object_object_add(data,
	   "fromDatetime",json_object_new_int64(9223372036854775807));
	json_object_object_add(data,
	   "toDatetime",json_object_new_int(39));
	json_object_object_add(data,
	   "logData",json_object_new_string("Logdatafromdtsdevice1asdfaew"));
	json_object_object_add(mainjson,"data",data);

	
	const unsigned char *str=json_object_to_json_string(mainjson);
	printf("%s\n",str);
	for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&DeviceLog_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
	reportDeviceLog_string=&DeviceLog_buf[LWS_SEND_BUFFER_PRE_PADDING];
	json_object_put(action);//�ڳ�������ͷ���Դ
	json_object_put(action1);
	json_object_put(header);
	json_object_put(requestId);
	json_object_put(data);
	json_object_put(mainjson);
	
	return reportDeviceLog_string;
	
}

/*
*2.1.3	�ϱ�DTS������Ϣ
*��������: unsigned char*
*����ֵ: json��ʽ����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/

static unsigned char* reportDeviceRuntimeInfo(void)
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*reportDeviceRuntimeInfo_string;
		unsigned char reportDeviceRuntimeInfo_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
	
		
		//header
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_RUNTIME_INFO"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
		//data
	// "deviceType1": "<DeviceType1>", "idenfities"
		json_object *status1=json_object_new_object();
		json_object_object_add(status1,
		   "deviceProperty",json_object_new_string("<DeviceProperty1>"));
		json_object_object_add(status1,
		   "propertyValue",json_object_new_string("PropertyValue1"));
		
		json_object *status11=json_object_new_object();
		json_object_object_add(status11,
		   "deviceProperty",json_object_new_string("<DeviceProperty2>"));
		json_object_object_add(status11,
		   "propertyValue",json_object_new_string("PropertyValue2"));
	
		// "deviceType2": "<DeviceType2>", "status"
		
		json_object *status2=json_object_new_object();
		json_object_object_add(status2,
		   "deviceProperty",json_object_new_string("<DeviceProperty3>"));
		json_object_object_add(status2,
		   "propertyValue",json_object_new_string("PropertyValue3"));
	
		json_object *status22=json_object_new_object();
		json_object_object_add(status22,
		   "deviceProperty",json_object_new_string("<DeviceProperty4>"));
		json_object_object_add(status22,
		   "propertyValue",json_object_new_string("PropertyValue4"));
	
	
		
		json_object *status3=json_object_new_array();
		json_object_array_add(status3,status1);
		json_object_array_add(status3,status11);
		
		
		
		const char *status3_str=json_object_to_json_string(status3);
		
		printf("status3_str=%s\n",status3_str);
		/*
			[
			{
				"deviceProperty": "<DeviceProperty1>",
				"propertyValue": "PropertyValue1"
			},
			{
				"deviceProperty": "<DeviceProperty2>",
				"propertyValue": "PropertyValue2"
			}
		]
	
		*/
	
		json_object *status5=json_object_new_array();
		json_object_array_add(status5,status2);
		json_object_array_add(status5,status22);
		const char *status5_str=json_object_to_json_string(status5);
		
		printf("status5_str=%s\n",status5_str);
		
		/*
			[
			{
				"deviceProperty": "<DeviceProperty3>",
				"propertyValue": "PropertyValue3"
			},
			{
				"deviceProperty": "<DeviceProperty4>",
				"propertyValue": "PropertyValue4"
			}
		]
		*/
	
		json_object *status4=json_object_new_object();
		json_object_object_add(status4, "deviceIdentify",json_object_new_string("DeviceIdentifyCode1"));
		json_object_object_add(status4,"informations",status3);
	
		json_object *status4A=json_object_new_array();
		json_object_array_add(status4A,status4);
		
		json_object *status4B=json_object_new_object();
		json_object_object_add(status4B, "deviceType",json_object_new_string("<DeviceType1>"));
		json_object_object_add(status4B,"idenfities",status4A);
		
		const char *status4B_str=json_object_to_json_string(status4B);
		
		printf("status4B_str=%s\n",status4B_str);
	
		/*
			{
			"deviceType": "<DeviceType1>",
			"idenfities": [
				{
					"deviceIdentify": "DeviceIdentifyCode1",
					"informations": [
						{
							"deviceProperty": "<DeviceProperty1>",
							"propertyValue": "PropertyValue1"
						},
						{
							"deviceProperty": "<DeviceProperty2>",
							"propertyValue": "PropertyValue2"
						}
						]
					}
			  ]
			}
	
		*/
		json_object *status6=json_object_new_object();
		json_object_object_add(status6, "deviceIdentify",json_object_new_string("DeviceIdentifyCode2"));
		json_object_object_add(status6,"informations",status5);
	
		json_object *status6A=json_object_new_array();
		json_object_array_add(status6A,status6);
	
		
		json_object *status6B=json_object_new_object();
		json_object_object_add(status6B, "deviceType",json_object_new_string("<DeviceType2>"));
		json_object_object_add(status6B,"idenfities",status6A);
		
		const char *status6B_str=json_object_to_json_string(status6B);
		
		printf("status6B_str=%s\n",status6B_str);
		/*
			{
			"deviceType": "<DeviceType2>",
			"idenfities": [
				{
					"deviceIdentify": "DeviceIdentifyCode2",
					"informations": [
						{
							"deviceProperty": "<DeviceProperty3>",
							"propertyValue": "PropertyValue3"
						},
						{
							"deviceProperty": "<DeviceProperty4>",
							"propertyValue": "PropertyValue4"
							}
						]
					}
				]
			}
		*/
		
	
		json_object *data=json_object_new_object();
		json_object *data1=json_object_new_array();
		json_object_array_add(data1,status4B);
		json_object_array_add(data1,status6B);
		json_object_object_add(mainjson,"data",data1);
	
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		/*
		{
		"header": {
			"action": [
				"REPORT_DTS_DEVICE_RUNTIME_INFO"
			],
			"requestId": [
				"bf77a870-1d28-4530-814d-8ec6fe8a9b5f"
			]
		},
		"data": [
			{
				"deviceType": "<DeviceType1>",
				"idenfities": [
					{
						"deviceIdentify": "DeviceIdentifyCode1",
						"informations": [
							{
								"deviceProperty": "<DeviceProperty1>",
								"propertyValue": "PropertyValue1"
							},
							{
								"deviceProperty": "<DeviceProperty2>",
								"propertyValue": "PropertyValue2"
							}
						]
					}
				]
			},
			{
				"deviceType": "<DeviceType2>",
				"idenfities": [
					{
						"deviceIdentify": "DeviceIdentifyCode2",
						"informations": [
							{
								"deviceProperty": "<DeviceProperty3>",
								"propertyValue": "PropertyValue3"
							},
							{
								"deviceProperty": "<DeviceProperty4>",
								"propertyValue": "PropertyValue4"
							}
						]
					}
				]
			}
		]
	}
		*/


		printf("%s\n",str);
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&reportDeviceRuntimeInfo_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		reportDeviceRuntimeInfo_string=&reportDeviceRuntimeInfo_buf[LWS_SEND_BUFFER_PRE_PADDING];

		json_object_put(mainjson);
		json_object_put(header);
		json_object_put(action);
		json_object_put(action1);
		json_object_put(requestId);
		json_object_put(status1);
		json_object_put(status11);
		json_object_put(status2);
		json_object_put(status22);
		json_object_put(status3);
		json_object_put(status4);
		json_object_put(status4A);
		json_object_put(status4B);
		json_object_put(status5);
		json_object_put(status6);
		json_object_put(status6A);
		json_object_put(status6B);
		json_object_put(data);
		json_object_put(data1);
		return reportDeviceRuntimeInfo_string;

}


/*
*2.1.4	����DTS�豸
*��������: unsigned char*
*����ֵ: json��ʽ����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/
static unsigned char* requestActiveDtsDevice()
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*requestActiveDtsDevice_string;
		unsigned char requestActiveDtsDevice_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		json_object *data=json_object_new_object();
		
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_LOG"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object *activeCode=json_object_new_array();
		json_object_array_add(activeCode,json_object_new_string("DTS Active code"));
		
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(action,"activeCode",activeCode);
		json_object_object_add(mainjson,"header",action);
	
	
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
				for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
				   L += sprintf((char *)&requestActiveDtsDevice_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		requestActiveDtsDevice_string=&requestActiveDtsDevice_buf[LWS_SEND_BUFFER_PRE_PADDING];
		json_object_put(action);
		json_object_put(action1);
		json_object_put(header);
		json_object_put(requestId);
		json_object_put(activeCode);
		json_object_put(mainjson);
		return requestActiveDtsDevice_string;
	
	
	/*�����ʽ
	{
		"header": {
			"action": [
				"REPORT_DTS_DEVICE_LOG"
			],
			"requestId": [
				"d4cbed46-3283-4ec3-b9e2-2bdde32914ee"
			],
			"activeCode": [
				"DTS Active code"
			]
		}
	}
	*/
	/*
	���ز���
	
	{
	"headers":{
		"action": ["REQUEST_ACTIVE_DTS_DEVICE"],
		"requestId": [""]	--���ֵ, ����������Ψһ. ���ڼ�������, ֧�ֵ��ַ� a-zA-Z0-9_-
	},
	 "statusCode":"<StatusCode>", ������
	 "data":{
		 "accessCode":"DTS Access Code" ����ɹ��� �� ������.
		 "failReason": "***", -- ʧ��ԭ��
	}
	}
	*/


}


/*
*2.1.5	�ϱ�RFID ˢ������
*��������: unsigned char*
*����ֵ: json��ʽ����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/
static unsigned char*reportDtsRfidCheckOnData()
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*reportDtsRfidCheckOnData_string;
		unsigned char reportDtsRfidCheckOnData_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REPORT_DTS_DEVICE_LOG"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
	
	
		json_object *data1=json_object_new_object();
		json_object_object_add(data1,
		   "cardId",json_object_new_string("RFID Card ID1"));
		json_object_object_add(data1,
		   "checkOnType",json_object_new_string("<CheckOnType1>"));
		json_object_object_add(data1,
		   "swipDatetime",json_object_new_string("Logdatafromdtsdevice1asdfaew1"));
	
		json_object *data2=json_object_new_object();
		json_object_object_add(data2,
		   "cardId",json_object_new_string("RFID Card ID2"));
		json_object_object_add(data2,
		   "checkOnType",json_object_new_string("<CheckOnType2>"));
		json_object_object_add(data2,
		   "swipDatetime",json_object_new_string("Logdatafromdtsdevice1asdfaew2"));
	
	
		json_object *data=json_object_new_array();
		json_object_array_add(data,data1);
		json_object_array_add(data,data2);
	
		json_object_object_add(mainjson,"data",data);
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
		   L += sprintf((char *)&reportDtsRfidCheckOnData_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		reportDtsRfidCheckOnData_string=&reportDtsRfidCheckOnData_buf[LWS_SEND_BUFFER_PRE_PADDING];

		json_object_put(action);
		json_object_put(action1);
		json_object_put(header);
		json_object_put(requestId);
		json_object_put(data);
		json_object_put(data1);
		json_object_put(data2);
		json_object_put(mainjson);
	
	return reportDtsRfidCheckOnData_string;
	/*�����ʽ
	{
		"header": {
			"action": [
				"REPORT_DTS_DEVICE_LOG"
			],
			"requestId": [
				"878e0a2c-a412-479b-b3e1-ccfdcafed02b"
			]
		},
		"data": [
			{
				"cardId": "RFID Card ID1",
				"checkOnType": "<CheckOnType1>",
				"swipDatetime": "Logdatafromdtsdevice1asdfaew1"
			},
			{
				"cardId": "RFID Card ID2",
				"checkOnType": "<CheckOnType2>",
				"swipDatetime": "Logdatafromdtsdevice1asdfaew2"
			}
		]
	}
	
	*/


}



/*
*
*2.1.6	����ͬ��DTS����
*��������: unsigned char*
*����ֵ: json��ʽ����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*
*/
static unsigned char* RequestSyncDtsAllConfigOptions(void)
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*RequestSyncDtsAllConfigOptions_string;
		unsigned char RequestSyncDtsAllConfigOptions_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REQUEST_SYNC_DTS_ALL_CONFIG_OPTIONS"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
		
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
			L += sprintf((char *)&RequestSyncDtsAllConfigOptions_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		RequestSyncDtsAllConfigOptions_string=&RequestSyncDtsAllConfigOptions_buf[LWS_SEND_BUFFER_PRE_PADDING];
		json_object_put(mainjson);
		json_object_put(header);
		json_object_put(action);
		json_object_put(action1);
		json_object_put(requestId);
		return RequestSyncDtsAllConfigOptions_string;

/*�����ʽ
{
    "header": {
        "action": [
            "REQUEST_SYNC_DTS_ALL_CONFIG_OPTIONS"
        ],
        "requestId": [
            "157ed038-1c6d-44f4-a036-1d52bb9314f3"
        ]
    }
}

*/


}

/*
*2.1.7	����ͬ��RFIDȨ��
*��������: unsigned char*
*����ֵ: json��ʽ����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/

static unsigned char* RequestSyncDtsAllRfidPermission(void)
{
		char struuidget[36];
		int L = 0;
		int n;
		unsigned char*RequestSyncDtsAllRfidPermission_string;
		unsigned char RequestSyncDtsAllRfidPermission_buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];
		json_object *mainjson=json_object_new_object();
		json_object *header=json_object_new_object();
		json_object *action=json_object_new_object();
		
		json_object *action1=json_object_new_array();
		json_object *requestId=json_object_new_array();
		json_object_array_add(action1,json_object_new_string("REQUEST_SYNC_DTS_ALL_RFID_PERMISSION"));
		uuidget(struuidget);//��ȡUUID��struuidget
		printf("uuidget = %s\n", struuidget);
		json_object_array_add(requestId,json_object_new_string(struuidget));
	
		json_object_object_add(action,"action",action1);
		json_object_object_add(action,"requestId",requestId);
		json_object_object_add(mainjson,"header",action);
		
		const char *str=json_object_to_json_string(mainjson);
		printf("%s\n",str);
		
		for (n = 0; n < 1; n++)//ת����libwebsocket ���Է��͵�����
			L += sprintf((char *)&RequestSyncDtsAllRfidPermission_buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(mainjson));
		RequestSyncDtsAllRfidPermission_string=&RequestSyncDtsAllRfidPermission_buf[LWS_SEND_BUFFER_PRE_PADDING];
		json_object_put(mainjson);
		json_object_put(header);
		json_object_put(action);
		json_object_put(action1);
		json_object_put(requestId);
		return RequestSyncDtsAllRfidPermission_string;
	
	/*�����ʽ
	{
		"header": {
			"action": [
				"REQUEST_SYNC_DTS_ALL_CONFIG_OPTIONS"
			],
			"requestId": [
				"157ed038-1c6d-44f4-a036-1d52bb9314f3"
			]
		}
	}
	
	*/

}


/*
* JSON��ʽ���ݲ���
*��������: unsigned char*
*����ֵ: json��ʽ����
*/

static unsigned char* json_text_test(void)
{	
	struct json_object *my_string;
	int L = 0;
	int n;
	unsigned char*json_string;
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
								  LWS_SEND_BUFFER_POST_PADDING];

	my_string = json_object_new_string("mleafhx");

	/*��� my_string=   */
	printf("my_string=%s\n", json_object_get_string(my_string));
	/*ת��json��ʽ�ַ��� ���my_string.to_string()="\t"*/
	printf("my_string.to_string()=%s\n", json_object_to_json_string(my_string)); 
	for (n = 0; n < 1; n++)
		   L += sprintf((char *)&buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s",json_object_to_json_string(my_string));
	json_string=&buf[LWS_SEND_BUFFER_PRE_PADDING];
	/*�ͷ���Դ*/
	json_object_put(my_string);

	return json_string;

}
/*
*text�ı����ݲ���
*��������: unsigned char*
*����ֵ: json��ʽ����
*/

static unsigned char* text_send_test(void)
{
		int L = 0;
		int n;
		unsigned char*text_string;
		unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 4096 +
									  LWS_SEND_BUFFER_POST_PADDING];

	for (n = 0; n < 1; n++)
				L+= sprintf((char *)&buf[LWS_SEND_BUFFER_PRE_PADDING + L],"%s","MLEAF hexi 350983773@qq.com");
	text_string=&buf[LWS_SEND_BUFFER_PRE_PADDING];
	return text_string;

}
/*json��ʽ������غ���*/
static void json_print_value(json_object *obj);
static void json_print_array(json_object *obj);
static void json_print_object(json_object *obj);

/*
*��ӡJSON���ݵ�ֵ
*��������: none
*����ֵ: none
*�������json_object *����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/ 
static void json_print_value(json_object *obj) 
{
	char *action="CONFIG_DTS_DEVICE_OPTIONS";
	char *configOption="CLOUD_PLATFORM_WEBSOCKET_URL";

	if(!obj) return;
	json_type type=json_object_get_type(obj);
	if(type == json_type_boolean) {
	    if(json_object_get_boolean(obj)) {
	        printf("true");
	    } else {
	        printf("false");
	    }
	} else if(type == json_type_double) {
	    printf("json_object_get_double=%lf\n",json_object_get_double(obj));
	} else if(type == json_type_int) {
	    printf("json_object_get_int=%d\n",json_object_get_int(obj));
	} 
	else if(type == json_type_string) 
	{
	    
		if(strcmp(json_object_get_string(obj),action)==0)//�ж��Ƿ�ΪCONFIG_DTS_DEVICE_OPTIONS
		{
			printf("action checking out\n");
			printf("action=%s\n",json_object_get_string(obj));
		}
		else if(strcmp(json_object_get_string(obj),configOption)==0)//�ж��Ƿ�ΪCLOUD_PLATFORM_WEBSOCKET_URL
		{
			printf("configOption checking out\n");
			printf("configOption=%s\n",json_object_get_string(obj));

		}
		else
		{
			printf("json_object_get_string=%s\n",json_object_get_string(obj));
		}
	} 
	else if(type == json_type_object) {
	    json_print_object(obj);
	} else if(type == json_type_array) {
	    json_print_array(obj);
	} else {
	    printf("ERROR");
	}
	printf(" ");
}
/*
*��ӡJSON�������͵�ֵ
*��������: none
*����ֵ: none
*�������json_object *����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/ 
static void json_print_array(json_object *obj) 
{
	int i;

	if(!obj) return;
    	
    int length=json_object_array_length(obj);
    for(i=0;i<length;i++) 
	{
        json_object *val=json_object_array_get_idx(obj,i);
        json_print_value(val);
    }
}
/*
*��ӡJSON object���͵�ֵ
*��������: none
*����ֵ: none
*�������json_object *����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*/ 
static void json_print_object(json_object *obj) 
{
	char *configData="configData";
	char *requestId="requestId";
	
	char *delim=":/,";//�ָ��ַ���
	char *p;
	char *s;
	char *name,*value,*next,*value1,*next1,*next2;
	int i;
	char buff[50],buff2[50];
	char *valueaddr1,*valueaddr2;
	char *use_ssl_get;
	if(!obj) return;
	//����json�����key��ֵ 
	//Linux�ں�2.6.29��˵����strtok()�Ѿ�����ʹ�ã����ٶȸ����strsep()���档
	json_object_object_foreach(obj,key,val) 
	{
		//printf("%s => ",key);
		if(strcmp(key,configData)==0)//ȡ��ͨѶ��url
		{
			printf("configData checking out\n");
			printf("configData=%s\n",json_object_get_string(val));
			
			s=(char*)json_object_get_string(val);
			printf("%s\n",s);
//wss: //area1.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0,wss: //primary.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0
			
			value = strdup(s);

			for(i=0 ;i<2 ;i++)
			{ // ��һ��ִ��ʱ
				name = strsep(&value,":"); // ��":"�ָ��ַ���,��ʱstrsep��������ֵΪ "wss",��":"��֮ǰ���ַ���
				next =value; // ��ʱָ��valueָ��":"�ź�����ַ���,�� //area1.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0,wss: //primary.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0
				printf(" name= %s\n",name); //��ӡ��һ�ַָ��name��ֵ
				use_ssl_get=name;
				name = strsep(&value,"/");// ��ʱͨ��"/"�ָ��ַ���
				next =value; 
				name = strsep(&value,"/");//ȥ���ڶ���/
				next =value; 
				if(i==0)
				{
					value=strsep(&next,",");// ��","�ָ��ַ���,��ʱstrsep��������ֵΪ "area1.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0",��","��֮ǰ���ַ���
					printf("value= %s\n",value);
					next2 =value;
					value=strsep(&next2,"/");
					printf("value= %s\n",value);//area1.dts.mpush.brxy-cloud.com
					valueaddr1=value;
					sprintf(buff2,"/%s",next2);
					printf("next2= %s\n",buff2);/* /websocket/connHandler/v2.0	 */ 
				}
				if(i==1)//�ڶ���ѭ��
				{
					value1=strsep(&next,"");// ��/0�ָ��ַ���,��ʱstrsep��������ֵΪ "primary.dts.mpush.brxy-cloud.com/websocket/connHandler/v2.0",��wss: //֮��"/0"֮ǰ���ַ���
					printf("value1= %s\n",value1);
					next1 =value1;
					value1=strsep(&next1,"/");
					printf("value2= %s\n",value1);//primary.dts.mpush.brxy-cloud.com
					valueaddr2=value1;
					sprintf(buff,"/%s",next1);
					printf("next1= %s\n",buff);/* /websocket/connHandler/v2.0  */

				}
				
				value=next;
			}
			printf("use_ssl_get= %s\n",use_ssl_get);
			updata_ipaddr_data(valueaddr1,valueaddr2,buff2,buff,use_ssl_get);//�����յ���ͨ��url���µ����ݿ���
			
//			char *source = strdup(s); 
//			char *token;  
//			for(token = strsep(&source, delim); token != NULL; token = strsep(&source, delim)) 
//			{  
//				printf("%s",token);  
//				printf("\n");  
//			}
		}
		else if (strcmp(key,requestId) == 0)//ȡ��requestId ��	
		{
				array_list* arr = json_object_get_array(val);
				json_object* obj = (json_object*)array_list_get_idx(arr,0);
				printf("requestId checking out\n");
				printf("requestId=%s\n", json_object_get_string(obj));
				serial_test_send(json_object_get_string(obj));//��requestId���ô��ڷ��ͳ�ȥ
		}
	
		json_print_value(val);
	}
}

static int Process_json(json_object *new_obj)
{

	printf("new_obj.to_string()=%s\n", json_object_to_json_string(new_obj));
	printf("Result is %s\n", (new_obj == NULL) ? "NULL (error!)" : "not NULL");
	if (!new_obj)
		return 1; // oops, we failed.

	json_print_object(new_obj);//����json�����key��ֵ ����json���� 

	json_object_put(new_obj);/*�ͷ���Դ*/

}

/* lws-mirror_protocol �ص�����*/
static int
callback_DTS2B_mirror(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	
		
		int n;
		int num;
		json_object *get_obj;
	switch (reason) {

	//when the websocket session ends
	case LWS_CALLBACK_CLOSED:
		fprintf(stderr, "mirror: LWS_CALLBACK_CLOSED mirror_lifetime=%d\n", mirror_lifetime);
		wsi_mirror = NULL;
		was_closed = 1;
		break;
	//after your client connection completed a handshake with the remote server 
	case LWS_CALLBACK_CLIENT_ESTABLISHED:
	
		fprintf(stderr, "LWS_CALLBACK_CLIENT_ESTABLISHED\n");

		/*
		 * start the ball rolling,
		 * LWS_CALLBACK_CLIENT_WRITEABLE will come next service
		 */

		libwebsocket_callback_on_writable(context, wsi);
		break;
	//data has appeared from the server for the client connection, it can be found at *in and is len bytes long 
	case LWS_CALLBACK_CLIENT_RECEIVE://���շ������Ϣ
		((char *)in)[len] = '\0';
		fprintf(stderr, "Received %d '%s'\n", (int)len, (char *)in); //��ӡ���յ�������
		get_obj=json_tokener_parse((char *)in);//�����յ����ַ���ת��Ϊjson object
		Process_json(get_obj);//����������յ�������
		break;
	/*

	If you call libwebsocket_callback_on_writable on a connection, 
	you will get one of these callbacks coming when the connection socket is able to accept another write packet without blocking. 
	If it already was able to take another packet without blocking, 
	you'll get this callback at the next call to the service loop function. 
	Notice that CLIENTs get LWS_CALLBACK_CLIENT_WRITEABLE and servers get LWS_CALLBACK_SERVER_WRITEABLE.

	*/
	//������д����
	
	case LWS_CALLBACK_CLIENT_WRITEABLE:

		
		num=strlen(text_send_test());//�����ݳ���

		n = libwebsocket_write(wsi,text_send_test(), num,LWS_WRITE_TEXT);//�����ı�����
		//n = libwebsocket_write(wsi,json_text_test(), num,  LWS_WRITE_TEXT);//����JSON ��������
		//n = libwebsocket_write(wsi,reportDeviceLog(), num,LWS_WRITE_TEXT);//����DTS��־
		//n = libwebsocket_write(wsi,reportDeviceStatus(), num,LWS_WRITE_TEXT);//����DTS״̬
		//n = libwebsocket_write(wsi,reportDeviceRuntimeInfo(), num,LWS_WRITE_TEXT);//�ϱ�DTS������Ϣ
		//n = libwebsocket_write(wsi,requestActiveDtsDevice(), num,LWS_WRITE_TEXT);//����DTS�豸
		//n = libwebsocket_write(wsi,reportDtsRfidCheckOnData(), num,LWS_WRITE_TEXT);//�ϱ�RFID ˢ������
		//n = libwebsocket_write(wsi,RequestSyncDtsAllConfigOptions(), num,LWS_WRITE_TEXT);//����ͬ��DTS����
		//n = libwebsocket_write(wsi,RequestSyncDtsAllRfidPermission(), num,LWS_WRITE_TEXT);//����ͬ��RFIDȨ��
		
		printf("Send success %s\n", text_send_test());//��ӡ������Ϣ
		if (n < 0)
		return -1;

		/* get notified as soon as we can write again */
		libwebsocket_callback_on_writable(context, wsi);
		sleep(3);//ÿ��3�뷢��һ������
		//was_closed = 1;//�ر�websocketͨѶ
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */
/*
struct libwebsocket_protocols {
    const char * name;
    callback_function * callback;
    size_t per_session_data_size;
    size_t rx_buffer_size;
    struct libwebsocket_context * owning_server;
    int protocol_index;
};
name
    Protocol name that must match the one given in the client Javascript new WebSocket(url, 'protocol') name 
callback
    The service callback used for this protocol. It allows the service action for an entire protocol to be encapsulated in the protocol-specific callback 
���ڴ�Э��Ļص���������������װ��Э���ض��ص�����������Ϊ����Э��
per_session_data_size
    Each new connection using this protocol gets this much memory allocated on connection establishment and freed on connection takedown. A pointer to this per-connection allocation is passed into the callback in the 'user' parameter 

rx_buffer_size
    if you want atomic frames delivered to the callback, you should set this to the size of the biggest legal frame that you support. If the frame size is exceeded, there is no error, but the buffer will spill to the user callback when full, which you can detect by using libwebsockets_remaining_packet_payload. Notice that you just talk about frame size here, the LWS_SEND_BUFFER_PRE_PADDING and post-padding are automatically also allocated on top.
owning_server
    the server init call fills in this opaque pointer when registering this protocol with the server. 

protocol_index
    which protocol we are starting from zero 

Description

    This structure represents one protocol supported by the server. An array of these structures is passed to libwebsocket_create_server allows as many protocols as you like to be handled by one server. 
    

*/
static struct libwebsocket_protocols protocols[] = {
	{
		"lws-mirror-protocol",
		callback_DTS2B_mirror,
		0,
		0,
	},
	{ NULL, NULL, 0, 0 } /* end */
};

/*���ڴ�����غ���*/

void signal_handler_IO(int status);

/*****************************
*�򿪴��ڲ���ʼ������
*Author  mleaf_hexi
*Mail:350983773@qq.com
*****************************/ 
init_serial(void)  
{  
	struct sigaction saio; /*definition of signal axtion */
	
	serial_fd = open(DEVICE, O_RDWR | O_NOCTTY | O_NDELAY);  
	if (serial_fd < 0) {  
	    perror("open");  
	    return -1;  
	}  
      
    //������Ҫ���ýṹ��termios <termios.h>  
    struct termios options;  
      
    /**1. tcgetattr�������ڻ�ȡ���ն���صĲ����� 
    *����fdΪ�ն˵��ļ������������صĽ��������termios�ṹ���� 
    */  
    tcgetattr(serial_fd, &options);  
    /**2. �޸�����õĲ���*/  
    options.c_cflag |= (CLOCAL | CREAD);//���ÿ���ģʽ״̬���������ӣ�����ʹ��  
    options.c_cflag &= ~CSIZE;//�ַ����ȣ���������λ֮ǰһ��Ҫ�������λ  
    options.c_cflag &= ~CRTSCTS;//��Ӳ������  
    options.c_cflag |= CS8;//8λ���ݳ���  
    options.c_cflag &= ~CSTOPB;//1λֹͣλ  
    options.c_iflag |= IGNPAR;//����ż����λ  
    options.c_oflag = 0; //���ģʽ  
    options.c_lflag = 0; //�������ն�ģʽ  
    cfsetospeed(&options, B115200);//���ò�����  
      
    /**3. ���������ԣ�TCSANOW�����иı�������Ч*/  
    tcflush(serial_fd, TCIFLUSH);//������ݿ��Խ��գ�������  
    tcsetattr(serial_fd, TCSANOW, &options);  
#if 0
    //�ź��������
	/* install the signal handle before making the device asynchronous*/
	saio.sa_handler = signal_handler_IO;
	sigemptyset(&saio.sa_mask);
	//saio.sa_mask = 0; ������sigemptyset������ʼ��act�ṹ��sa_mask��Ա 

	saio.sa_flags = 0;
	saio.sa_restorer = NULL;
	sigaction(SIGIO,&saio,NULL);

	/* allow the process to recevie SIGIO*/
	fcntl(serial_fd,F_SETOWN,getpid());
	/* Make the file descriptor asynchronous*/
	fcntl(serial_fd,F_SETFL,FASYNC);
#endif
    return 0;  
}  
  
/************************ 
*���ڷ������� 
*@fd:���������� 
*@data:���������� 
*@datalen:���ݳ��� 
*Author  mleaf_hexi
*Mail:350983773@qq.com
*************************/  
int uart_send(int fd, char *data, int datalen)  
{  
  int len = 0;	
  len = write(fd, data, datalen);//ʵ��д��ĳ���  
  if(len == datalen) {	
	  return len;  
  } else {	
	  tcflush(fd, TCOFLUSH);//TCOFLUSHˢ��д������ݵ�������  
	  return -1;  
  }  
	
  return 0;  
} 
/************************ 
*���ڷ������� ����
*Author  mleaf_hexi
*Mail:350983773@qq.com
*************************/

void serial_test_send(char *buf1)
{

  int i,len;
  int num;
 
  //char buf1[]="hello world mleaf test";  
 num=strlen(buf1);//�����ݳ���
 uart_send(serial_fd, buf1, num);  //���ڷ�������
 send_wait_flag=TRUE_SERIAL;
 printf("\n");	
 
}
/************************ 
*���ڽ������� ���� ͨ��signal���ƶ�ȡ����
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************/

int serial_test_receive(void){
	char buf[255];
	int res;
	int i=0;
	
	if(wait_flag == FALSE_SERIAL)
    {	
		memset(buf,0,255);
		while(i!= 8)
			{
				res = read(serial_fd,buf+i,8-i);
				i+=res;
			}
		printf("res=%d\n",res);
		printf("Serial Received=%d,%s\n",res,buf);
		wait_flag = TRUE_SERIAL; /*wait for new input*/
		return 1;
	}
	else
	{
		return 0;
	}

}
/************************************************************************ 
*���ڽ������� ���� ͨ��selectϵͳ���ý���io��·�л���ʵ���첽��ȡ��������
*Version 1.0 
*Created on: 2015-2-2 
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************************************************************/
void serial_select_receive(void){

	char buf[13];
	char buff[256];
	char *buf3;
	int flag=0;
	fd_set rd;  
  	int nread = 0;
	int count = 0;
	printf("serial select receive test\n");  
	FD_ZERO(&rd);  
	FD_SET(serial_fd, &rd); 
	memset(buf, 0 , sizeof(buf));

/*
FD_ISSET:
int FD_ISSET(int fd,fd_set *fdset) /*is the bit for fd on in fdset
�ж�������fd�Ƿ��ڸ�������������fdset�У�ͨ�����select����ʹ�ã�
����select�����ɹ�����ʱ�Ὣδ׼���õ�������λ���㡣
ͨ������ʹ��FD_ISSET��Ϊ�˼����select�������غ�
ĳ���������Ƿ�׼���ã��Ա���н������Ĵ��������
��������fd����������fdset�з��ط���ֵ�����򣬷����㡣
*/

while(FD_ISSET(serial_fd, &rd))  
	   {  
		   if(select(serial_fd+1, &rd, NULL,NULL,NULL) < 0)  
		   {  
			   perror("select error\n");  
		   }  
		   else  
		   {  
		   	while((nread = read(serial_fd, buf, sizeof(buf))) > 0)
			   	{
						printf("\nLen %d\n",nread);
						printf("nread = %d,%s\n",nread, buf);
						memcpy(&buff[count],buf,nread);
						count+=nread;
					if(count==13)//�жϴ��ڽ��յ������Ƿ�Ϊ13λ
					{	
						
						buff[count+1] = '\0';
						printf("buff = %s\n", buff);
						count=0;//�������� ��Ȼֻ�ܽ���һ������
					}
//					else{
//						count=0;//�������� ��Ȼֻ�ܽ���һ������
//					}
			   	}
		  
			} 
		}  
}


/****************************************************
*�źŴ��������豸wait_flag=FASLE
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*
******************************************************/
void signal_handler_IO(int status)
{
   printf("received SIGIO signale.\n");
  
   if(send_wait_flag==TRUE_SERIAL)//�ж��Ƿ�Ϊ���Ͳ������ź�
   {
	   wait_flag = TRUE_SERIAL;//��Ϊ�����źŹʲ����н������ݴ���
	   send_wait_flag=FALSE_SERIAL;//���ͱ�־λ����
	   printf("wait_flag = TRUE_SERIAL\n");

   }
   else//������Ƿ��Ͳ������ź����ǽ��յ�����
   {
	 wait_flag = FALSE_SERIAL;//��־λ��λ��ʼ��������
	 printf("wait_flag=FALSE_SERIAL\n");
   }

}

/************************ 
*���̴߳�����
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************/
/********************************************************************************************************
���̵߳��Լ�¼:
1:ʹ���źŴ�����ʽ���մ��� �ᵼ�¶��߳�ֻ����һ�ξͲ��ܽ��մ��������ˡ�Ŀǰԭ������
2:���̺߳�����ͨ��selectϵͳ���ý���io��·�л���ʵ���첽��ȡ�������ݣ����ȶ��������ݣ����ִ��ڲ�����
*********************************************************************************************************/
void* Serial_Thread(void)
{	
	char buf[255];
	int res;

	printf("Serial Thread Test\n");
#if 0	
	if(wait_flag == FALSE_SERIAL)
	   {	

			memset(buf,0,255);
			res = read(serial_fd,buf,255);
			printf("res=%d\n",res);
			printf("Serial Received=%d,%s\n",res,buf);
			wait_flag = TRUE_SERIAL; /*wait for new input*/
		}
#endif
	while(1)
	{
		serial_select_receive();//select ��ʽ�첽��ȡ��������
	}


}
/************************ 
*websocket ���Ӻ���
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************/
int libwebsockets_connect_tryagain(void)
{	
	
	char cert_path[1024];
	char key_path[1024];

	int use_ssl = 0;
	int ret = 0;
	int connect_error=0;
	int ietf_version = -1; /* latest */
	char *use_ssl_ws="ws";
	char *use_ssl_wss="wss";

	struct lws_context_creation_info info;
	struct libwebsocket *wsi_dumb;
	//	int port = 8543;
		int port = 9000;
	memset(&info, 0, sizeof info);
	
	if(strcmp(use_ssl_set,use_ssl_ws)==0)//�ж��Ƿ�ʹ�ü���
		{
			use_ssl=0;
			printf("use_ssl = %d\n",use_ssl);
			printf("We do not use ssl\n");
			
		}
	else if(strcmp(use_ssl_set,use_ssl_wss)==0)
		{
			use_ssl=2;
			printf("use_ssl = %d\n",use_ssl);
		}

	/*
	 * create the websockets context.  This tracks open connections and
	 * knows how to route any traffic and which protocol version to use,
	 * and if each connection is client or server side.
	 *
	 * For this client-only demo, we tell it to not listen on any port.
	 */

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	/****************
	*****use ssl******
	*****************/
	if (!use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	} else {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}


	info.gid = -1;
	info.uid = -1;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		return 1;
	}

	/* create a client websocket using dumb increment protocol */
	/*

TITLE:	libwebsocket_client_connect - Connect to another websocket server

	struct libwebsocket * libwebsocket_client_connect (struct libwebsocket_context * context, const char * address, int port, int ssl_connection, const char * path, const char * host, const char * origin, const char * protocol, int ietf_version_or_minus_one)

	Arguments

	context
	    Websocket context 
	address
	    Remote server address, eg, "myserver.com" 
	port
	    Port to connect to on the remote server, eg, 80 
	ssl_connection
	    0 = ws://, 1 = wss:// encrypted, 2 = wss:// allow self signed certs 
	path
	    Websocket path on server 
	host
	    Hostname on server 
	origin
	    Socket origin name 
	protocol
	    Comma-separated list of protocols being asked for from the server, or just one. The server will pick the one it likes best. 
	ietf_version_or_minus_one
	    -1 to ask to connect using the default, latest protocol supported, or the specific protocol ordinal 

	Description

	    This function creates a connection to a remote server 

	*/
	fprintf(stderr, "Connecting to %s:%u\n", get_ipaddr2, port);
	wsi_dumb = libwebsocket_client_connect(context, get_ipaddr2, port, use_ssl,
			get_ipindex2, get_ipaddr2,"origin",
			 protocols[0].name, ietf_version);

	if (wsi_dumb == NULL) {
		fprintf(stderr, "libwebsocket connect failed\n");
		ret = 1;
		//goto bail;
		was_closed=1;//�ر�socketͨ��
	}

	fprintf(stderr, "Waiting for connect...\n");
	return 0;

}
/****************************************************************************** 
*websocket ���Ӻ��� �������ʧ�������libwebsockets_connect_tryagain ���Եڶ�������
*Author:  mleaf_hexi
*Mail:350983773@qq.com
********************************************************************************/

int libwebsockets_connect_server(void)
{
	char cert_path[1024];
	char key_path[1024];

	int use_ssl = 0;
	int ret = 0;
	int connect_error=0;
	int ietf_version = -1; /* latest */
	char *use_ssl_ws="ws";
	char *use_ssl_wss="wss";

	struct lws_context_creation_info info;
	struct libwebsocket *wsi_dumb;
	//	int port = 8543;
		int port = 9000;
	memset(&info, 0, sizeof info);
	
	if(strcmp(use_ssl_set,use_ssl_ws)==0)//�ж��Ƿ�ʹ�ü���
		{
			use_ssl=0;
			printf("use_ssl = %d\n",use_ssl);
			printf("We do not use ssl\n");
			
		}
	else if(strcmp(use_ssl_set,use_ssl_wss)==0)
		{
			use_ssl=2;
			printf("use_ssl = %d\n",use_ssl);
		}
	/*
	 * create the websockets context.  This tracks open connections and
	 * knows how to route any traffic and which protocol version to use,
	 * and if each connection is client or server side.
	 *
	 * For this client-only demo, we tell it to not listen on any port.
	 */

	info.port = CONTEXT_PORT_NO_LISTEN;
	info.protocols = protocols;
#ifndef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	/********************
	*****use ssl**********
	*********************/
	if (!use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	} else {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}


	info.gid = -1;
	info.uid = -1;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		fprintf(stderr, "Creating libwebsocket context failed\n");
		return 1;
	}

	/* create a client websocket using dumb increment protocol */
	/*

TITLE:	libwebsocket_client_connect - Connect to another websocket server

	struct libwebsocket * libwebsocket_client_connect (struct libwebsocket_context * context, const char * address, int port, int ssl_connection, const char * path, const char * host, const char * origin, const char * protocol, int ietf_version_or_minus_one)

	Arguments

	context
		Websocket context 
	address
		Remote server address, eg, "myserver.com" 
	port
		Port to connect to on the remote server, eg, 80 
	ssl_connection
		0 = ws://, 1 = wss:// encrypted, 2 = wss:// allow self signed certs 
	path
		Websocket path on server 
	host
		Hostname on server 
	origin
		Socket origin name 
	protocol
		Comma-separated list of protocols being asked for from the server, or just one. The server will pick the one it likes best. 
	ietf_version_or_minus_one
		-1 to ask to connect using the default, latest protocol supported, or the specific protocol ordinal 

	Description

		This function creates a connection to a remote server 

	*/
	
	fprintf(stderr, "Connecting to %s:%u\n", get_ipaddr1, port);
	wsi_dumb = libwebsocket_client_connect(context, get_ipaddr1, port, use_ssl,
			get_ipindex1, get_ipaddr1,"origin",
			 protocols[0].name, ietf_version);

	if (wsi_dumb == NULL) {
		fprintf(stderr, "libwebsocket connect failed\n");
		//ret = 1;
		//goto bail;
		connect_error=1;
		
	}
	if(connect_error==1)
	{	
		printf("Use other url connection\n");
		connect_error=0;
		libwebsockets_connect_tryagain();//��һ��URL����ʧ�ܺ����libwebsockets_connect_tryagain�����õڶ���URL����
	}
	fprintf(stderr, "Waiting for connect...\n");
	return 0;

}
/************************ 
*������
*Author:  mleaf_hexi
*Mail:350983773@qq.com
*************************/

int main(int argc, char **argv)
{
	int n = 0;

	//���̶߳���
	pthread_t Serial_ID;
			
	char buf1[]="hello world mleaf test";  //���ڲ���Ҫ���͵�����
	init_serial();	 /*��ʼ�����ڴ��豸�ڵ�*/
	sqlite3_init();//sqlite3���ݿ��ʼ��
	time_get_init();//��ʼʹ��ʱ���ʼ��
	get_start_time();//��ȡ��ʼʹ��ʱ��
	get_sqlite3_ipaddr_data();//��ȡIP��ַ��Ϣ��	

	fprintf(stderr, "DTS2B websockets client\n"
			"(C) Copyright 2014-2015 Mleaf_HEXI <350983773@qq.com>"
			"licensed under LGPL2.1\n");

	libwebsockets_connect_server();//���ӷ����
	
	serial_test_send(buf1);//���ڷ��Ͳ���

	if (!fork())
	{
		pthread_create(&Serial_ID, NULL, &Serial_Thread, NULL);//�����߳�

		/*
		 * sit there servicing the websocket context to handle incoming
		 * packets, and drawing random circles on the mirror protocol websocket
		 * nothing happens until the client websocket connection is
		 * asynchronously established
		 */
		 //�ӽ���
		while(1)
		{
			n = 0;
			while (n >= 0 && !was_closed && !force_exit) 
				{
					n = libwebsocket_service(context, 100);
					
					if (n < 0)
						continue;

					if (wsi_mirror)
						continue;		
				}
			goto bail;//�˳�ͨ��
		}
	}
bail:
	get_end_time();//��ȡ�������ʹ��ʱ��
	time_get_end();//�洢�����ݿ���
	fprintf(stderr, "websocket Exiting\n");
	close(serial_fd);//�رմ���
	quit_sqlite3();//�ر�SQLITE3���ݿ�
	libwebsocket_context_destroy(context);//�˳�websocket
	return 1;
}
