; timeline2gif Windows Installer
; Build: makensis /DAPP_VERSION=1.2.3 windows-installer.nsi

!ifndef APP_VERSION
  !define APP_VERSION "dev"
!endif

!define APP_NAME    "timeline2gif Studio"
!define APP_EXE     "t2g-ui.exe"
!define CLI_EXE     "timeline2gif.exe"
!define PUBLISHER   "Seb Tricaud"
!define REG_KEY     "Software\Microsoft\Windows\CurrentVersion\Uninstall\timeline2gif"
!define DIST_DIR    "dist"

Name            "${APP_NAME} ${APP_VERSION}"
OutFile         "timeline2gif-windows-x86_64-setup.exe"
InstallDir      "$PROGRAMFILES64\timeline2gif"
InstallDirRegKey HKLM "Software\timeline2gif" "InstallDir"
RequestExecutionLevel admin
SetCompressor   /SOLID lzma
Unicode         True

!include "MUI2.nsh"

!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN          "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT     "Launch timeline2gif Studio"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ── Install ────────────────────────────────────────────────────────────────
Section "timeline2gif Studio" SecMain
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
    CreateDirectory "$SMPROGRAMS\timeline2gif"
    CreateShortCut  "$SMPROGRAMS\timeline2gif\timeline2gif Studio.lnk" "$INSTDIR\${APP_EXE}"
    CreateShortCut  "$SMPROGRAMS\timeline2gif\Uninstall.lnk"           "$INSTDIR\uninstall.exe"
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

    Delete "$SMPROGRAMS\timeline2gif\timeline2gif Studio.lnk"
    Delete "$SMPROGRAMS\timeline2gif\Uninstall.lnk"
    RMDir  "$SMPROGRAMS\timeline2gif"

    DeleteRegKey HKLM "${REG_KEY}"
    DeleteRegKey /ifempty HKLM "Software\timeline2gif"
SectionEnd
