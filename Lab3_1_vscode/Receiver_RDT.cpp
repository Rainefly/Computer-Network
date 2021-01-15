#include <fstream>
#include <iostream>
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <cstring>
#include <ctime>
#include <process.h>

using namespace std;

#pragma comment(lib, "ws2_32")

int identifier = 0;    // 期望接收到的分组
string input;         // 用户输入内容

// WSA&socket变量
WSADATA wsaData;
int sock_error;
SOCKADDR_IN RecvAddr;
SOCKADDR_IN SenderAddr;
SOCKET receiver;
char* bufSend, * bufRecv;
const int MAX_PACLEN = 65535;    // 因为报文给长度表示位流出来的是16位，所以2^16-1=65535

//文件读写变量
string File_path = ".\\workfile3_1\\";
string File_des_path = ".\\workfile_res3_1\\";
int Data_length;      // 数据总长
int Gram_length;      // 报文总长

//计时变量
clock_t lastTime = 0;     // 持续时间 （endTime - startTime）
clock_t timecount = 0;    // 时间的计数器
clock_t startTime = 0, endTime = 0;
HANDLE ticktime;   
int waiting_max = 50;        // 计时器限度

USHORT geneCheckSum(char* buf, int len)
{
    if (len % 2 == 1) len++;
    unsigned char* ubuf = (unsigned char*)buf;
    //register ULONG sum = 1;    //用于检验
    register ULONG sum= 0;
    while (len)
    {
        USHORT temp = USHORT(*ubuf << 8) + USHORT(*(ubuf + 1));
        sum += temp;
        if (sum & 0xFFFF0000)
        {
            sum &= 0xFFFF;
            sum++;
        }
        ubuf += 2;
        len -= 2;
    }
    return ~(sum & 0xFFFF);
}

char* geneDataGram(SOCKADDR_IN* src, SOCKADDR_IN* tar, char* data, string name)
{
    USHORT sport = src->sin_port;
    USHORT tport = tar->sin_port;
    int len = 12 + Data_length + name.length();
    Gram_length = ((len % 2 == 0) ? len : len + 1);
    bufSend = new char[Gram_length];
    bufSend[0] = sport >> 8;
    bufSend[1] = sport & 0xFF;
    bufSend[2] = tport >> 8;
    bufSend[3] = tport & 0xFF;
    bufSend[4] = len >> 8;
    bufSend[5] = len & 0xFF;
    bufSend[6] = 0;
    bufSend[7] = 0;
    bufSend[8] = identifier & 0xFF;
    bufSend[9] = 0;
    bufSend[10] = name.length() >> 8;
    bufSend[11] = name.length() & 0xFF;
    int phead = 12;
    for (int i = 0; i < name.length(); i++)
        bufSend[phead++] = name[i];
    for (int i = 0; i < Data_length; i++)
        bufSend[phead++] = data[i];
    if (len % 2 == 1) bufSend[len] = 0;
    USHORT ck = geneCheckSum(bufSend, len);
    bufSend[6] = ck >> 8;
    bufSend[7] = ck & 0xFF;
    return bufSend;
}

string getFileName()
{
    string name;
    int len = (int)(USHORT((unsigned char)(bufRecv[10]) << 8) + USHORT((unsigned char)(bufRecv[11])));
    for (int i = 0; i < len; i++)
        name += bufRecv[i + 12];
    return name;
}

void updateCheckSum()
{
    USHORT CheckSum = (USHORT(((unsigned char)bufSend[6]) << 8) + USHORT((unsigned char)bufSend[7]));
    register ULONG sum = (~CheckSum & 0xFFFF) + USHORT((unsigned char)bufSend[9]);
    if (sum & 0xFFFF0000)
        CheckSum = (sum & 0xFFFF) + 1;
    else
        CheckSum = sum & 0xFFFF;
    CheckSum = ~CheckSum;
    bufSend[6] = CheckSum >> 8;
    bufSend[7] = CheckSum & 0xFF;
}


bool CheckID() {return (identifier == int(USHORT((unsigned char)bufRecv[8]))); }
bool checkACK()  { return bufRecv[9] & 0x01;}
bool checkFIN()  {return bufRecv[9] & 0x04; }
bool checkSYN() { return bufRecv[9] & 0x02; }

void setNAK() { bufSend[9] &= 0xFE; updateCheckSum(); }
void setACK() { bufSend[9] |= 0x01; updateCheckSum(); }
void setNFIN() { bufSend[9] &= 0xFB; updateCheckSum(); }
void setFIN() { 
    bufSend[9] |= 0x04; 
    bufSend[8] = bufRecv[8] + 1;
    updateCheckSum(); 
}
void setSYN() { 
    bufSend[9] |= 0x02; 
    bufSend[8] = bufRecv[8] + 1;    //回复的是原始序列号+1
    updateCheckSum();
}

// 开线程： 用来计时（用于超时等待）
DWORD WINAPI TimeCheck(LPVOID lparam)
{
    int* waiting_max = (int*)lparam;
    while (true)
    {
        endTime = clock();
        lastTime = endTime - startTime;
        if (lastTime / CLOCKS_PER_SEC > timecount)    // CLOCKS_PER_SEC=1000
        {
            timecount = lastTime / CLOCKS_PER_SEC;
            cout <<  "*******    ticks : timed waited for information: ";
            cout << lastTime * 1.0 / CLOCKS_PER_SEC << "s     ********" << endl;
        }
        if (lastTime / CLOCKS_PER_SEC == *waiting_max)
        {
            delete[] bufSend;
            Data_length = 0;
            bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
            setFIN();    //告知对方关闭
            sock_error = sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, sizeof(SOCKADDR));
            cout <<  "The program is going to close ... " << endl;
            //timecount = 0;
            CloseHandle(ticktime);
            delete[] bufRecv;
            delete[] bufSend;
            exit(0);
        }
    }
    return 0;
}

int main()
{
    // 以下内容为socket的绑定
    const WORD wVersionRequested = MAKEWORD(2, 2);
    sock_error = WSAStartup(wVersionRequested, &wsaData);
    if (sock_error != 0)
    {
        cout <<  "sock_error code " << WSAGetLastError() << endl;
        cout <<  "init WSA failed" << endl;
        return -1;
    }

    cout <<  "WSA has started successfully" << endl;

    receiver = socket(AF_INET, SOCK_DGRAM, 0);
    if (receiver == INVALID_SOCKET)
    {
        cout <<  "bind socket error" << endl;
        sock_error = WSACleanup();
        return -1;
    }
 
    RecvAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(6665);
    sock_error = bind(receiver, (SOCKADDR*)&RecvAddr, sizeof(SOCKADDR));
    if (sock_error != 0)
    {
        cout <<  "sock_error code " << WSAGetLastError() << endl;
        cout <<  "bind socket error" << endl;
        sock_error = WSACleanup();
        return -1;
    }
    cout <<  "bind socket successfully" << endl;

    int socketlen = sizeof(SOCKADDR);
    SenderAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    SenderAddr.sin_family = AF_INET;
    SenderAddr.sin_port = htons(8887);

    cout << "The Receiver's max waiting time is :"<< waiting_max <<endl;
    // 与对方建立连接
    startTime = clock();
    ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);
    bool InitCon = false;
    while (!InitCon)
    {
        cout << " wait for others to connect  ..." << endl;
        bufRecv = new char[MAX_PACLEN];
        sock_error = recvfrom(receiver, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
        if (sock_error == SOCKET_ERROR || !checkSYN())
        {
            delete[] bufRecv;
            continue;
        }
        Data_length = 0;
        bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
        setACK();
        setSYN();
        sock_error = sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);
        if (sock_error != Gram_length)
        {
            delete[] bufRecv;
            continue;
        }
        InitCon = true;
        TerminateThread(ticktime, 0);
        CloseHandle(ticktime);
        delete[] bufRecv;
    }

    cout <<  "connect successfully" << endl;

    // 建立连接后，开始接收数据
    identifier = 0;
    startTime = clock();
    timecount = 0;
    ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);
    while (true)
    {
        bufRecv = new char[MAX_PACLEN];
        sock_error = recvfrom(receiver, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
        Gram_length = (int)((USHORT)((unsigned char)bufRecv[4] << 8) + (USHORT)(unsigned char)bufRecv[5]);
        Gram_length = ((Gram_length % 2 == 0) ? Gram_length : Gram_length + 1);
        if (sock_error != Gram_length)
        {
            delete[] bufRecv;
            startTime = clock();
            timecount = 0;
            TerminateThread(ticktime, 0);
            CloseHandle(ticktime);
            ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);
            continue;
        }

        USHORT CheckSum = geneCheckSum(bufRecv, Gram_length);    // compute checksum  ( the correct answer is 0 )
        Data_length = 0;
        if (!CheckSum && CheckID())
        {
            if (checkFIN())
            {
                //发一个回复FIN的报文
                bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
                setFIN();
                sock_error = sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);
                cout <<  "The program is going to close ... " << endl;
                delete[] bufRecv;
                delete[] bufSend;
                TerminateThread(ticktime, 0);
                CloseHandle(ticktime);
                InitCon = false;
                timecount = 0;
                system("pause");
                return 0;
            }

            bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
            setACK();
            identifier = (identifier + 1) % 2;   //状态迁移
        }
        else
        {
            if (CheckSum) cout << "wrong CheckSum: " << CheckSum <<" Means that sender's packet has something wrong. "<< endl;
            if (!CheckID()) cout << "sender need identifier " << identifier << endl;
            bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
            setNAK();
            sock_error = sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);
            delete[] bufRecv;
            delete[] bufSend;
            startTime = clock();
            timecount = 0;
            TerminateThread(ticktime, 0);
            CloseHandle(ticktime);
            ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);
            continue;
        }
        sendto(receiver, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);

        // 收到数据后，把数据写入文件
        Gram_length = (int)((USHORT)((unsigned char)bufRecv[4] << 8) + (USHORT)(unsigned char)bufRecv[5]);
        string name = getFileName();
        ofstream os((File_des_path + name).c_str(), ios::app | ios::binary);
        os.seekp(0, os.end);
        for (int i = 0; i < Gram_length - name.length() - 12; i++)
            os << (unsigned char)bufRecv[i + 12 + name.length()];
        os.close();
        if( Gram_length < 65535-256)
            cout <<  "all data of " << name << " has been written into the file" << endl;

        delete[] bufSend;
        delete[] bufRecv;
        startTime = clock();
        timecount = 0;
        TerminateThread(ticktime, 0);
        ticktime = CreateThread(NULL, NULL, TimeCheck, LPVOID(&waiting_max), 0, 0);
    }

    // close
    CloseHandle(ticktime);
    sock_error = closesocket(receiver);
    sock_error = WSACleanup();
    system("pause");
    return 0;
}