#pragma once

#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS

__inline
PCHAR
SQL_STM_CREATE_TABLES(
    VOID
)
{
    return
        // AppCtrl Rules
        "CREATE TABLE IF NOT EXISTS [appctrl_rules] ("
        "  [id] INTEGER PRIMARY KEY AUTOINCREMENT, "
        "  [process_path] TEXT, "
        "  [pid] INT, "
        "  [parent_path] TEXT, "
        "  [parent_pid] INT, "
        "  [verdict] INT, "
        "  [add_time] INTEGER "
        ");"
        "";
}

__inline
PCHAR
SQL_STM_SEARCH_APPCTRL_RULE(
    VOID
)
{
    return
        "SELECT "
        "  [id], [verdict] "
        "FROM "
        "  [appctrl_rules] "
        "WHERE "
        "  (([process_path] IS NULL) OR ([process_path] = ?)) AND "
        "  (([pid] IS NULL) OR ([pid] = ?)) AND "
        "  (([parent_path] IS NULL) OR ([parent_path] = ?)) AND"
        "  (([parent_pid] IS NULL) OR ([parent_pid] = ?)) "
        "ORDER BY"
        "  [add_time] DESC "
        "LIMIT 1;"
        "";
}

__inline
PCHAR
SQL_STM_INSERT_APPCTRL_RULE(
    VOID
)
{
    return
        "INSERT INTO [appctrl_rules] "
        "   ([id], [process_path], [pid], [parent_path], [parent_pid], [verdict], [add_time]) "
        "VALUES "
        "   (?, ?, ?, ?, ?, ?, ?);"
        "";
}

__inline
PCHAR
SQL_STM_DELETE_APPCTRL_RULE(
    VOID
)
{
    return
        "DELETE FROM "
        "  [appctrl_rules] "
        "WHERE "
        "  [id] = ?"
        "";
}

__inline
PCHAR
SQL_STM_UPDATE_APPCTRL_RULE(
    VOID
)
{
    return
        "UPDATE [appctrl_rules] "
        "SET "
        "  [process_path] = ?, "
        "  [pid] = ?, "
        "  [parent_path] = ?, "
        "  [parent_pid] = ?, "
        "  [verdict] = ? "
        "WHERE "
        "  [id] = ?"
        "";
}

__inline
PCHAR
SQL_STM_GET_APPCTRL_RULES(
    VOID
)
{
    return "SELECT * FROM [appctrl_rules];";
}

__inline
PCHAR
SQL_STM_GET_APPCTRL_RULES_COUNT(
    VOID
)
{
    return "SELECT COUNT(*) FROM [appctrl_rules];";
}
