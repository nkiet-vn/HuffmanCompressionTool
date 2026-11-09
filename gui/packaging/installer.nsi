Unicode True
!ifndef STAGE
!error "Pass /DSTAGE=<absolute directory>"
!endif
!ifndef OUTPUT
!define OUTPUT "TypeFoundry-Setup.exe"
!endif
Name "The Type Foundry"
OutFile "${OUTPUT}"
InstallDir "$PROGRAMFILES64\The Type Foundry"
RequestExecutionLevel admin
ShowInstDetails show
Page directory
Page instfiles
Section "Application" MainInstall
  SetOutPath "$INSTDIR"
  File /r "${STAGE}\*"
  CreateDirectory "$SMPROGRAMS\The Type Foundry"
  CreateShortCut "$SMPROGRAMS\The Type Foundry\The Type Foundry.lnk" "$INSTDIR\huffman_gui.exe"
  CreateShortCut "$DESKTOP\The Type Foundry.lnk" "$INSTDIR\huffman_gui.exe"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheTypeFoundry" "DisplayName" "The Type Foundry"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheTypeFoundry" "UninstallString" '"$INSTDIR\Uninstall.exe"'
SectionEnd
Section "Uninstall"
  Delete "$DESKTOP\The Type Foundry.lnk"
  RMDir /r "$SMPROGRAMS\The Type Foundry"
  RMDir /r "$INSTDIR"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TheTypeFoundry"
SectionEnd
