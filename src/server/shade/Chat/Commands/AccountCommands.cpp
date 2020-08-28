#include "Chat.h"
#include "ChannelMgr.h"
#include "AccountMgr.h"
#include "Language.h"

bool ChatHandler::HandleAccountCommand(char const* /*args*/)
{
    AccountTypes gmLevel = GetSession()->GetSecurity();
    PSendSysMessage(LANG_ACCOUNT_LEVEL, uint32(gmLevel));
    return true;
}

bool ChatHandler::HandleAccountAddonCommand(const char *args)
{
    if (!*args)
    {
        SendSysMessage(LANG_CMD_SYNTAX);
        SetSentErrorMessage(true);
        return false;
    }

    char* exp = strtok((char*)args, " ");

    uint32 accountId = GetSession()->GetAccountId();

    int expansion = atoi(exp); //get int anyway (0 if error)
    if (expansion < 0 || uint8(expansion) > sLogon->getIntConfig(CONFIG_EXPANSION))
    {
        SendSysMessage(LANG_IMPROPER_VALUE);
        SetSentErrorMessage(true);
        return false;
    }

    PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_EXPANSION);

    stmt->setUInt8(0, uint8(expansion));
    stmt->setUInt32(1, accountId);

    LoginDatabase.Execute(stmt);

    PSendSysMessage(LANG_ACCOUNT_ADDON, expansion);
    return true;
};

bool ChatHandler::HandleAccountCreateCommand(const char *args)
{
    if ( !*args )
        return false;

    ///- %Parse the command line arguments
    char *accountName = strtok ( ( char * ) args, " " );
    char *password = strtok ( NULL, " " );
    if ( !accountName || !password )
        return false;

    AccountOpResult result = AccountMgr::CreateAccount ( std::string ( accountName ), std::string ( password ) );
    switch ( result )
    {
    case AOR_OK:
        PSendSysMessage ( LANG_ACCOUNT_CREATED, accountName );
        if ( GetSession() )
            sLog->outChar ( "Account: %d (IP: %s) Character:[%s] (GUID: %u) Change Password.", GetSession()->GetAccountId(),GetSession()->GetRemoteAddress().c_str(), GetSession()->GetPlayer()->GetName(), GetSession()->GetPlayer()->GetGUIDLow() );
        break;
    case AOR_NAME_TOO_LONG:
        SendSysMessage ( LANG_ACCOUNT_TOO_LONG );
        SetSentErrorMessage ( true );
        return false;
    case AOR_NAME_ALREDY_EXIST:
        SendSysMessage ( LANG_ACCOUNT_ALREADY_EXIST );
        SetSentErrorMessage ( true );
        return false;
    case AOR_DB_INTERNAL_ERROR:
        PSendSysMessage ( LANG_ACCOUNT_NOT_CREATED_SQL_ERROR, accountName );
        SetSentErrorMessage ( true );
        return false;
    default:
        PSendSysMessage ( LANG_ACCOUNT_NOT_CREATED, accountName );
        SetSentErrorMessage ( true );
        return false;
    }
    return true;
};

/// Delete a user account and all associated characters in this realm
/// \todo This function has to be enhanced to respect the login/realm split (delete char, delete account chars in realm then delete account)
bool ChatHandler::HandleAccountDeleteCommand(char const* args)
{
    if (!*args)
        return false;

    ///- Get the account name from the command line
    char* account = strtok((char*)args, " ");
    if (!account)
        return false;

    std::string accountName = account;
    if (!AccountMgr::normalizeString(accountName))
    {
        PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    uint32 accountId = AccountMgr::GetId(accountName);
    if (!accountId)
    {
        PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
        SetSentErrorMessage(true);
        return false;
    }

    /// Commands not recommended call from chat, but support anyway
    /// can delete only for account with less security
    /// This is also reject self apply in fact
    if (HasLowerSecurityAccount(NULL, accountId, true))
        return false;

    AccountOpResult result = AccountMgr::DeleteAccount(accountId);
    switch (result)
    {
    case AOR_OK:
        PSendSysMessage(LANG_ACCOUNT_DELETED, accountName.c_str());
        break;
    case AOR_NAME_NOT_EXIST:
        PSendSysMessage(LANG_ACCOUNT_NOT_EXIST, accountName.c_str());
        SetSentErrorMessage(true);
        return false;
    case AOR_DB_INTERNAL_ERROR:
        PSendSysMessage(LANG_ACCOUNT_NOT_DELETED_SQL_ERROR, accountName.c_str());
        SetSentErrorMessage(true);
        return false;
    default:
        PSendSysMessage(LANG_ACCOUNT_NOT_DELETED, accountName.c_str());
        SetSentErrorMessage(true);
        return false;
    }
    return true;
}

bool ChatHandler::HandleAccountLockCommand(char const* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_USE_BOL);
        SetSentErrorMessage(true);
        return false;
    }

    std::string param = (char*)args;

    if (!param.empty())
    {
        PreparedStatement* stmt = LoginDatabase.GetPreparedStatement(LOGIN_UPD_ACCOUNT_LOCK);

        if (param == "on")
        {
            stmt->setBool(0, true);                                     // locked
            PSendSysMessage(LANG_COMMAND_ACCLOCKLOCKED);
        }
        else if (param == "off")
        {
            stmt->setBool(0, false);                                    // unlocked
            PSendSysMessage(LANG_COMMAND_ACCLOCKUNLOCKED);
        }

        stmt->setUInt32(1, GetSession()->GetAccountId());

        LoginDatabase.Execute(stmt);
        return true;
    }

    SendSysMessage(LANG_USE_BOL);
    SetSentErrorMessage(true);
    return false;
}

bool ChatHandler::HandleAccountOnlineListCommand(char const* /*args*/)
{
    ///- Get the list of accounts ID logged to the realm
    PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_ONLINE);
    PreparedQueryResult result = CharacterDatabase.Query(stmt);
    if (!result)
    {
        SendSysMessage(LANG_ACCOUNT_LIST_EMPTY);
        return true;
    }

    ///- Display the list of account/characters online
    SendSysMessage(LANG_ACCOUNT_LIST_BAR_HEADER);
    SendSysMessage(LANG_ACCOUNT_LIST_HEADER);
    SendSysMessage(LANG_ACCOUNT_LIST_BAR);

    ///- Cycle through accounts
    do
    {
        Field* fieldsDB = result->Fetch();
        std::string name = fieldsDB[0].GetString();
        uint32 account = fieldsDB[1].GetUInt32();

        ///- Get the username, last IP and GM level of each account
        // No SQL injection. account is uint32.
        stmt = LoginDatabase.GetPreparedStatement(LOGIN_SEL_ACCOUNT_INFO);
        stmt->setUInt32(0, account);
        PreparedQueryResult resultLogin = LoginDatabase.Query(stmt);

        if (resultLogin)
        {
            Field* fieldsLogin = resultLogin->Fetch();
            PSendSysMessage(LANG_ACCOUNT_LIST_LINE,
                                     fieldsLogin[0].GetCString(), name.c_str(), fieldsLogin[1].GetCString(),
                                     fieldsDB[2].GetUInt16(), fieldsDB[3].GetUInt16(), fieldsLogin[3].GetUInt8(),
                                     fieldsLogin[2].GetUInt8());
        }
        else
            PSendSysMessage(LANG_ACCOUNT_LIST_ERROR, name.c_str());

    } while (result->NextRow());

    SendSysMessage(LANG_ACCOUNT_LIST_BAR);
    return true;
}

bool ChatHandler::HandleAccountPasswordCommand(char const* args)
{
    if (!*args)
    {
        SendSysMessage(LANG_CMD_SYNTAX);
        SetSentErrorMessage(true);
        return false;
    }

    char* oldPassword = strtok((char*)args, " ");
    char* newPassword = strtok(NULL, " ");
    char* passwordConfirmation = strtok(NULL, " ");

    if (!oldPassword || !newPassword || !passwordConfirmation)
    {
        SendSysMessage(LANG_CMD_SYNTAX);
        SetSentErrorMessage(true);
        return false;
    }

    if (!AccountMgr::CheckPassword(GetSession()->GetAccountId(), std::string(oldPassword)))
    {
        SendSysMessage(LANG_COMMAND_WRONGOLDPASSWORD);
        SetSentErrorMessage(true);
        return false;
    }

    if (strcmp(newPassword, passwordConfirmation) != 0)
    {
        SendSysMessage(LANG_NEW_PASSWORDS_NOT_MATCH);
        SetSentErrorMessage(true);
        return false;
    }

    AccountOpResult result = AccountMgr::ChangePassword(GetSession()->GetAccountId(), std::string(newPassword));
    switch (result)
    {
    case AOR_OK:
        SendSysMessage(LANG_COMMAND_PASSWORD);
        break;
    case AOR_PASS_TOO_LONG:
        SendSysMessage(LANG_PASSWORD_TOO_LONG);
        SetSentErrorMessage(true);
        return false;
    default:
        SendSysMessage(LANG_COMMAND_NOTCHANGEPASSWORD);
        SetSentErrorMessage(true);
        return false;
    }

    return true;
}
