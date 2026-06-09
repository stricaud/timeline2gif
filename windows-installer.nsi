; Timeline Studio Windows Installer
; Build: makensis /DAPP_VERSION=1.2.3 windows-installer.nsi

!ifndef APP_VERSION
  !define APP_VERSION "dev"
!endif

!define APP_NAME    "Timeline Studio"
!define APP_EXE     "Timeline.exe"
!define CLI_EXE     "timeline2gif.exe"
!define PUBLISHER   "Seb Tricaud"
!define REG_KEY     "Software\Microsoft\Windows\CurrentVersion\Uninstall\TimelineStudio"
!define DIST_DIR    "dist"

Name            "${APP_NAME} ${APP_VERSION}"
OutFile         "timeline2gif-windows-x86_64-setup.exe"
InstallDir      "$PROGRAMFILES64\Timeline Studio"
InstallDirRegKey HKLM "Software\Timeline Studio" "InstallDir"
RequestExecutionLevel admin
SetCompressor   /SOLID lzma
Unicode         True

; Use app icon for installer/uninstaller if available
!ifdef ICON_FILE
  Icon          "${ICON_FILE}"
  UninstallIcon "${ICON_FILE}"
!endif

!include "MUI2.nsh"

!define MUI_ABORTWARNING
!ifdef ICON_FILE
  !define MUI_ICON   "${ICON_FILE}"
  !define MUI_UNICON "${ICON_FILE}"
!endif
!define MUI_FINISHPAGE_RUN      "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch Timeline Studio"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Install ────────────────────────────────────────────────────────────────
Section "Timeline Studio" SecMain
    SectionIn RO

    SetOutPath "$INSTDIR"
    File "${DIST_DIR}\${APP_EXE}"
    File "${DIST_DIR}\${CLI_EXE}"
    File "${DIST_DIR}\*.dll"
    File "${DIST_DIR}\QUICKSTART.txt"

    SetOutPath "$INSTDIR\samples"
    File "${DIST_DIR}\samples\*.tig"

    SetOutPath "$INSTDIR\samples\icons"
    File "${DIST_DIR}\samples\icons\*.svg"

    ; Uninstall registry entry (shows in Add/Remove Programs)
    WriteRegStr   HKLM "${REG_KEY}" "DisplayName"     "${APP_NAME}"
    WriteRegStr   HKLM "${REG_KEY}" "DisplayVersion"  "${APP_VERSION}"
    WriteRegStr   HKLM "${REG_KEY}" "Publisher"       "${PUBLISHER}"
    WriteRegStr   HKLM "${REG_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKLM "${REG_KEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
    WriteRegDWORD HKLM "${REG_KEY}" "NoModify" 1
    WriteRegDWORD HKLM "${REG_KEY}" "NoRepair" 1

    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; Start Menu shortcuts
    CreateDirectory "$SMPROGRAMS\Timeline Studio"
    CreateShortCut  "$SMPROGRAMS\Timeline Studio\Timeline Studio.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortCut  "$SMPROGRAMS\Timeline Studio\Uninstall.lnk"       "$INSTDIR\uninstall.exe"
SectionEnd

; ── Uninstall ──────────────────────────────────────────────────────────────
Section "Uninstall"
    Delete "$INSTDIR\${APP_EXE}"
    Delete "$INSTDIR\${CLI_EXE}"
    Delete "$INSTDIR\*.dll"
    Delete "$INSTDIR\QUICKSTART.txt"
    Delete "$INSTDIR\uninstall.exe"

    RMDir /r "$INSTDIR\samples"
    RMDir "$INSTDIR"

    Delete "$SMPROGRAMS\Timeline Studio\Timeline Studio.lnk"
    Delete "$SMPROGRAMS\Timeline Studio\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\Timeline Studio"

    DeleteRegKey HKLM "${REG_KEY}"
    DeleteRegKey /ifempty HKLM "Software\Timeline Studio"
SectionEnd
