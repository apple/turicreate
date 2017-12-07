# Microsoft Developer Studio Project File - Name="libcurl" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102
# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libcurl - Win32 LIB Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "vc6libcurl.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "vc6libcurl.mak" CFG="libcurl - Win32 LIB Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libcurl - Win32 DLL Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libcurl - Win32 DLL Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libcurl - Win32 LIB Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "libcurl - Win32 LIB Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""

!IF  "$(CFG)" == "libcurl - Win32 DLL Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "dll-debug"
# PROP BASE Intermediate_Dir "dll-debug/obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "dll-debug"
# PROP Intermediate_Dir "dll-debug/obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Zi /Od /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "BUILDING_LIBCURL" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /EHsc /Zi /Od /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "BUILDING_LIBCURL" /FD /GZ /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wldap32.lib ws2_32.lib advapi32.lib kernel32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"dll-debug/libcurld.dll" /implib:"dll-debug/libcurld_imp.lib" /pdbtype:con /fixed:no
# ADD LINK32 wldap32.lib ws2_32.lib advapi32.lib kernel32.lib /nologo /dll /incremental:no /debug /machine:I386 /out:"dll-debug/libcurld.dll" /implib:"dll-debug/libcurld_imp.lib" /pdbtype:con /fixed:no

!ELSEIF  "$(CFG)" == "libcurl - Win32 DLL Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "dll-release"
# PROP BASE Intermediate_Dir "dll-release/obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "dll-release"
# PROP Intermediate_Dir "dll-release/obj"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "BUILDING_LIBCURL" /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "BUILDING_LIBCURL" /FD /c
MTL=midl.exe
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 wldap32.lib ws2_32.lib advapi32.lib kernel32.lib /nologo /dll /pdb:none /machine:I386 /out:"dll-release/libcurl.dll" /implib:"dll-release/libcurl_imp.lib" /fixed:no /release /incremental:no
# ADD LINK32 wldap32.lib ws2_32.lib advapi32.lib kernel32.lib /nologo /dll /pdb:none /machine:I386 /out:"dll-release/libcurl.dll" /implib:"dll-release/libcurl_imp.lib" /fixed:no /release /incremental:no

!ELSEIF  "$(CFG)" == "libcurl - Win32 LIB Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "lib-debug"
# PROP BASE Intermediate_Dir "lib-debug/obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "lib-debug"
# PROP Intermediate_Dir "lib-debug/obj"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MDd /W3 /EHsc /Zi /Od /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "BUILDING_LIBCURL" /D "CURL_STATICLIB" /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /EHsc /Zi /Od /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "_DEBUG" /D "BUILDING_LIBCURL" /D "CURL_STATICLIB" /FD /GZ /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib-debug/libcurld.lib" /machine:I386
# ADD LIB32 /nologo /out:"lib-debug/libcurld.lib" /machine:I386

!ELSEIF  "$(CFG)" == "libcurl - Win32 LIB Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "lib-release"
# PROP BASE Intermediate_Dir "lib-release/obj"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "lib-release"
# PROP Intermediate_Dir "lib-release/obj"
# PROP Target_Dir ""
CPP=cl.exe
# ADD BASE CPP /nologo /MD /W3 /EHsc /O2 /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "BUILDING_LIBCURL" /D "CURL_STATICLIB" /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /O2 /I "..\..\..\lib" /I "..\..\..\include" /D "WIN32" /D "NDEBUG" /D "BUILDING_LIBCURL" /D "CURL_STATICLIB" /FD /c
RSC=rc.exe
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo /out:"lib-release/libcurl.lib" /machine:I386
# ADD LIB32 /nologo /out:"lib-release/libcurl.lib" /machine:I386

!ENDIF 

# Begin Target

# Name "libcurl - Win32 DLL Debug"
# Name "libcurl - Win32 DLL Release"
# Name "libcurl - Win32 LIB Debug"
# Name "libcurl - Win32 LIB Release"
# Begin Group "Source Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\lib\amigaos.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\asyn-ares.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\asyn-thread.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\axtls.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\base64.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\bundles.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\conncache.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\connect.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\content_encoding.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\cookie.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_addrinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_darwinssl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_fnmatch.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_gethostname.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_gssapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_memrchr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_multibyte.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm_core.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm_msgs.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm_wb.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_rtmp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_sasl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_schannel.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_sspi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_threads.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\cyassl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\dict.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\dotdot.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\easy.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\escape.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\file.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\fileinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\formdata.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ftp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ftplistparser.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\getenv.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\getinfo.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\gopher.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\gskit.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\gtls.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hash.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hmac.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostasyn.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostcheck.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostip4.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostip6.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostsyn.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http2.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_chunks.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_digest.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_negotiate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_negotiate_sspi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_proxy.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\idn_win32.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\if2ip.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\imap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\inet_ntop.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\inet_pton.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\krb5.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ldap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\llist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\md4.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\md5.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\memdebug.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\mprintf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\multi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\netrc.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\non-ascii.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\nonblock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\nss.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\openldap.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\parsedate.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\pingpong.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\pipeline.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\polarssl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\polarssl_threadlock.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\pop3.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\progress.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\qssl.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\rawstr.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\rtsp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\security.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\select.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\sendf.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\share.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\slist.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\smtp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\socks.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\socks_gssapi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\socks_sspi.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\speedcheck.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\splay.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ssh.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\sslgen.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ssluse.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strdup.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strequal.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strerror.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strtok.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strtoofft.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\telnet.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\tftp.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\timeval.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\transfer.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\url.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\version.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\warnless.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\wildcard.c
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\x509asn1.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\lib\amigaos.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\arpa_telnet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\asyn.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\axtls.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\bundles.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\config-win32.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\conncache.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\connect.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\content_encoding.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\cookie.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_addrinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_base64.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_darwinssl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_fnmatch.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_gethostname.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_gssapi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_hmac.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ldap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_md4.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_md5.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_memory.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_memrchr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_multibyte.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm_core.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm_msgs.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_ntlm_wb.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_rtmp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_sasl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_schannel.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_sec.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_setup.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_setup_once.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_sspi.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curl_threads.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\curlx.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\cyassl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\dict.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\dotdot.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\easyif.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\escape.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\file.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\fileinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\formdata.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ftp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ftplistparser.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\getinfo.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\gopher.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\gskit.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\gtls.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hash.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostcheck.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\hostip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http2.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_chunks.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_digest.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_negotiate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\http_proxy.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\if2ip.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\imap.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\inet_ntop.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\inet_pton.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\llist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\memdebug.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\multihandle.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\multiif.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\netrc.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\non-ascii.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\nonblock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\nssg.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\parsedate.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\pingpong.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\pipeline.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\polarssl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\polarssl_threadlock.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\pop3.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\progress.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\qssl.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\rawstr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\rtsp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\select.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\sendf.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\setup-vms.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\share.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\slist.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\smtp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\sockaddr.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\socks.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\speedcheck.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\splay.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ssh.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\sslgen.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\ssluse.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strdup.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strequal.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strerror.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strtok.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\strtoofft.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\telnet.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\tftp.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\timeval.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\transfer.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\urldata.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\url.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\warnless.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\wildcard.h
# End Source File
# Begin Source File

SOURCE=..\..\..\lib\x509asn1.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter ""
# Begin Source File

SOURCE=..\..\..\lib\libcurl.rc
# End Source File
# End Group
# End Target
# End Project
