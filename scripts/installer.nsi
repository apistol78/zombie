Unicode True

!include "MUI2.nsh"


;--------------------------------
; Custom defines
!define NAME "Zombie"
!define VERSION "1.0.0"
!define SLUG "${NAME} v${VERSION}"
!define PUBLISHER "Anders Pistol"
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${NAME}"

;--------------------------------
; General
Name "${NAME}"
RequestExecutionLevel admin
ManifestDPIAware true

;--------------------------------
; Version information
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${NAME}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "${NAME} Setup"
VIAddVersionKey "CompanyName" "${PUBLISHER}"
VIAddVersionKey "LegalCopyright" "Copyright (C) 2026 ${PUBLISHER}"

;--------------------------------
; UI
;; !define MUI_ICON "assets\captura.ico"
!define MUI_HEADERIMAGE
;; !define MUI_WELCOMEFINISHPAGE_BITMAP "assets\welcome.bmp"
;; !define MUI_HEADERIMAGE_BITMAP "assets\head.bmp"
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "${SLUG} Setup"
!define MUI_FINISHPAGE_TITLE  "${SLUG} Setup"


OutFile "..\Zombie Setup.exe"
InstallDir $PROGRAMFILES64\Zombie


;--------------------------------
; Pages
  
; Installer pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "..\License"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Set UI language
!insertmacro MUI_LANGUAGE "English"


Section

SetOutPath $INSTDIR\bin64
File ..\output\Zombie\Windows\bin64\*.*

SetOutPath $INSTDIR
File ..\output\Zombie\Windows\Application.config
File ..\output\Zombie\Windows\Content.compact

CreateShortCut "$DESKTOP\Zombie.lnk" "$INSTDIR\bin64\Traktor.Runtime.App.exe"

WriteUninstaller $INSTDIR\Uninstaller.exe

; Register in Add/Remove Programs
WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayName" "${NAME}"
WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayVersion" "${VERSION}"
WriteRegStr HKLM "${UNINSTALL_KEY}" "Publisher" "${PUBLISHER}"
WriteRegStr HKLM "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
WriteRegStr HKLM "${UNINSTALL_KEY}" "DisplayIcon" "$INSTDIR\bin64\Traktor.Runtime.App.exe"
WriteRegStr HKLM "${UNINSTALL_KEY}" "UninstallString" '"$INSTDIR\Uninstaller.exe"'
WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify" 1
WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair" 1

SectionEnd

 
Section "Uninstall"

Delete $INSTDIR\bin64\*.*
Delete $INSTDIR\Application.config
Delete $INSTDIR\Content.compact
Delete $INSTDIR\Uninstaller.exe

RMDir $INSTDIR\bin64
RMDir $INSTDIR

Delete "$DESKTOP\Zombie.lnk"

DeleteRegKey HKLM "${UNINSTALL_KEY}"

SectionEnd