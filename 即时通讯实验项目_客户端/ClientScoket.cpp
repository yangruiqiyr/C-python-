#include "stdafx.h"
#include "ClientScoket.h"
#include <WS2tcpip.h>

CClientSocket::CClientSocket()
{
	m_hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
}


CClientSocket::~CClientSocket()
{
}

bool CClientSocket::ConnectServer(char * szIp, WORD port)
{
	//1.初始化套接字动态库
	WSADATA wsd;
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0) {
		return false;
	}
	//检查返回的库是否是2.2
	if (LOBYTE(wsd.wVersion) != 2 || HIBYTE(wsd.wHighVersion) != 2) {
		WSACleanup();
		return false;
	}
	//2.创建套接字
	m_sClient = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sClient == INVALID_SOCKET) {
		WSACleanup();
		return false;
	}
	//3.设置服务器地址
	SOCKADDR_IN servAddr = {};//服务器地址
	servAddr.sin_family = AF_INET;
	//servAddr.sin_addr.s_addr = inet_addr(szIp);
	inet_pton(AF_INET, szIp, &servAddr.sin_addr);
	//szIp = inet_ntop(AF_INET, &servAddr.sin_addr.s_addr, str, sizeof(str));
	servAddr.sin_port = htons((short)port);
	//6.连接服务器
	//SOCKERT sService;//跟服务端相连的套接字
	int nRetValue = connect(m_sClient, (sockaddr*)&servAddr, sizeof(SOCKADDR_IN));
	if (nRetValue == SOCKET_ERROR) {
		closesocket(m_sClient);
		m_sClient = NULL;
		WSACleanup();
		return false;
	}
	return true;
}


char * CClientSocket::Recv()
{
	CHATSEND ct = {};

	if (SOCKET_ERROR == recv(m_sClient, (char*)&ct, sizeof(CHATSEND), NULL)) {
		printf("recv error!\n");
		return nullptr;
	}
	m_pObjChatRecv = &ct;
	switch (ct.m_type) {
	case ANONYMOUS:
		return RecvForAnonumous();
	case CHAT:
		return RecvForChat();
		break;
	case ONE2ONE:
		return RecvForOne2One();
		break;
	case REGISTER:
		return RecvForRegister();
	case LOGIN:
		return RecvForLogin();
    case ADDFRIEND:
    	return RecvForAddFriend();
    case SEARCHUSER:
    	return RecvForSearchUser();
	case FILETRANS:
		return RecvForFiletrans();
	break;
    case MSGRECORD:
    	return RecvForGetMsgRecord();
	case UPDATEUSER:
		return RecvForUpdataUserList();
	default:
		break;
	}
	return nullptr;
}

bool CClientSocket::Send(CHATPURPOSE purpose, char * bufSend, DWORD dwLen, SOCKET client)
{
	switch (purpose)
	{
	case ANONYMOUS:
		SendForAnonymous(bufSend, dwLen);
		break;
	case CHAT:
		SendForChat(bufSend, dwLen);
		break;
	case ONE2ONE:
		SendForOne2One(bufSend, dwLen);
		break;
	case REGISTER:
		SendForRegister(bufSend,dwLen);
		break;
	case LOGIN:
		SendForLogin(bufSend, dwLen);
		break;
    case ADDFRIEND:
    	SendForAddFriend(bufSend, dwLen);
    	break;
    case SEARCHUSER:
    	SendForSearchUser(bufSend, dwLen);
    	break;
	case FILETRANS:
		SendForFiletrans(bufSend, dwLen, client);
		break;
    case MSGRECORD:
    	SendForGetMsgRecord(bufSend, dwLen);
    	break;
	case UPDATEUSER:
		break;
	default:
		break;
	}
	return false;
}

bool CClientSocket::Close()
{
	WSACleanup();
	closesocket(m_sClient);
	return true;
}

char* CClientSocket::RecvForFiletrans()
{
	static long long i = 0;
	char* buf = nullptr;
	buf = m_pObjChatRecv->m_content.trs.szContent;
	if (i == 0)
	{
		i++;
		ReceiveFile.open(buf, ios::out | ios::binary);
	}
	else if (strcmp(buf, "over") == 0)
	{
		MessageBox(NULL, L"文件接收成功", L"提示", MB_OK);
		i = 0;
		ReceiveFile.close();
	}
	else
	{
		ReceiveFile.write(buf, 1024);
	}
	return nullptr;
}

char * CClientSocket::RecvForAnonumous()
{
	sprintf_s(m_bufRecv, BUFMSG, "%s加入聊天室！\n", m_pObjChatRecv->m_content.any.buf);
	return m_bufRecv;
}

char * CClientSocket::RecvForChat()
{
	for (int i = 0;i < m_pObjChatRecv->m_content.chat.dwLen;i++) {
		m_pObjChatRecv->m_content.chat.buf[i] ^= 15;
	}
	strcpy_s(m_bufRecv, m_pObjChatRecv->m_content.chat.buf);
	return m_bufRecv;
}

char * CClientSocket::RecvForUpdataUserList()
{
	//新用户加入，更新到用户列表窗口
	m_pObjUpdate = new CHATUPDATEUSER;
	memcpy_s(m_pObjUpdate, sizeof(CHATUPDATEUSER), &m_pObjChatRecv->m_content.upd, sizeof(CHATUPDATEUSER));
	return nullptr;
}

char * CClientSocket::RecvForOne2One()
{//新用户加入，更新到用户列表窗口
	m_pObjOne2One = new CHATONE2ONE;
	memcpy_s(m_pObjOne2One, sizeof(CHATONE2ONE), &m_pObjChatRecv->m_content.o2o, sizeof(CHATONE2ONE));
		return nullptr;
}

char * CClientSocket::RecvForRegister()
{
	if (!strcmp(m_pObjChatRecv->m_content.log.szName, "注册成功!")) {
		return "注册成功！";

	}
	else {
		return nullptr;
	}
	
}

char * CClientSocket::RecvForLogin()
{
	if (!strcmp(m_pObjChatRecv->m_content.log.szName, "登录成功!")) {
		return "登录成功！";

	}
	else {
		return nullptr;
	}
}

char * CClientSocket::RecvForAddFriend()
{
	MessageBoxA(NULL, m_pObjChatRecv->m_content.adf.szName, "添加好友",MB_OK);
	return nullptr;
}

char * CClientSocket::RecvForSearchUser()
{
	MessageBoxA(NULL, m_pObjChatRecv->m_content.seu.szName, "搜索用户", MB_OK);
	return nullptr;
}

char * CClientSocket::RecvForGetMsgRecord()
{
	if (!strcmp(m_pObjChatRecv->m_content.rec.szFrom, "~~~end~~~")) {
		SetEvent(m_hEvent);
	}
	else {
		m_vecMsgRecord.push_back(m_pObjChatRecv->m_content.rec);
	}
	return nullptr;
}

void CClientSocket::SendForAnonymous(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { ANONYMOUS };
	//匿名 长度+昵称
	strcpy_s(ct.m_content.chat.buf, bufSend);
	ct.m_content.chat.dwLen = dwLen;
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);
}

void CClientSocket::SendForChat(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { CHAT };
	//聊天 长度加姓名：内容
	strcpy_s(ct.m_content.chat.buf, m_szName);
	strcat_s(ct.m_content.chat.buf, ":");
	strcat_s(ct.m_content.chat.buf, bufSend);
	ct.m_content.chat.dwLen = strlen(ct.m_content.chat.buf) + 1;
	//加密
	for (int i = 0;i < ct.m_content.any.dwLen;i++) {
		ct.m_content.chat.buf[i] ^= 15;
	}
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);
}

void CClientSocket::SendForOne2One(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { ONE2ONE };
	char* nextToken = nullptr;
	char* szContext = strtok_s(bufSend, ":", &nextToken);
	memcpy_s(ct.m_content.o2o.szName, nextToken - bufSend, bufSend, nextToken - bufSend);
	memcpy_s(ct.m_content.o2o.szContent, strlen(nextToken), nextToken, strlen(nextToken));
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);

}

void CClientSocket::SendForRegister(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { REGISTER };
	char* pwd = nullptr;
	strtok_s(bufSend, ":", &pwd);
	memcpy_s(ct.m_content.reg.szName, pwd - bufSend, bufSend, pwd - bufSend);
	memcpy_s(ct.m_content.reg.szPwd, strlen(pwd), pwd, strlen(pwd));
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);
}
void CClientSocket::SendForLogin(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { LOGIN };
	char* pwd = nullptr;
	strtok_s(bufSend, ":", &pwd);
	memcpy_s(ct.m_content.log.szName, pwd - bufSend, bufSend, pwd - bufSend);
	memcpy_s(ct.m_content.log.szPwd, strlen(pwd), pwd, strlen(pwd));
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);
}

void CClientSocket::SendForAddFriend(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { ADDFRIEND };
	char* frd = nullptr;
	//构造内容 谁要添加：要添加谁
	strtok_s(bufSend, ":", &frd);
	memcpy_s(ct.m_content.adf.szName, frd - bufSend, bufSend, frd - bufSend);
	memcpy_s(ct.m_content.adf.szFrd, strlen(frd), frd, strlen(frd));
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);
}

void CClientSocket::SendForSearchUser(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { SEARCHUSER };
	memcpy_s(ct.m_content.seu.szName, dwLen, bufSend, dwLen);
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);
}

void CClientSocket::SendForGetMsgRecord(char * bufSend, DWORD dwLen)
{
	CHATSEND ct = { MSGRECORD };
	//服务器保存有当前客户端的名称，所以查询所以记录只需要发送消息模型就行
	send(m_sClient, (char*)&ct, sizeof(ct), NULL);
}

void CClientSocket::SendForFiletrans(char * bufSend, DWORD dwLen, SOCKET client)
{
	CHATSEND ct = { FILETRANS };
	//姓名：内容
	PCHAR nextToken = nullptr;
	PCHAR szContext = strtok_s(bufSend, ":", &nextToken);
	//SOCKET temp = m_sClient;
	memcpy_s(ct.m_content.trs.szName, nextToken - bufSend, bufSend, nextToken - bufSend);
	memcpy_s(ct.m_content.trs.szContent, strlen(nextToken), nextToken, strlen(nextToken));
	send(client, (PCHAR)&ct, sizeof(ct), NULL);
}
