#include "stdafx.h"
#include "LoginProcess.h"

#define REGIST_PACKET_PROCESS(type) runFuncTable_.insert(make_pair(E_##type, &LoginProcess::##type))

LoginProcess::LoginProcess()
{
	registSubPacketFunc();
}

void LoginProcess::registSubPacketFunc()
{
	REGIST_PACKET_PROCESS(C_REQ_ID_PW);
	REGIST_PACKET_PROCESS(I_DB_ANS_ID_PW);
	REGIST_PACKET_PROCESS(I_LOGIN_NOTIFY_ID_LOADED);
}

//---------------------------------------------------------------//
//패킷 처리 정의
void LoginProcess::C_REQ_ID_PW(Session *session, Packet *rowPacket)
{
	PK_C_REQ_ID_PW *packet = (PK_C_REQ_ID_PW *)rowPacket;

	PK_I_DB_REQ_ID_PW dbPacket;
	dbPacket.clientId_ = (UInt64)session->id();
	dbPacket.id_ = packet->id_;
	dbPacket.password_ = packet->password_;

	Terminal *terminal = _terminal.get(L"DBAgent");
	terminal->sendPacket(&dbPacket);
}

void LoginProcess::I_DB_ANS_ID_PW(Session *session, Packet *rowPacket)
{
	PK_I_DB_ANS_ID_PW *packet = (PK_I_DB_ANS_ID_PW  *)rowPacket;
	Log(L"* id/ pw result = %d", packet->result_);

	Session *clientSession = SESSION_MANAGER.session(packet->clientId_);
	if (clientSession == nullptr) {
		return;
	}

	const int authFail = 0;
	if (packet->result_ == authFail) {
		PK_S_ANS_ID_PW_FAIL ansPacket;
		clientSession->sendPacket(&ansPacket);
		return;
	}
	
	PK_I_CHTTING_NOTIFY_ID iPacket;
	iPacket.oidAccountId_ = packet->oidAccountId_;
	iPacket.clientId_ = packet->clientId_;
	Terminal *terminal = _terminal.get(L"ContentServer");
	if (terminal == nullptr) {
		Log(L"! Chatting Server terminal is not connected");
	}
	terminal->sendPacket(&iPacket);
}

void LoginProcess::I_LOGIN_NOTIFY_ID_LOADED(Session *session, Packet *rowPacket)
{
	PK_I_LOGIN_NOTIFY_ID_LOADED *packet = (PK_I_LOGIN_NOTIFY_ID_LOADED *)rowPacket;
	
	const int dataNull = 0;
	// msg failover 처리가 제대로 되지 않음
	// 데이터 없을 시 그대로 return 문으로 돌아감
	if (packet->result_ == dataNull) {
		return;
	}
	Session *clientSession = SESSION_MANAGER.session(packet->clientId_);
	if (clientSession == nullptr) {
		return;
	}
	Terminal *terminal = _terminal.get(L"ContentServer");
	if (terminal == nullptr) {
		Log(L"! Chatting Server terminal is not connected");
	}
	PK_S_ANS_ID_PW_SUCCESS ansPacket;
	ansPacket.ip_ = terminal->ip();
	ansPacket.port_ = terminal->port();
	ansPacket.name_ = packet->name_;

	Log(L"* loaded [%S] user name, from [%s]", ansPacket.name_.c_str(), session->clientAddress().c_str());
	clientSession->sendPacket(&ansPacket);
}