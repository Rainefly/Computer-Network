#include <iostream>
#include <fstream>
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <cstring>
#include <ctime>
#include <io.h>
#include <iomanip>
#include <process.h>
#include <sys/types.h>
#include <vector>

using namespace std;

#pragma comment(lib,"ws2_32.lib")

//WSA&socket����
WSADATA wsaData;
int sock_error;

// ���ݰ�����
const int MAX_PACLEN = 65535 - 256;
char* bufSend, * bufRecv;
int identifier = 0;       
int Max_Resend = 10;    //�ش�������Ϊ10
int Cur_Resend = 0;       
int Data_length;         
int Gram_length;  

//��ʱ����
clock_t startTime, endTime;
clock_t FileStartTime , FileEndTime;
struct _finddata_t info;
struct timeval timeout;

//��д�ļ�����
string File_Path = ".\\workfile3_1\\";
string File_Des_Path = ".\\workfile_res3_1\\";
vector<string> files;
long handle;

string input;

// �������ϵ�д��д��
USHORT geneChecksum(char* buf, int len)
{
    if (len % 2 == 1) len++;
    unsigned char* ubuf = (unsigned char*)buf;
    register ULONG sum = 0;
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

// ��������UDP�ı����ԼӸı�
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
    int packet_loc = 12;
    for (int i = 0; i < name.length(); i++)
        bufSend[packet_loc++] = name[i];
    for (int i = 0; i < Data_length; i++)
        bufSend[packet_loc++] = data[i];
    if (len % 2 == 1) bufSend[len] = 0;
    USHORT ck = geneChecksum(bufSend, len);
    bufSend[6] = ck >> 8;
    bufSend[7] = ck & 0xFF;
    return bufSend;
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

bool CheckID() { return (identifier == int(USHORT((unsigned char)bufRecv[8])));}
bool checkACK() { return bufRecv[9] & 0x01; }
bool checkRecvFIN()  {return ((identifier + 1) == int(USHORT((unsigned char)bufRecv[8]))); }
bool checkFIN()  { return bufRecv[9] & 0x04; }
bool checkSYN()  { return ((identifier + 1) == int(USHORT((unsigned char)bufRecv[8])))&&(bufRecv[9] & 0x02);}   //����SYN��Ӧ��Ҫ�������

void setFIN() { bufSend[9] |= 0x04; updateCheckSum(); }
void setNSYN() { bufSend[9] &= 0xFD; updateCheckSum(); }
void setSYN() { bufSend[9] |= 0x02; updateCheckSum(); }


int main()
{
    // ������ɶ��׽��ֵİ�
    const WORD wVersionRequested = MAKEWORD(2, 2);
    sock_error = WSAStartup(wVersionRequested, &wsaData);
    if (sock_error != 0)
    {
        cout <<  "sock_error code " << WSAGetLastError() << endl;
        cout <<  "init WSA failed" << endl;
        return -1;
    }
    cout<<"WSA has started successfully"<<endl;

    SOCKET Sender = socket(AF_INET, SOCK_DGRAM, 0);
    if (Sender == INVALID_SOCKET)
    {
        cout <<  "bind sock_error" << endl;
        sock_error = WSACleanup();
        return -1;
    }

    // ���׽����շ�ת����ʱ���趨 -> ʵ����ɵĹ����Ƿ��ͷ������ʱ��Ϊ�յ��ظ������ش���
    timeout.tv_sec = 1000 * 20;     //�趨�ȴ��ظ�ʱ��Ϊ20��
    timeout.tv_usec = 0;
    setsockopt(Sender, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    SOCKADDR_IN RecvAddr;
    RecvAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(8887);
    sock_error = bind(Sender, (SOCKADDR*)&RecvAddr, sizeof(SOCKADDR));
    if (sock_error != 0)
    {
        cout <<  "sock_error code " << WSAGetLastError() << endl;
        cout <<  "bind sock_error" << endl;
        sock_error = WSACleanup();
        return -1;
    }
    cout <<  "bind successfully" << endl;

    cout <<  "Please input Receiver's ip address: ";
    cin >> input;

    string ip = input;
    SOCKADDR_IN SenderAddr;
    int socketlen = sizeof(SOCKADDR);
    SenderAddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
    SenderAddr.sin_family = AF_INET;
    cout <<  "Please input Receiver's port: ";
    cin >> input;
    SenderAddr.sin_port = htons(atoi(input.c_str()));  //����Ҫ����6667
    cout <<  " The sender is sending SYN ..." << endl;
    Data_length = 0;
    bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
    setSYN();
    bool InitCon = false;
    while (!InitCon)
    {
        sock_error = sendto(Sender, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, sizeof(SOCKADDR));
        //cout << sock_error<<"  "<<Gram_length<<endl;
        if (sock_error != Gram_length)
        {
            cout <<  "wait..." << endl;
            Cur_Resend++;
            if (Cur_Resend == Max_Resend)
            {
                cout <<  "failed to connect" << endl;
                cout <<  "The program would shut down, wait..." << endl;
                delete[] bufRecv;
                delete[] bufSend;
                Sleep(1000);
                return -1;
            }
            continue;
        }
        bufRecv = new char[MAX_PACLEN];
        sock_error = recvfrom(Sender, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
        if (sock_error == SOCKET_ERROR || !checkACK() || !checkSYN())
        {
            delete[] bufRecv;
            cout <<  "wait..." << endl;
            Cur_Resend++;
            if (Cur_Resend == Max_Resend)
            {
                cout <<  "failed to connect" << endl;
                delete[] bufRecv;
                delete[] bufSend;
                cout <<  "The program would shut down, wait..." << endl;
                Sleep(1000);
                return -1;
            }
            continue;
        }
        InitCon = true;
        delete[] bufRecv;
    }

    cout <<  "connect successfully" << endl;
    cout << "Notice: The max resend chance is 10 "<<endl;
    cout << "Notice: The max waiting time for sender is 20 seconds" << endl;


    // �����ļ�������ѹջ���������
    handle = _findfirst((File_Path + "*").c_str(), &info);
    if (handle == -1)
    {
        cout <<  "failed to open directory..." << endl;
        return -1;
    }
    do {
        files.push_back(info.name);
    } while (!_findnext(handle, &info));

    cout <<  " The files have read in successfully ." << endl;

    Cur_Resend = 0;

    while (true)
    {
        cout << "There are the files existing in the path. " << endl;
        for (int i = 2; i < files.size(); i++)
            cout << "(" << i - 2 << ") " << files[i] << endl;
        cout <<  "You can input the num '-1' to quit" << endl;
        cout <<  "Please input the number of the file which you want to choose to send: ";
        cin >> input;
        FileStartTime=clock();
        if (input == "-1")
        {
            Data_length = 0;
            bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
            setFIN();
            sock_error = sendto(Sender, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);
            bufRecv = new char[MAX_PACLEN];
            sock_error = recvfrom(Sender, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
            if (checkRecvFIN())       //���ͷ�Ҫ������շ��Ƿ��յ����ظ���ȷ��Ϣ
            {
                cout <<  "The program is going to shut down..." << endl;
                delete[] bufRecv;
                delete[] bufSend;
                system("pause");
                exit(0);
            }
        }
        // ���ļ���������ܳ��ȣ�����֮���ѭ���ж�
        int i = atoi(input.c_str()) + 2;
        cout <<  "send file " << files[i] << endl;
        ifstream is((File_Path + files[i]).c_str(), ifstream::in | ios::binary);
        is.seekg(0, is.end);
        int File_length = is.tellg();
        //cout <<  "The file's space is : " << File_length << endl;
        is.seekg(0, is.beg);
        is.close();
        Data_length = File_length;

        //��ʼѭ����������
        int packet_loc = 0, packet_length;
        startTime = clock();
        while (packet_loc < File_length)
        {
            packet_length = ((File_length - packet_loc < MAX_PACLEN) ? File_length - packet_loc : MAX_PACLEN);
            ifstream is((File_Path + files[i]).c_str(), ifstream::in | ios::binary);
            is.seekg(packet_loc);
            bufSend = new char[packet_length];
            is.read(bufSend, packet_length);
            is.close();
            packet_loc += packet_length;
            Data_length = packet_length;
            bool isSend = false;
            while (!isSend)
            {
                bufSend = geneDataGram(&RecvAddr, &SenderAddr, bufSend, files[i]);
                sock_error = sendto(Sender, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, sizeof(SOCKADDR));
                cout << "Now the progress is : " << packet_loc << "/" << File_length << endl;
                if (sock_error != Gram_length)
                {
                    cout << " send " << files[i] << " failed, the network would resend it." << endl;
                    Cur_Resend++;
                    if (Cur_Resend == Max_Resend)         // �ﵽ����ش���������FIN����
                    {
                        delete[] bufSend;
                        Data_length = 0;
                        bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
                        setFIN();
                        sock_error = sendto(Sender, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);
                        bufRecv = new char[MAX_PACLEN];
                        sock_error = recvfrom(Sender, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
                        if (checkFIN())
                        {
                            cout <<  "failed to send" << endl;
                            delete[] bufRecv;
                            delete[] bufSend;
                            cout <<  "The program would shut down, wait..." << endl;
                            return -1;
                        }
                    }
                    continue;
                }
                else
                    cout << "*********************     sent    *********************" << endl;


                bufRecv = new char[MAX_PACLEN];
                sock_error = recvfrom(Sender, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
                Gram_length = (unsigned int)((USHORT)((unsigned char)bufRecv[4] << 8) + (USHORT)(unsigned char)bufRecv[5]);
                Gram_length = ((Gram_length % 2 == 0) ? Gram_length : Gram_length + 1);
                //cout << sock_error << "/" << Gram_length << endl;
                if (sock_error != Gram_length)
                {
                    cout << "The file receive failed, the program would resend it " << endl;
                    delete[] bufRecv;
                    Cur_Resend++;
                    if (Cur_Resend == Max_Resend)
                    {
                        delete[] bufSend;
                        Data_length = 0;
                        bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
                        setFIN();
                        sock_error = sendto(Sender, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);
                        bufRecv = new char[MAX_PACLEN];
                        sock_error = recvfrom(Sender, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
                        if (checkFIN())
                        {
                            cout <<  "failed to send" << endl;
                            delete[] bufRecv;
                            delete[] bufSend;
                            cout <<  "The program would shut down, wait..." << endl;
                            return -1;
                        }
                    }
                    continue;
                }

                USHORT CheckSum = geneChecksum(bufRecv, sock_error);
                if (!CheckSum && CheckID()&&checkACK()&&!checkFIN())       //�ж����������ж�
                {
                    identifier = (identifier + 1) % 2;             //����ת��
                    isSend = true;
                    cout<<" send successfully! "<<endl;
                }
                else
                {
                    if (CheckSum)
                        cout << "Receiver's response packet has something wrong. " << endl;
                    if (!CheckID())
                        cout << "sender need identifier: " << identifier << endl;
                    if (!checkACK())
                        cout << "sender's packet has something wrong. " << endl;
                    if (checkFIN())           //�յ��Է��رձ��ģ�ֱ���˳�
                    {
                        cout <<" receiver time out , stop sending ...";
                        delete[] bufRecv;
                        delete[] bufSend;
                        cout <<  " The program would shut down, wait... " << endl;
                        return -1;
                    }
                    cout << "ACK failed, the program would send it " << endl;
                    Cur_Resend++;
                    delete[] bufRecv;
                    if (Cur_Resend == Max_Resend)
                    {
                        delete[] bufSend;
                        Data_length = 0;
                        bufSend = geneDataGram(&RecvAddr, &SenderAddr, (char*)"", "");
                        setFIN();
                        sock_error = sendto(Sender, bufSend, Gram_length, 0, (SOCKADDR*)&SenderAddr, socketlen);
                        bufRecv = new char[MAX_PACLEN];
                        sock_error = recvfrom(Sender, bufRecv, MAX_PACLEN, 0, (SOCKADDR*)&SenderAddr, &socketlen);
                        if (checkFIN())
                        {
                            cout <<  "failed to send" << endl;
                            delete[] bufRecv;
                            delete[] bufSend;
                            cout <<  "The program would shut down, wait..." << endl;
                            return -1;
                        }
                    }
                    delete[] bufRecv;
                    continue;
                }
                delete[] bufRecv;
                delete[] bufSend;
            }
        }
        endTime = clock();
        FileEndTime=clock();
        cout <<  "Total time: " << (FileEndTime - FileStartTime) * 1.0 / CLOCKS_PER_SEC << endl;
        cout <<  "Throughout Rate: " << File_length * 1.0 / (FileEndTime - FileStartTime) << endl;
    }

    // close
    sock_error = closesocket(Sender);
    sock_error = WSACleanup();
    return 0;
}