; Script based on one generated by the HM NIS Edit Script Wizard.

; file association rocks!
!macro registerExtension executable extension description
       Push "${executable}"  ; "full path to my.exe"
       Push "${extension}"   ;  ".mkv"
       Push "${description}" ;  "MKV File"
       Call _registerExtension
!macroend

; back up old value of .opt
Function _registerExtension
  !define Index "Line${__LINE__}"
  pop $R0 ; ext name
  pop $R1
  pop $R2
  push $1
  push $0
  ReadRegStr $1 HKCR $R1 ""
  StrCmp $1 "" "${Index}-NoBackup"
    StrCmp $1 "OptionsFile" "${Index}-NoBackup"
    WriteRegStr HKCR $R1 "backup_val" $1
"${Index}-NoBackup:"
  WriteRegStr HKCR $R1 "" $R0
  ReadRegStr $0 HKCR $R0 ""
  StrCmp $0 "" 0 "${Index}-Skip"
	WriteRegStr HKCR $R0 "" $R0
	WriteRegStr HKCR "$R0\shell" "" "open"
;	WriteRegStr HKCR "$R0\DefaultIcon" "" "$R2,0"
"${Index}-Skip:"
  WriteRegStr HKCR "$R0\shell\open\command" "" '"$R2" "%1"'
  WriteRegStr HKCR "$R0\shell\edit" "" "Edit $R0"
  WriteRegStr HKCR "$R0\shell\edit\command" "" '"$R2" "%1"'
  pop $0
  pop $1
  !undef Index
FunctionEnd

!macro unregisterExtension extension description
       Push "${extension}"   ;  ".mkv"
       Push "${description}"   ;  "MKV File"
       Call un._unregisterExtension
!macroend

Function un._unregisterExtension
  pop $R1 ; description
  pop $R0 ; extension
  !define Index "Line${__LINE__}"
  ReadRegStr $1 HKCR $R0 ""
  StrCmp $1 $R1 0 "${Index}-NoOwn" ; only do this if we own it
  ReadRegStr $1 HKCR $R0 "backup_val"
  StrCmp $1 "" 0 "${Index}-Restore" ; if backup="" then delete the whole key
  DeleteRegKey HKCR $R0
  Goto "${Index}-NoOwn"
"${Index}-Restore:"
  WriteRegStr HKCR $R0 "" $1
  DeleteRegValue HKCR $R0 "backup_val"
  DeleteRegKey HKCR $R1 ;Delete key with association name settings
"${Index}-NoOwn:"
  !undef Index
FunctionEnd


; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Edyuk"
!define PRODUCT_VERSION "1.1.0"
!define PRODUCT_WEB_SITE "http://www.edyuk.org"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\edyuk.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

SetCompressor lzma

; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "src\exec\edyuk.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME

; License page
!define MUI_LICENSEPAGE_CHECKBOX
!insertmacro MUI_PAGE_LICENSE "GPL.txt"

; Components page
!insertmacro MUI_PAGE_COMPONENTS

; Directory page
!insertmacro MUI_PAGE_DIRECTORY

; Qt page
Var QT_GUI_TITLE
Var QT_TEXT
Var QT_FOLDER
!define MUI_DIRECTORYPAGE_VARIABLE          $QT_FOLDER   ;selected by user
!define MUI_DIRECTORYPAGE_TEXT_DESTINATION  $QT_TEXT     ;descriptive text
!define MUI_DIRECTORYPAGE_TEXT_TOP          $QT_GUI_TITLE  ; GUI page title
!insertmacro MUI_PAGE_DIRECTORY

; custom page
; page custom QtCheck "" ": Qt options"

; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!define MUI_FINISHPAGE_RUN "$INSTDIR\edyuk.exe"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\doc\README.htm"
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
; !insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "English"

; Reserve files
!insertmacro MUI_RESERVEFILE_INSTALLOPTIONS

; MUI end ------

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "edyuk-install.exe"
InstallDir "$PROGRAMFILES\Edyuk"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Function .onInit
  #InitPluginsDir
  #File /oname=$PLUGINSDIR\options.ini "edyuk-options.ini"
  ReadRegStr $R0 HKLM  "${PRODUCT_UNINST_KEY}" "UninstallString"
  StrCmp $R0 "" done

  ReadRegStr $R1 HKLM "${PRODUCT_UNINST_KEY}" "DisplayVersion"
  StrCmp $R1 "${PRODUCT_VERSION}" sameversion
  
  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "${PRODUCT_NAME} is already installed in a different version. $\n$\nClick `OK` to remove the \
  previous version or `Cancel` to cancel this upgrade." \
  IDOK uninst
  
  Abort

  sameversion:

  MessageBox MB_OKCANCEL|MB_ICONEXCLAMATION \
  "The same version of ${PRODUCT_NAME} is already installed. $\n$\nClick `OK` to restore \
  the previous installation or `Cancel` to keep it as is." \
  IDOK uninst
  Abort
  
  ;Run the uninstaller
  uninst:
  Exec $R0
  done:
  
  StrCpy $QT_GUI_TITLE "Select Qt 4 install path"
  StrCpy $QT_TEXT "Qt 4 path (used for SDK installation only)"
  StrCpy $QT_FOLDER ""    ;default path
FunctionEnd

#Function QtCheck
#  InstallOptions::dialog $PLUGINSDIR\options.ini
#  Pop $R1
#  StrCmp $R1 "cancel" done
#  StrCmp $R1 "back" done
#  StrCmp $R1 "success" done
#  error: MessageBox MB_OK|MB_ICONSTOP "InstallOptions error:$\r$\n$R1"
#  done:
#FunctionEnd

Section "Core" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File "edyuk.exe"
  CreateDirectory "$SMPROGRAMS\Edyuk"
  CreateShortCut "$SMPROGRAMS\Edyuk\Edyuk.lnk" "$INSTDIR\edyuk.exe"
  CreateShortCut "$DESKTOP\Edyuk.lnk" "$INSTDIR\edyuk.exe"
  File "edyuk.dll"
  File /nonfatal "mingwm10.dll"
  File /nonfatal "Qt*.dll"
SectionEnd

Section "Plugins" SEC02
  SetOutPath "$INSTDIR\plugins"
  File "plugins\*.dll"
SectionEnd

Section "Syntax" SEC03
  SetOutPath "$INSTDIR\qxs"
  File "qxs\*.qnfa"
  File "qxs\marks.qxm"
  File "qxs\formats.qxf"
SectionEnd

Section "Templates" SEC04
  SetOutPath "$INSTDIR\templates"
  File "templates\*.ui"
  File "templates\*.template"
  File "templates\*.pro"
  File "templates\*.main"
  File "templates\*.qrc"
  File "templates\*.xml"
  File "templates\*.extra"
  File "templates\*.cpp"
  File "templates\*.h"
SectionEnd

Section "Sources" SEC05
  SetOutPath "$INSTDIR\src\exec"
  SetOverwrite try
  File /nonfatal "src\exec\edyuk.ico"
  File /nonfatal "src\exec\edyuk.rc"
  File /nonfatal "src\exec\exec.pro"
  File /nonfatal "src\exec\main.cpp"
  SetOutPath "$INSTDIR\src\lib"
  File /nonfatal "src\lib\*.pro"
  File /nonfatal "src\lib\*.h"
  File /nonfatal "src\lib\*.cpp"
  File /nonfatal "src\lib\*.dox"
  SetOutPath "$INSTDIR\src\lib\images"
  File /nonfatal "src\lib\images\*.png"
  File /nonfatal "src\lib\images\*.svg"
  File /nonfatal "src\lib\images\*.qrc"
  SetOutPath "$INSTDIR\src\lib\images\completion"
  File /nonfatal "src\lib\images\completion\*.png"
  SetOutPath "$INSTDIR\src\lib\ui"
  File /nonfatal "src\lib\ui\*.ui"
  SetOutPath "$INSTDIR\src\plugins\assistant"
  File /nonfatal "src\plugins\assistant\*.pro"
  File /nonfatal "src\plugins\assistant\*.xml"
  File /nonfatal "src\plugins\assistant\*.qrc"
  File /nonfatal "src\plugins\assistant\*.ui"
  File /nonfatal "src\plugins\assistant\*.h"
  File /nonfatal "src\plugins\assistant\*.cpp"
  SetOutPath "$INSTDIR\src\plugins\default"
  File /nonfatal "src\plugins\default\*.pro"
  File /nonfatal "src\plugins\default\*.xml"
  File /nonfatal "src\plugins\default\*.qrc"
  File /nonfatal "src\plugins\default\*.ui"
  File /nonfatal "src\plugins\default\*.h"
  File /nonfatal "src\plugins\default\*.cpp"
  SetOutPath "$INSTDIR\src\plugins\default\qmake"
  File /nonfatal "src\plugins\default\qmake\*.h"
  File /nonfatal "src\plugins\default\qmake\*.cpp"
  SetOutPath "$INSTDIR\src\plugins\designer"
  File /nonfatal "src\plugins\designer\*.pro"
  File /nonfatal "src\plugins\designer\*.xml"
  File /nonfatal "src\plugins\designer\*.qrc"
  File /nonfatal "src\plugins\designer\*.h"
  File /nonfatal "src\plugins\designer\*.cpp"
  SetOutPath "$INSTDIR\src\plugins\gdb"
  File /nonfatal "src\plugins\gdb\*.pro"
  File /nonfatal "src\plugins\gdb\*.xml"
  File /nonfatal "src\plugins\gdb\*.qrc"
  File /nonfatal "src\plugins\gdb\*.ui"
  File /nonfatal "src\plugins\gdb\*.h"
  File /nonfatal "src\plugins\gdb\*.cpp"
  SetOutPath "$INSTDIR\src\plugins\vimacs"
  File /nonfatal "src\plugins\vimacs\*.pro"
  File /nonfatal "src\plugins\vimacs\*.xml"
  File /nonfatal "src\plugins\vimacs\*.qrc"
  File /nonfatal "src\plugins\vimacs\*.ui"
  File /nonfatal "src\plugins\vimacs\*.h"
  File /nonfatal "src\plugins\vimacs\*.cpp"
  SetOutPath "$INSTDIR\src\qplugin_generator"
  File /nonfatal "src\qplugin_generator\*.pro"
  File /nonfatal "src\qplugin_generator\*.qrc"
  File /nonfatal "src\qplugin_generator\*.h"
  File /nonfatal "src\qplugin_generator\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2\example"
  File /nonfatal "3rdparty\qcodeedit2\example\*.pro"
  File /nonfatal "3rdparty\qcodeedit2\example\*.ui"
  File /nonfatal "3rdparty\qcodeedit2\example\*.h"
  File /nonfatal "3rdparty\qcodeedit2\example\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2"
  File /nonfatal "3rdparty\qcodeedit2\example.sh"
  File /nonfatal "3rdparty\qcodeedit2\*.txt"
  File /nonfatal "3rdparty\qcodeedit2\*.pro"
  File /nonfatal "3rdparty\qcodeedit2\*.pri"
  File /nonfatal "3rdparty\qcodeedit2\*.prf"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2\lib\document"
  File /nonfatal "3rdparty\qcodeedit2\lib\document\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\document\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2\lib\language"
  File /nonfatal "3rdparty\qcodeedit2\lib\language\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\language\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2\lib"
  File /nonfatal "3rdparty\qcodeedit2\lib\lib.pri"
  File /nonfatal "3rdparty\qcodeedit2\lib\lib.pro"
  File /nonfatal "3rdparty\qcodeedit2\lib\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2\lib\qnfa"
  File /nonfatal "3rdparty\qcodeedit2\lib\qnfa\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\qnfa\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2\lib\widgets"
  File /nonfatal "3rdparty\qcodeedit2\lib\widgets\*.ui"
  File /nonfatal "3rdparty\qcodeedit2\lib\widgets\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\widgets\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcodeedit2\qxs"
  File /nonfatal "3rdparty\qcodeedit2\qxs\*.qnfa"
  File /nonfatal "3rdparty\qcodeedit2\qxs\formats.qxf"
  File /nonfatal "3rdparty\qcodeedit2\qxs\marks.qxm"
  SetOutPath "$INSTDIR\3rdparty\qcodemodel2"
  File /nonfatal "3rdparty\qcodemodel2\*.pri"
  File /nonfatal "3rdparty\qcodemodel2\*.h"
  File /nonfatal "3rdparty\qcodemodel2\*.cpp"
  SetOutPath "$INSTDIR\3rdparty\qcumber"
  File /nonfatal "3rdparty\qcumber\*.cpp"
  File /nonfatal "3rdparty\qcumber\*.h"
  File /nonfatal "3rdparty\qcumber\*.pri"
  File /nonfatal "3rdparty\qcumber\*.pro"
  File /nonfatal "3rdparty\qcumber\*.ui"
  SetOutPath "$INSTDIR\3rdparty\qmdi"
  File /nonfatal "3rdparty\qmdi\*.cpp"
  File /nonfatal "3rdparty\qmdi\*.h"
  File /nonfatal "3rdparty\qmdi\*.pri"
  SetOutPath "$INSTDIR\3rdparty\qpluginsystem"
  File /nonfatal "3rdparty\qpluginsystem\*.cpp"
  File /nonfatal "3rdparty\qpluginsystem\*.h"
  File /nonfatal "3rdparty\qpluginsystem\*.pri"
  SetOutPath "$INSTDIR\3rdparty\qprojectmodel2"
  File /nonfatal "3rdparty\qprojectmodel2\*.cpp"
  File /nonfatal "3rdparty\qprojectmodel2\*.h"
  File /nonfatal "3rdparty\qprojectmodel2\*.pri"
  SetOutPath "$INSTDIR"
  SetOverwrite ifnewer
  File /nonfatal "*.desktop"
  File /nonfatal "write_log"
  File /nonfatal "*.bat"
  File /nonfatal "*.pri"
  File /nonfatal "*.xml"
  File /nonfatal "*.pro"
  File /nonfatal "*.nsi"
  File /nonfatal "*.dox"
  File /nonfatal "edyuk"
  File /nonfatal "Doxyfile"
  File /nonfatal "debug"
  File /nonfatal "*.sh"
  File /nonfatal "build"
SectionEnd

Section "Documentation" SEC06
  SetOutPath "$INSTDIR"
  File "TODO.txt"
  File "STANDARDS.txt"
  File "README.txt"
  File "GPL.txt"
  File "CHANGELOG.txt"
  SetOutPath "$INSTDIR\doc"
  File /nonfatal "doc\*.*"
SectionEnd

Section "SDK" SEC07
  # ReadINIStr $0 "$PLUGINSDIR\edyuk-options.ini" "field 2" "state"
  SetOutPath "$QT_FOLDER\bin"
  File "qplugin_generator.exe"
  SetOutPath "$QT_FOLDER\lib\Edyuk"
  File "libedyuk.a"
  SetOutPath "$QT_FOLDER\mkspecs\features"
  File "installs\features\*.prf"
  SetOutPath "$QT_FOLDER\include\Edyuk"
  File /nonfatal "src\lib\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\document\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\language\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\qnfa\*.h"
  File /nonfatal "3rdparty\qcodeedit2\lib\widgets\*.h"
  File /nonfatal "3rdparty\qcodemodel2\*.h"
  File /nonfatal "3rdparty\qcumber\*.h"
  File /nonfatal "3rdparty\qmdi\*.h"
  File /nonfatal "3rdparty\qpluginsystem\*.h"
  File /nonfatal "3rdparty\qprojectmodel2\*.h"
SectionEnd

SectionGroup "File association" SEC08
  ;Section -Common
  ;  WriteRegStr HKCR "${PRODUCT_NAME}\Shell\open\command" "" '"$INSTDIR\edyuk.exe" %1'
  ;SectionEnd
  
  Section "C++" S08S1
    !insertmacro registerExtension "${PRODUCT_NAME}" ".cpp" "C++ source file"
    !insertmacro registerExtension "${PRODUCT_NAME}" ".hpp" "C++ header file"
    !insertmacro registerExtension "${PRODUCT_NAME}" ".c" "C source file"
    !insertmacro registerExtension "${PRODUCT_NAME}" ".h" "C header file"
  SectionEnd
  
  Section "Qt" S08S2
    !insertmacro registerExtension "${PRODUCT_NAME}" ".pro" "qmake project file"
    !insertmacro registerExtension "${PRODUCT_NAME}" ".pri" "qmake project include file"
    !insertmacro registerExtension "${PRODUCT_NAME}" ".qrc" "Qt resource file"
  SectionEnd
  
  Section "QCodeEdit" S08S3
    !insertmacro registerExtension "${PRODUCT_NAME}" ".qnfa" "QCodeEdit syntax file"
    !insertmacro registerExtension "${PRODUCT_NAME}" ".qxf" "QCodeEdit format file"
    !insertmacro registerExtension "${PRODUCT_NAME}" ".qxm" "QCodeEdit line marks file"
  SectionEnd
SectionGroupEnd

Section "Translations" SEC09
  SetOutPath "$INSTDIR\translations"
  File "translations\*.qm"
  File "translations\*.ts"
SectionEnd

Section -AdditionalIcons
  SetOutPath $INSTDIR
  WriteIniStr "$INSTDIR\${PRODUCT_NAME}.url" "InternetShortcut" "URL" "${PRODUCT_WEB_SITE}"
  CreateShortCut "$SMPROGRAMS\Edyuk\Website.lnk" "$INSTDIR\${PRODUCT_NAME}.url"
  CreateShortCut "$SMPROGRAMS\Edyuk\Uninstall.lnk" "$INSTDIR\uninst.exe"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$INSTDIR\edyuk.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\edyuk.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Edyuk core files required to run"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "A set of default plugins meant to support C++/Qt4"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "Syntax files and basic configuration for the editor"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} "File and project templates"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC09} "Translations"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC05} "Edyuk sources"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC06} "Edyuk documentation"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC07} "Edyuk plugin development kit"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC08} "Registers file extensions"
  !insertmacro MUI_DESCRIPTION_TEXT ${S08S1} "Registers C++ headers/source to be opened inside Edyuk"
  !insertmacro MUI_DESCRIPTION_TEXT ${S08S2} "Registers Qt resources (.qrc) and qmake projects (.pro) to be opened inside Edyuk"
  !insertmacro MUI_DESCRIPTION_TEXT ${S08S3} "Registers QCodeEdit syntax files (.qnfa, .qxm, .qxf) to be opened inside Edyuk"
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) has been successfully uninstalled."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to remove $(^Name) and all its components ?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  RMDir /r "$INSTDIR"

  Delete "$SMPROGRAMS\Edyuk\Uninstall.lnk"
  Delete "$SMPROGRAMS\Edyuk\Website.lnk"
  Delete "$DESKTOP\Edyuk.lnk"
  Delete "$SMPROGRAMS\Edyuk\Edyuk.lnk"

  RMDir "$SMPROGRAMS\Edyuk"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd