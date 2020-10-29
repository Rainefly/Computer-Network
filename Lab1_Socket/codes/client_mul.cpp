#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include<iostream>
using namespace std;
#pragma comment(lib,"ws2_32.lib")

#define RECV_OVER 1    //已收到
#define RECV_YET 0     //还没收到
char userName[16] = { 0 };
boolean isPrint = false;  // 判断是否要在客户端打印名字
int Status = RECV_YET;

//接收数据线程
unsigned __stdcall ThreadRecv(void* param)
{
	char bufferRecv[128] = { 0 };
	while (true)
	{
		int error = recv(*(SOCKET*)param, bufferRecv, sizeof(bufferRecv), 0);
		if (error == SOCKET_ERROR)
		{
			Sleep(500);
			continue;
		}
		if (strlen(bufferRecv) != 0)
		{
			if (isPrint)
			{
				for (int i = 0; i <= strlen(userName) + 2; i++)
					cout << "\b";
				cout << bufferRecv << endl;
				//printf("\r");    //覆盖之前的名字
                //printf("%s\n", bufferRecv);
				cout << userName << ": ";   //因为这是在用户的send态时，把本来打印出来的userName给退回去了，所以收到以后需要再把userName打印出来
				Status = RECV_OVER;
			}
			else
				cout << bufferRecv << endl;
		}
		else
			Sleep(100);
	}
	return 0;
}


int main()
{
	WSADATA wsaData = { 0 };//存放套接字信息
	SOCKET ClientSocket = INVALID_SOCKET;//客户端套接字
	SOCKADDR_IN ServerAddr = { 0 };//服务端地址
	USHORT uPort = 1666;//服务端端口
						 //初始化套接字
	if (WSAStartup(MAKEWORD(2, 2), &wsaData))//将库文件和程序绑定
	{
		cout << "WSAStartup error: " << WSAGetLastError() << endl;
		return -1;
	}

	//创建套接字
	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ClientSocket == INVALID_SOCKET)
	{
		cout << "Socket error: " << WSAGetLastError() << endl;
		return -1;
	}

	//输入服务器IP
	/*cout << "Please input the server IP:";
	char IP[32] = { 0 };
	cin.getline(IP,32);*/


	//设置服务器地址
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(uPort);//服务器端口
	ServerAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");//服务器地址

	cout << "connecting...please wait..." << endl;

	//连接服务器
	if (SOCKET_ERROR == connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
	{
		cout << "Connect error: "<<WSAGetLastError() << endl;
		closesocket(ClientSocket);
		WSACleanup();
		return -1;
	}

	cout << "Connect successfully!" << "The server's ip is:" << inet_ntoa(ServerAddr.sin_addr);
	cout << "The server's port is:" << htons(ServerAddr.sin_port) << endl << endl;
	cout << "Please input your name: ";

	cin.getline(userName,16);
	cout << "You can input 'exit' to close the client. " << endl;
	send(ClientSocket, userName, sizeof(userName), 0);
	cout << endl;

	char identify[16] = { 0 };
	cout << "You can choose the other client to chat. If you input'all',then everyone can receive your message; if you input the client's name, then the only client can receive your message. " << endl;
	cout << "choose the client: ";
	cin.getline(identify, 16);
	send(ClientSocket, identify, sizeof(identify), 0);

	_beginthreadex(NULL, 0, ThreadRecv, &ClientSocket, 0, NULL); //启动接收线程

	char bufferSend[128] = { 0 };

	while (1)
	{
		if (isPrint != true)
		{
			cout << userName << ": ";
			isPrint = true;
		}
		cin.getline(bufferSend,128);
		if (strcmp(bufferSend, "exit") == 0)
		{
			cout << "you are going to exit. " << endl;
			Sleep(2000);
			int error = send(ClientSocket, bufferSend, sizeof(bufferSend), 0);
			if (error == SOCKET_ERROR)
				return -1;    //退出当前线程
			return 0;   //线程会关闭
		}

		int error = send(ClientSocket, bufferSend, sizeof(bufferSend), 0);
		if (error == SOCKET_ERROR)
			return -1;    //退出当前线程
		if (error != 0)
		{
			isPrint = false;
		}
	}
	for (int k = 0; k < 1000; k++)    //让主线程一直转
		Sleep(10000000);

	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}