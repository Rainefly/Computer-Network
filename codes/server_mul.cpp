#include <WinSock2.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include<iostream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

#define SEND_OVER 1							 //已经转发消息
#define SEND_YET  0							 //还没转发消息
int i = 0;
int Status = SEND_YET;

SOCKET ServerSocket = INVALID_SOCKET;		 //服务端套接字
SOCKADDR_IN ClientAddr = { 0 };			 //客户端地址
int ClientAddrLen = sizeof(ClientAddr);

bool CheckConnect = false;                //检查连接情况
HANDLE HandleRecv[10] = { NULL };				 //线程句柄
HANDLE mainHandle;							 //用于accept的线程句柄

//客户端信息结构体
typedef struct _Client
{
	SOCKET sClient;      //客户端套接字
	char buffer[128];		 //数据缓冲区
	char userName[16];   //客户端用户名
	char identify[16];   //用于标识转发的范围
	char IP[20];		 //客户端IP
	UINT_PTR flag;       //标记客户端，用来区分不同的客户端
} Client;

Client inClient[10] = { 0 };                  //创建一个客户端结构体,最多同时10人在线

//发送数据线程
unsigned __stdcall ThreadSend(void* param)
{
	int error = 0;
	int flag = *(int*)param;
	SOCKET client = INVALID_SOCKET;					//创建一个临时套接字来存放要转发的客户端套接字
	char temp[128] = { 0 };							//创建一个临时的数据缓冲区，用来存放接收到的数据
	memcpy(temp, inClient[flag].buffer, sizeof(temp));
	sprintf_s(inClient[flag].buffer, "%s: %s", inClient[flag].userName, temp); //把发送源的名字添进转发的信息里
	if (strlen(temp) != 0 && Status == SEND_YET) //如果数据不为空且还没转发则转发
	{
		if (strcmp(inClient[flag].identify, "all") == 0)
		{
			for (int j = 0; j < i; j++) {
				if (j != flag) {//向除自己之外的所有客户端发送信息
					error = send(inClient[j].sClient, inClient[flag].buffer, sizeof(inClient[j].buffer), 0);
					if (error == SOCKET_ERROR)
						return 1;
				}
			}
		}
		else
		{
			for(int j=0;j<i;j++)
				if (strcmp(inClient[j].userName, inClient[flag].identify) == 0)
				{
					error = send(inClient[j].sClient, inClient[flag].buffer, sizeof(inClient[j].buffer), 0);
					if (error == SOCKET_ERROR)
						return 1;
				}
		}
	}
		
	Status = SEND_OVER;   //转发成功后设置状态为已转发
	return 0;
}

//接受数据线程
unsigned __stdcall ThreadRecv(void* param)
{
	SOCKET client = INVALID_SOCKET;
	int flag = 0;
	for (int j = 0; j < i; j++) {
		if (*(int*)param == inClient[j].flag)            //判断是为哪个客户端开辟的接收数据线程
		{
			client = inClient[j].sClient;
			flag = j;
		}
	}
	char temp[128] = { 0 };  //临时数据缓冲区
	while (true)
	{
		memset(temp, 0, sizeof(temp));
		int error = recv(client, temp, sizeof(temp), 0); //接收数据
		if (error == SOCKET_ERROR)
			continue;
		Status = SEND_YET;	//设置转发状态为未转发
		memcpy(inClient[flag].buffer, temp, sizeof(inClient[flag].buffer));
		if (strcmp(temp, "exit") == 0)   //判断如果客户发送exit请求，那么直接关闭线程，不打开转发线程
		{
			closesocket(inClient[flag].sClient);//关闭该套接字
			CloseHandle(HandleRecv[flag]); //这里关闭了线程句柄
			inClient[flag].sClient = 0;  //把这个位置空出来，留给以后进入的线程使用
			HandleRecv[flag] = NULL;
			//用printf来取参数
			cout << "'"<<inClient[flag].userName <<"'" << "has disconnected from the server. " << endl;
		}
		else
		{
			cout << inClient[flag].userName <<": "<< temp << endl;
			_beginthreadex(NULL, 0, ThreadSend, &flag, 0, NULL); //开启一个转发线程,flag标记着这个需要被转发的信息是从哪个线程来的；
		}
	}
	return 0;
}


//接受请求
unsigned __stdcall MainAccept(void* param)
{
	int flag[10] = { 0 };
	while (true)
	{
		if (inClient[i].flag != 0)   //找到从前往后第一次没被连接的inClient
		{
			i++;
			continue;
		}
		//如果有客户端申请连接就接受连接
		if ((inClient[i].sClient = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &ClientAddrLen)) == INVALID_SOCKET)
		{
			cout << "Accept error: " << WSAGetLastError() << endl;
			closesocket(ServerSocket);
			WSACleanup();
			return -1;
		}
		recv(inClient[i].sClient, inClient[i].userName, sizeof(inClient[i].userName), 0); //接收用户名
		recv(inClient[i].sClient, inClient[i].identify, sizeof(inClient[i].identify), 0); //接收转发的范围

		cout << "The user: '" << inClient[i].userName <<"'"<< " connect successfully!" << endl;
		memcpy(inClient[i].IP, inet_ntoa(ClientAddr.sin_addr), sizeof(inClient[i].IP)); //记录客户端IP
		inClient[i].flag = inClient[i].sClient; //不同的socke有不同UINT_PTR类型的数字来标识
		i++;    //如果去掉这个最开始的赋值会报错
		for (int j = 0; j < i; j++) 
		{
			if (inClient[j].flag != flag[j]) 
			{
				if (HandleRecv[j])   //如果那个句柄已经被清零了，那么那就关掉那个句柄
					CloseHandle(HandleRecv[j]);
				HandleRecv[j] = (HANDLE)_beginthreadex(NULL, 0, ThreadRecv, &inClient[j].flag, 0, NULL); //开启接收消息的线程
			}
		}

		for (int j = 0; j < i; j++) 
		{
			flag[j] = inClient[j].flag;//防止ThreadRecv线程多次开启
		}
		Sleep(3000);
	}
	return 0;
}

int main()
{
	WSADATA wsaData = { 0 };

	//初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "WSAStartup error: " << WSAGetLastError() << endl;
		return -1;
	}

	//创建套接字
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET)
	{
		cout << "Socket error: " << WSAGetLastError() << endl;
		return -1;
	}

	SOCKADDR_IN ServerAddr = { 0 };				//服务端地址
	USHORT uPort = 1666;						//服务器监听端口
	//设置服务器地址
	ServerAddr.sin_family = AF_INET;//连接方式
	ServerAddr.sin_port = htons(uPort);//服务器监听端口
	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//任何客户端都能连接这个服务器

	//绑定服务器
	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		cout << "Bind error: " << WSAGetLastError() << endl;
		closesocket(ServerSocket);
		return -1;
	}

	//设置监听客户端连接数
	if (SOCKET_ERROR == listen(ServerSocket, 20))
	{
		cout << "Listen error: " << WSAGetLastError() << endl;
		closesocket(ServerSocket);
		WSACleanup();
		return -1;
	}

	cout << "please wait for other clients to wait..." << endl;
	mainHandle = (HANDLE)_beginthreadex(NULL, 0, MainAccept, NULL, 0, 0);   //mainHandle是连接其他client的主要线程
	char Serversignal[10];
	cout << "if you want to close the server, please input 'exit' " << endl;
	while (true)
	{
		cin.getline(Serversignal,10);
		if (strcmp(Serversignal, "exit") == 0) 
		{
			cout << "The server is closed. " << endl;
			CloseHandle(mainHandle);
			for (int j = 0; j < i; j++) //依次关闭套接字
			{
				if (inClient[j].sClient != INVALID_SOCKET)
					closesocket(inClient[j].sClient);
			}
			closesocket(ServerSocket);
			WSACleanup();
			exit(1);
			return 1;
		}
	}
	return 0;
}