/* main.c — One Click Game Manager */

#include <switch.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>

#define MAX_ENTRIES 256
#define MAX_VISIBLE 18
#define MAX_PATH_LEN 512

// ---------- ANSI ----------
#define ANSI_RESET   "\x1b[0m"
#define ANSI_BOLD    "\x1b[1m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_RED     "\x1b[31m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_WHITE   "\x1b[37m"
#define ANSI_GRAY    "\x1b[90m"

static inline void ansiClearHome(void)
{
    printf("\x1b[2J\x1b[H");
}

// ---------- Game Structure ----------
typedef struct
{
    u64 titleId;
    char name[128];
    char version[32];
} GameEntry;

GameEntry games[MAX_ENTRIES];
int gameCount = 0;
int selectionIndex = 0;
int startIndex = 0;

PadState pad;
bool g_isAppletMode = false;

// ---------- Folder Stats ----------
typedef struct
{
    int fileCount;
    u64 totalSize;
} FolderStats;

FolderStats getFolderStatsRecursive(const char* basePath)
{
    FolderStats stats = {0, 0};

    DIR* dir = opendir(basePath);
    if (!dir) return stats;

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;

        char fullPath[MAX_PATH_LEN];
        snprintf(fullPath, MAX_PATH_LEN, "%s/%s", basePath, entry->d_name);

        struct stat st;
        if (stat(fullPath, &st) == 0)
        {
            if (S_ISDIR(st.st_mode))
            {
                FolderStats sub = getFolderStatsRecursive(fullPath);
                stats.fileCount += sub.fileCount;
                stats.totalSize += sub.totalSize;
            }
            else
            {
                stats.fileCount++;
                stats.totalSize += st.st_size;
            }
        }
    }

    closedir(dir);
    return stats;
}

void formatSize(u64 bytes, char* out, size_t outSize)
{
    const char* units[] = {"B", "KB", "MB", "GB"};
    double size = (double)bytes;
    int unitIndex = 0;

    while (size > 1024 && unitIndex < 3)
    {
        size /= 1024;
        unitIndex++;
    }

    snprintf(out, outSize, "%.2f %s", size, units[unitIndex]);
}

int folderExists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

int toggleModFolder(const char* titleIdStr, int enable)
{
    char basePath[MAX_PATH_LEN];
    snprintf(basePath, MAX_PATH_LEN,
             "sdmc:/atmosphere/contents/%s", titleIdStr);

    char romfs[MAX_PATH_LEN];
    char romfs_disabled[MAX_PATH_LEN];
    char exefs[MAX_PATH_LEN];
    char exefs_disabled[MAX_PATH_LEN];

    snprintf(romfs, MAX_PATH_LEN, "%s/romfs", basePath);
    snprintf(romfs_disabled, MAX_PATH_LEN, "%s/romfs_disabled", basePath);
    snprintf(exefs, MAX_PATH_LEN, "%s/exefs", basePath);
    snprintf(exefs_disabled, MAX_PATH_LEN, "%s/exefs_disabled", basePath);

    int success = 1;

    if (enable)
    {
        if (folderExists(romfs_disabled))
            if (rename(romfs_disabled, romfs) != 0)
                success = 0;

        if (folderExists(exefs_disabled))
            if (rename(exefs_disabled, exefs) != 0)
                success = 0;
    }
    else
    {
        if (folderExists(romfs))
            if (rename(romfs, romfs_disabled) != 0)
                success = 0;

        if (folderExists(exefs))
            if (rename(exefs, exefs_disabled) != 0)
                success = 0;
    }

    return success;
}

// ---------- NCM ----------
void scanInstalledGames(void)
{
    Result rc = nsInitialize();
    if (R_FAILED(rc))
        return;

    NsApplicationRecord records[MAX_ENTRIES];
    s32 total = 0;

    rc = nsListApplicationRecord(records, MAX_ENTRIES, 0, &total);
    if (R_FAILED(rc))
    {
        nsExit();
        return;
    }

    for (int i = 0; i < total && i < MAX_ENTRIES; i++)
    {
        games[gameCount].titleId = records[i].application_id;

        NsApplicationControlData controlData;
        size_t outSize;

        if (R_SUCCEEDED(nsGetApplicationControlData(
                NsApplicationControlSource_Storage,
                games[gameCount].titleId,
                &controlData,
                sizeof(controlData),
                &outSize)))
        {
            strncpy(games[gameCount].name,
                    (char*)controlData.nacp.lang[0].name,
                    sizeof(games[gameCount].name)-1);

            strncpy(games[gameCount].version,
                    controlData.nacp.display_version,
                    sizeof(games[gameCount].version)-1);
        }
        else
        {
            strcpy(games[gameCount].name, "Unknown");
            strcpy(games[gameCount].version, "Unknown");
        }

        gameCount++;
    }

    nsExit();
}

// ---------- Confirmation ----------
int confirmToggle(const char* action)
{
    printf("\n" ANSI_YELLOW "Are you sure you want to %s this mod?\n" ANSI_RESET, action);
    printf(ANSI_GREEN "A = Yes    " ANSI_RED "B = Cancel\n" ANSI_RESET);
    consoleUpdate(NULL);

    while (appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_A)
            return 1;

        if (kDown & HidNpadButton_B)
            return 0;

        svcSleepThread(10000000);
    }

    return 0;
}

// ---------- UI ----------
void drawMainUI(void)
{
    ansiClearHome();

    printf(ANSI_BOLD ANSI_CYAN "One Click Game Manager (OCGM)" ANSI_RESET "\n");

    if (g_isAppletMode)
    {
        printf(ANSI_RED "Running in Applet Mode\n" ANSI_RESET);
        printf(ANSI_YELLOW "For large mods and games run using Title Override for better stability.\n\n" ANSI_RESET);
    }

    printf(ANSI_YELLOW "Installed Applications" ANSI_RESET "\n\n");

    if (gameCount == 0)
    {
        printf(ANSI_RED "No games found.\n" ANSI_RESET);
        return;
    }

    int endIndex = startIndex + MAX_VISIBLE;
    if (endIndex > gameCount)
        endIndex = gameCount;

    printf(ANSI_GRAY "Showing %d-%d of %d\n\n" ANSI_RESET,
           startIndex + 1, endIndex, gameCount);

    for (int i = startIndex; i < endIndex; i++)
    {
        if (i == selectionIndex)
            printf(ANSI_GREEN "> %s\n" ANSI_RESET, games[i].name);
        else
            printf("  %s\n", games[i].name);
    }

    printf("\n" ANSI_MAGENTA "A = Details  |  + = Exit" ANSI_RESET "\n");
}

void showDetails(int index)
{
    int needsScan = 1;
    FolderStats cachedStats = {0,0};

    while (1)
    {
        ansiClearHome();

        char titleIdStr[17];
        snprintf(titleIdStr, sizeof(titleIdStr), "%016llX", games[index].titleId);

        char basePath[MAX_PATH_LEN];
        snprintf(basePath, MAX_PATH_LEN,
                 "sdmc:/atmosphere/contents/%s", titleIdStr);

        char romfs[MAX_PATH_LEN];
        char romfs_disabled[MAX_PATH_LEN];
        char exefs[MAX_PATH_LEN];
        char exefs_disabled[MAX_PATH_LEN];

        snprintf(romfs, MAX_PATH_LEN, "%s/romfs", basePath);
        snprintf(romfs_disabled, MAX_PATH_LEN, "%s/romfs_disabled", basePath);
        snprintf(exefs, MAX_PATH_LEN, "%s/exefs", basePath);
        snprintf(exefs_disabled, MAX_PATH_LEN, "%s/exefs_disabled", basePath);

        int modActive = folderExists(romfs) || folderExists(exefs);
        int modDisabled = folderExists(romfs_disabled) || folderExists(exefs_disabled);

        if (needsScan && (modActive || modDisabled))
        {
            cachedStats = getFolderStatsRecursive(basePath);
            needsScan = 0;
        }

        printf(ANSI_BOLD ANSI_CYAN "%s\n\n" ANSI_RESET, games[index].name);
        printf(ANSI_YELLOW "Title ID: " ANSI_WHITE "%s\n" ANSI_RESET, titleIdStr);
        printf(ANSI_YELLOW "Version:  " ANSI_WHITE "%s\n\n" ANSI_RESET, games[index].version);

        if (modActive || modDisabled)
        {
            if (modActive)
                printf(ANSI_GREEN "Mod Installed (Enabled)\n\n" ANSI_RESET);
            else
                printf(ANSI_YELLOW "Mod Installed (Disabled)\n\n" ANSI_RESET);

            char totalSizeStr[32];
            formatSize(cachedStats.totalSize, totalSizeStr, sizeof(totalSizeStr));

            printf(ANSI_CYAN "Total Mod Size: " ANSI_WHITE "%s\n" ANSI_RESET, totalSizeStr);
            printf(ANSI_CYAN "Total Files:    " ANSI_WHITE "%d\n\n" ANSI_RESET, cachedStats.fileCount);

            printf("\n" ANSI_MAGENTA "Y = %s Mod\n" ANSI_RESET,
                   modActive ? "Disable" : "Enable");
        }
        else
        {
            printf(ANSI_RED "No Mod Installed\n" ANSI_RESET);
        }

        printf("\n" ANSI_MAGENTA "B = Back" ANSI_RESET "\n");
        consoleUpdate(NULL);

        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_B)
            return;

        if ((kDown & HidNpadButton_Y) && (modActive || modDisabled))
        {
            const char* action = modActive ? "disable" : "enable";

            if (confirmToggle(action))
            {
                ansiClearHome();
                printf(ANSI_YELLOW "Processing...\n" ANSI_RESET);
                consoleUpdate(NULL);

                int success = toggleModFolder(titleIdStr, modDisabled);

                needsScan = 1; // force rescan after toggle

                ansiClearHome();
                if (success)
                    printf(ANSI_GREEN "Mod successfully %sd!\n" ANSI_RESET, action);
                else
                    printf(ANSI_RED "Failed to %s mod!\n" ANSI_RESET, action);

                consoleUpdate(NULL);
                svcSleepThread(800000000);
                return;
            }
        }

        svcSleepThread(10000000);
    }
}

// ---------- Main ----------
int main(int argc, char* argv[])
{
    consoleInit(NULL);

    AppletType type = appletGetAppletType();
    g_isAppletMode = (type != AppletType_Application);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad);

    scanInstalledGames();
    drawMainUI();
    consoleUpdate(NULL);

    while (appletMainLoop())
    {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        bool needsRedraw = false;

        if (kDown & HidNpadButton_Down)
        {
            if (selectionIndex < gameCount - 1)
            {
                selectionIndex++;
                if (selectionIndex >= startIndex + MAX_VISIBLE)
                    startIndex++;
                needsRedraw = true;
            }
        }

        if (kDown & HidNpadButton_Up)
        {
            if (selectionIndex > 0)
            {
                selectionIndex--;
                if (selectionIndex < startIndex)
                    startIndex--;
                needsRedraw = true;
            }
        }

        if (kDown & HidNpadButton_A)
        {
            showDetails(selectionIndex);
            needsRedraw = true;
        }

        if (kDown & HidNpadButton_Plus)
            break;

        if (needsRedraw)
        {
            drawMainUI();
            consoleUpdate(NULL);
        }

        svcSleepThread(10000000);
    }

    printf(ANSI_RESET);
    consoleExit(NULL);
    return 0;
}