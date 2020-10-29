////这是实现的是简单的一台服务器和一个客户端的交流；
//
//#include <WinSock2.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include<iostream>
//using namespace std;
//#pragma comment(lib,"ws2_32.lib")
//
//
//int main()
//{
//	WSADATA wsaData;
//	//判断自己想要的版本和系统能用的版本是否一致
//	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
//	{
//		cout << "WSAStartup failed!" << endl;
//		return 1;
//	}
//
//	//创建并检验client所需的socket
//	SOCKET ClientSocket;
//	ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
//	if (ClientSocket == INVALID_SOCKET)
//	{
//		cout << "socket failed with error code:" << WSAGetLastError() << endl;
//		return -1;
//	}
//
//	char clientName[32] = { 0 };
//	cout << "please input your name as a client: " << endl;
//	cin >> clientName;
//
//	//下面为想要连接的远端进行设置。
//	char IP[32] = { 0 };
//	cout << "please input the IP of the server you want to connect: " << endl;
//	cin >> IP;
//
//	SOCKADDR_IN ServerAddr;
//	USHORT uPort = 666;
//	ServerAddr.sin_family = AF_INET;
//	ServerAddr.sin_port = htons(uPort);
//	ServerAddr.sin_addr.S_un.S_addr = inet_addr(IP);
//
//
//	cout << "start connecting...please wait for the server to connect..." << endl;
//	//把自己的socket与远端连接：
//	if (SOCKET_ERROR == connect(ClientSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)))
//	{
//		cout << "connect failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ClientSocket);
//		WSACleanup();
//		return -1;
//	}
//	
//	cout << "Now you are successfully connected to a server!Have a chat now!You can input 'seeu' to end the commucation!" << endl;
//	cout << "The server's information is shown as below:" << endl;
//	cout << "The server's Ip is: "<<inet_ntoa(ServerAddr.sin_addr)<<"      "<<"The server's port is: "<< htons(ServerAddr.sin_port)<<endl<<endl;
//
//	//下面是对收发消息的存储和显示
//	char buffer[4096] = { 0 };
//	char serverName[32] = { 0 };
//	int RecvLen = 0;    //实际收到的字节数
//	int SendLen = 0;    //实际发送的字节数
//
//	//把自己的名字发过去
//	SendLen = send(ClientSocket, clientName, strlen(clientName), 0);
//	if (SOCKET_ERROR == SendLen)
//	{
//		cout << "send failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ClientSocket);
//		WSACleanup();
//		return -1;
//	}
//
//	//接收对方的名字，存进serverName
//	//双方无论谁先发送结束字符，在等到对方回复一条消息后，双方都会结束。
//	RecvLen = recv(ClientSocket, serverName, sizeof(serverName), 0);
//	if (SOCKET_ERROR == RecvLen)
//	{
//		cout << "recv failed with error code:" << WSAGetLastError() << endl;
//		closesocket(ClientSocket);
//		WSACleanup();
//		return -1;
//	}
//	strcat_s(serverName, "\0");
//
//	while (true)
//	{
//		memset(buffer, 0, sizeof(buffer));
//		cout << clientName << ":";
//		cin >> buffer;
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			SendLen = send(ClientSocket, buffer, strlen(buffer), 0);
//			if (SOCKET_ERROR == SendLen)
//			{
//				cout << "send failed with error code:" << WSAGetLastError() << endl;
//				closesocket(ClientSocket);
//				WSACleanup();
//				return -1;
//			}
//			cout << serverName << ":";
//			memset(buffer, 0, sizeof(buffer));
//			RecvLen = recv(ClientSocket, buffer, sizeof(buffer), 0);
//			cout << buffer << endl;
//			break;
//		}
//		SendLen = send(ClientSocket, buffer, strlen(buffer), 0);
//		if (SOCKET_ERROR == SendLen)
//		{
//			cout << "send failed with error code:" << WSAGetLastError() << endl;
//			closesocket(ClientSocket);
//			WSACleanup();
//			return -1;
//		}
//
//		cout << serverName<<":";
//		memset(buffer, 0, sizeof(buffer));
//		RecvLen = recv(ClientSocket, buffer, sizeof(buffer), 0);
//		if (SOCKET_ERROR == RecvLen)
//		{
//			cout << "recv failed with error code:" << WSAGetLastError() << endl;
//			closesocket(ClientSocket);
//			WSACleanup();
//			return -1;
//		}
//		strcat_s(buffer, "\0");
//		if (strcmp(buffer, "seeu") == 0)
//		{
//			cout << buffer << endl;
//			memset(buffer, 0, sizeof(buffer));
//			cout << clientName << ":";
//			cin >> buffer;
//			SendLen = send(ClientSocket, buffer, strlen(buffer), 0);
//			break;
//		}
//		cout << buffer << endl;
//	}
//
//	closesocket(ClientSocket);
//	WSACleanup();
//	system("pause");
//	return 0;
//}