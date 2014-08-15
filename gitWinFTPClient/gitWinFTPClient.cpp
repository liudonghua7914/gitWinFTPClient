// gitWinFTPClient.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include<Winsock2.h>
#include<stdio.h> 
#pragma comment(lib,"ws2_32.lib")

#define  CMD_USER	"USER \r\n"
#define  CMD_PASS	"PASS \r\n"
#define	 CMD_PORT	"PROT 192.168.8.209 212,29\r\n"	//"PORT \r\n"
#define  CMD_LIST	"LIST \r\n"
#define  CMD_PWD	"PWD \r\n"

enum FTP_STATUS
{
	eSTAT = 0,
	eUSER,
	ePASS,
	eLOGIN,
	ePWD,
	ePORT,
	eLIST,
	eMAX
};

typedef struct  
{
	ULONG net_ip;
	UINT16 port;
	BYTE status;
	
	SOCKET sftpClient;
	WSADATA sftpClientData;
	SOCKADDR_IN saftpClient;
	char clientRecvBuf[100];
	HANDLE hFTPClientThread;
	BOOL bKillFTPClientThread;

	SOCKET sftpSerives;
	SOCKET sftpNewSerives;
	WSADATA sftpSerivesData;
	SOCKADDR_IN saftpSerives;
	SOCKADDR_IN remoteSerives;
	char SerivesRecvBuf[100];
	HANDLE hFTPSeriversThread;
	BOOL bKillFTPSeriversThread;
	
	
}FTP_INFO;

FTP_INFO ftpInfo;
FTP_INFO *pftpInfo;
/*************************************************************************************************************************
**函数名称：	fptSend
**函数功能:
**入口参数:
**返回参数:
*************************************************************************************************************************/
void fptSend(SOCKET ssocket,const char *p,UINT16 len)
{
	int ret;
	int nLeft;
	int idx;
	nLeft = len;
	idx = 0;
	while(nLeft > 0)
	{
		ret = send(ssocket,&p[idx],nLeft,0);
		if (ret == SOCKET_ERROR)
		{
			printf("\r\n send error");
		}
		nLeft -= ret;
		idx += ret;
	}
}
/*************************************************************************************************************************
**函数名称：	fptCmdSend
**函数功能:
**入口参数:
**返回参数:
***************************************************************************************************************************/
void fptCmdSend(const char *p,UINT16 len)
{
	printf("\r\n fptCmd  %s ",p);
	fptSend(pftpInfo->sftpClient,p,len);
}
/*************************************************************************************************************************
**函数名称：	ThreadFTPSeriversFunc
**函数功能:
**入口参数:
**返回参数:
*************************************************************************************************************************/
DWORD WINAPI ThreadFTPSeriversFunc(LPVOID arg)
{
	int ret;
	pftpInfo->sftpSerives = socket(AF_INET,SOCK_STREAM,0);
	if(INVALID_SOCKET == pftpInfo->sftpSerives)
	{
		printf("\r\n sftpSerives Tell the user that socket INVALID_SOCKET ");
		WSACleanup();
	}
	pftpInfo->saftpSerives.sin_family = AF_INET;
	pftpInfo->saftpSerives.sin_addr.S_un.S_addr = inet_addr("192.168.8.209");
	pftpInfo->saftpSerives.sin_port =  htons(54301);
	if(INVALID_SOCKET == bind(pftpInfo->sftpSerives,(SOCKADDR *)&pftpInfo->saftpSerives,sizeof(pftpInfo->saftpSerives)))
	{
		printf("\r\n sftpSerives Tell the user that bind INVALID_SOCKET ");
	}
	listen(pftpInfo->sftpSerives,8);
	int addrlen = sizeof(pftpInfo->remoteSerives);
	pftpInfo->sftpNewSerives = accept(pftpInfo->sftpSerives,(SOCKADDR *)&pftpInfo->remoteSerives,&addrlen);
	while(!pftpInfo->bKillFTPSeriversThread)
	{
		
		
		if(INVALID_SOCKET == pftpInfo->sftpNewSerives)
		{
			printf("\r\n sftpSerives Tell the user that accept INVALID_SOCKET ");
		}
		memset(pftpInfo->SerivesRecvBuf,'\0',sizeof(pftpInfo->SerivesRecvBuf));
		ret = recv(pftpInfo->sftpNewSerives,pftpInfo->SerivesRecvBuf,sizeof(pftpInfo->SerivesRecvBuf) - 1,0);
		if(INVALID_SOCKET == ret)
		{
			printf("\r\n sftpSerives Tell the user that accept INVALID_SOCKET ");
			pftpInfo->bKillFTPSeriversThread = TRUE;
		}
		else
		{
			if(ret > 0)
			{
				printf("sftpSerives rev:  %s ",pftpInfo->SerivesRecvBuf);
			}
		}
	}
	printf("\r\n sftpSerives closesocket");
	closesocket(pftpInfo->sftpSerives);
	WSACleanup();
	CloseHandle(pftpInfo->hFTPSeriversThread);
	return 0;
}
/*************************************************************************************************************************
**函数名称：	fptClientProcess
**函数功能:
**入口参数:
**返回参数:
*************************************************************************************************************************/
void fptClientProcess(const char *p,UINT16 len)
{
	printf("\r\n Process: %s ",p);
	switch(pftpInfo->status)
	{
		case eSTAT:		if (!memcmp(p,"220",strlen("220")))
						{
							fptCmdSend(CMD_USER,strlen(CMD_USER));
							pftpInfo->status = eUSER;
						}
						break;

		case eUSER:		if (!memcmp(p,"331",strlen("331")))
						{
							fptCmdSend(CMD_PASS,strlen(CMD_PASS));
							pftpInfo->status = ePASS;
						}
						break;

		case ePASS:		if (!memcmp(p,"230",strlen("230")))
						{
							fptCmdSend(CMD_PWD,strlen(CMD_PWD));
							pftpInfo->status = ePWD;
							printf("\r\n listen 192.168.8.209 212,29 port 54301");
							pftpInfo->hFTPSeriversThread = CreateThread(NULL,0,ThreadFTPSeriversFunc,NULL,0,NULL);
							if(NULL == pftpInfo->hFTPSeriversThread)
							{
								printf("\r\n CreateThread hFTPSeriversThread Fail");
							}						
						}
						break;
		case ePWD:		fptCmdSend(CMD_PORT,strlen(CMD_PORT));
						pftpInfo->status = ePORT;
						break;

		case ePORT:		fptCmdSend(CMD_LIST,strlen(CMD_LIST));
						pftpInfo->status = eLIST;
						break;
		case eLIST:		
						pftpInfo->status = eMAX;
						break;
		default:
						break;
	}
}
/*************************************************************************************************************************
**函数名称：	ThreadFTPClientFunc
**函数功能:
**入口参数:
**返回参数:
*************************************************************************************************************************/
DWORD WINAPI ThreadFTPClientFunc(LPVOID arg)
{
	WORD dwVersion;
	int err;
	int ret = 0;
	
	dwVersion = MAKEWORD(2,2);
	err = WSAStartup(dwVersion,&pftpInfo->sftpClientData);
	if ( err != 0 ) 
	{
		printf("\r\n Tell the user that we could not find a usable WinSock DLL");
		return 0;
	}

	if ( LOBYTE(pftpInfo->sftpClientData.wVersion ) != 2 ||HIBYTE(pftpInfo->sftpClientData.wVersion ) != 2 ) 
	{
		printf("\r\n Tell the user that we could not find a usable WinSock DLL. ");
		WSACleanup();
		return 0;
	}

	pftpInfo->sftpClient = socket(AF_INET,SOCK_STREAM,0);
	if(INVALID_SOCKET == pftpInfo->sftpClient)
	{
		printf("\r\n Tell the user that socket INVALID_SOCKET ");
		WSACleanup();
		return 0;
	}
	pftpInfo->saftpClient.sin_family = AF_INET;
	pftpInfo->saftpClient.sin_addr.S_un.S_addr = inet_addr("192.168.8.7");
	pftpInfo->saftpClient.sin_port =  htons(21);

	err = connect(pftpInfo->sftpClient,(SOCKADDR *)&pftpInfo->saftpClient,sizeof(SOCKADDR));
	if (SOCKET_ERROR == err)
	{
		printf("connect() failed!\n");
		closesocket(pftpInfo->sftpClient); //关闭套接字
		WSACleanup();
		return 0;
	}

	while(!pftpInfo->bKillFTPClientThread)
	{
		memset(pftpInfo->clientRecvBuf,'\0',sizeof(pftpInfo->clientRecvBuf));
		ret = recv(pftpInfo->sftpClient,pftpInfo->clientRecvBuf,sizeof(pftpInfo->clientRecvBuf) - 1,0);
		if (SOCKET_ERROR == ret)
		{
			printf("\r\n recv fail");
			pftpInfo->bKillFTPClientThread = TRUE;
		}
		else
		{
			fptClientProcess(pftpInfo->clientRecvBuf,ret);
		}
	}
	closesocket(pftpInfo->sftpClient);
	WSACleanup();
	CloseHandle(pftpInfo->hFTPClientThread);
	return 0;
}
/*************************************************************************************************************************
**函数名称：	_tmain
**函数功能:
**入口参数:
**返回参数:
*************************************************************************************************************************/
int _tmain(int argc, _TCHAR* argv[])
{
	memset(&ftpInfo,0,sizeof(ftpInfo));
	pftpInfo = &ftpInfo;
	pftpInfo->bKillFTPClientThread = FALSE;
	pftpInfo->hFTPClientThread = CreateThread(NULL,0,ThreadFTPClientFunc,NULL,0,NULL);
	if(NULL == pftpInfo->hFTPClientThread)
	{
		printf("\r\n CreateThread Fail");
	}
	
	while (1);
	return 0;
}

