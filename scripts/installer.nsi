!include "MUI2.nsh"


;--------------------------------
; Custom defines
!define NAME "Zombie"
!define VERSION "1.0.0"
!define SLUG "${NAME} v${VERSION}"

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

SectionEnd

 
Section "Uninstall"

Delete $INSTDIR\bin64\*.*
Delete $INSTDIR\Application.config
Delete $INSTDIR\Content.compact
Delete $INSTDIR\Uninstaller.exe

RMDir $INSTDIR\bin64
RMDir $INSTDIR

Delete "$DESKTOP\Zombie.lnk"

SectionEnd