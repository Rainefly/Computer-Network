//#include <WinSock2.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include<iostream>
//
//#pragma comment(lib,"ws2_32.lib")
//
//using namespace std;
//
//int main()
//{
//	//定义获得可用socket的详细信息的变量
//	WSADATA wsaData;
//
//	//判断自己想要的版本和系统能用的版本是否一致
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
//	{
//		cout << "WSAStartup failed!" << endl;
//		return -1;
//	}
//
//	//先创建所需的socket
//	SOCKET ServerSocket; //server的socket
//	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  //ipv4的地址类型；流结构的服务类型；TCP的协议；
//
//	if (ServerSocket == INVALID_SOCKET)
//	{
//
//		cout << "socket failed with error code:" << WSAGetLastError() << endl;
//		return -1;
//	}
//
//	//而后要把本地地址bind到socket上
//	SOCKADDR_IN ServerAddr;
//	USHORT uPort = 666;
//	ServerAddr.sin_family = AF_INET;
//	ServerAddr.sin_port = htons(uPort);
//	ServerAddr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
//	int iServerAddLen = sizeof(ServerAddr);
//
//	//绑定并检验
//	if (SOCKET_ERROR == bind(ServerSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
//	{
//		printf("bind failed with error code: %d\n", WSAGetLastError());
//		closesocket(ServerSocket);
//		return -1;
//	}
//
//	//界面初始化，提示服务器输入名字
//	cout << "please input your name as a server: " << endl;
//	char serverName[32] = { 0 };
//	cin >> serverName;
//
//	cout << "start listening...please wait for other users to connect..." << endl;
//
//	//绑定完成后开始listen，这里设置的等待队列长度是1
//	if (SOCKET_ERROR == listen(ServerSocket, 1))
//	{
//		cout << "listen failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//以下为后续的accept开辟新的线程里的socket做准备
//	SOCKET Conn_new_Socket;
//	SOCKADDR_IN Conn_new_Addr;
//	int iClientAddrLen = sizeof(Conn_new_Addr);
//
//	//Conn_new_Socket的信息不用bind，因为它会随着serveraccept的地址传过来
//	Conn_new_Socket = accept(ServerSocket, (SOCKADDR*)&Conn_new_Addr, &iClientAddrLen);
//
//	if (Conn_new_Socket == INVALID_SOCKET)
//	{
//		cout << "accept failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//界面提示，显示连接者的信息
//	cout << "Now a user is successfully connectting to you! Have a chat now! You can input 'seeu' to end the commucation!" << endl;
//	cout << "The connector's informaintion is shown as below:" << endl;
//	cout << "connector's ip is: " << inet_ntoa(Conn_new_Addr.sin_addr) << "    " << "connector's port is: " << htons(Conn_new_Addr.sin_port) << endl << endl;
//
//	//对收发的信息进行存储和显示
//	char clientName[32] = { 0 };
//	char buffer[4096] = { 0 };
//	int RecvLen = 0;  //实际收到的字节数
//	int SendLen = 0;  //实际发送的字节数
//
//	//把serverName发给对方
//	SendLen = send(Conn_new_Socket, serverName, strlen(serverName), 0);
//	if (SOCKET_ERROR == SendLen)
//	{
//		cout << "send failed with error code:" << WSAGetLastError() << endl;
//		closesocket(Conn_new_Socket);
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//接收对方发过来的信息
//	RecvLen = recv(Conn_new_Socket, clientName, sizeof(clientName), 0);
//	if (SOCKET_ERROR == RecvLen)
//	{
//		cout << "recv failed with error code:" << WSAGetLastError() << endl;
//		closesocket(Conn_new_Socket);
//		closesocket(ServerSocket);
//		WSACleanup();
//		return -1;
//	}
//	strcat_s(clientName, "\0");
//
//	//下面的逻辑就是先显示从client方收到的消息，判断是否是结束字符；
//	//而后输入server自己想发的消息，传到client去;
//	//任意一方如果发出结束符，那么等对方回复一条消息后，两边都会结束对话。
//	while (true)
//	{
//		cout << clientName << ":";
//		memset(buffer, 0, sizeof(buffer));
//		RecvLen = recv(Conn_new_Socket, buffer, sizeof(buffer), 0);
//		if (SOCKET_ERROR == RecvLen)
//		{
//			cout << "recv failed with error code:" << WSAGetLastError() << endl;
//			closesocket(Conn_new_Socket);
//			closesocket(ServerSocket);
//			WSACleanup();
//			return -1;
//		}
//		strcat_s(buffer, "\0");
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			cout << buffer << endl;
//			memset(buffer, 0, sizeof(buffer));
//			cout << serverName << ":";
//			cin >> buffer;
//			SendLen = send(Conn_new_Socket, buffer, strlen(buffer), 0);
//			break;
//		}
//		cout << buffer << endl;
//
//
//		memset(buffer, 0, sizeof(buffer));
//		cout << serverName << ":";
//		cin >> buffer;
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			SendLen = send(Conn_new_Socket, buffer, strlen(buffer), 0);
//			if (SOCKET_ERROR == SendLen)
//			{
//				cout << "send failed with error code:" << WSAGetLastError() << endl;
//				closesocket(Conn_new_Socket);
//				closesocket(ServerSocket);
//				WSACleanup();
//				return -1;
//			}
//			cout << clientName << ":";
//			memset(buffer, 0, sizeof(buffer));
//			RecvLen = recv(Conn_new_Socket, buffer, sizeof(buffer), 0);
//			cout << buffer << endl;
//			break;
//		}
//		SendLen = send(Conn_new_Socket, buffer, strlen(buffer), 0);
//		if (SOCKET_ERROR == SendLen)
//		{
//			cout << "send failed with error code:" << WSAGetLastError() << endl;
//			closesocket(Conn_new_Socket);
//			closesocket(ServerSocket);
//			WSACleanup();
//			return -1;
//		}
//	}
//
//	closesocket(Conn_new_Socket);
//	closesocket(ServerSocket);
//	WSACleanup();
//	system("pause");
//	return 0;
//}