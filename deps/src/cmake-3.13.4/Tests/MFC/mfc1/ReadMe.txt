================================================================================
    MICROSOFT FOUNDATION CLASS LIBRARY : mfc1 Project Overview
===============================================================================

The application wizard has created this mfc1 application for
you.  This application not only demonstrates the basics of using the Microsoft
Foundation Classes but is also a starting point for writing your application.

This file contains a summary of what you will find in each of the files that
make up your mfc1 application.

mfc1.vcproj
    This is the main project file for VC++ projects generated using an application wizard.
    It contains information about the version of Visual C++ that generated the file, and
    information about the platforms, configurations, and project features selected with the
    application wizard.

mfc1.h
    This is the main header file for the application.  It includes other
    project specific headers (including Resource.h) and declares the
    Cmfc1App application class.

mfc1.cpp
    This is the main application source file that contains the application
    class Cmfc1App.

mfc1.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
    Visual C++. Your project resources are in 1033.

res\mfc1.ico
    This is an icon file, which is used as the application's icon.  This
    icon is included by the main resource file mfc1.rc.

res\mfc1.rc2
    This file contains resources that are not edited by Microsoft
    Visual C++. You should place all resources not editable by
    the resource editor in this file.
mfc1.reg
    This is an example .reg file that shows you the kind of registration
    settings the framework will set for you.  You can use this as a .reg
    file to go along with your application or just delete it and rely
    on the default RegisterShellFileTypes registration.
/////////////////////////////////////////////////////////////////////////////

For the main frame window:
    The project includes a standard MFC interface.
MainFrm.h, MainFrm.cpp
    These files contain the frame class CMainFrame, which is derived from
    CMDIFrameWnd and controls all MDI frame features.
res\Toolbar.bmp
    This bitmap file is used to create tiled images for the toolbar.
    The initial toolbar and status bar are constructed in the CMainFrame
    class. Edit this toolbar bitmap using the resource editor, and
    update the IDR_MAINFRAME TOOLBAR array in mfc1.rc to add
    toolbar buttons.
/////////////////////////////////////////////////////////////////////////////

For the child frame window:

ChildFrm.h, ChildFrm.cpp
    These files define and implement the CChildFrame class, which
    supports the child windows in an MDI application.

/////////////////////////////////////////////////////////////////////////////

The application wizard creates one document type and one view:

mfc1Doc.h, mfc1Doc.cpp - the document
    These files contain your Cmfc1Doc class.  Edit these files to
    add your special document data and to implement file saving and loading
    (via Cmfc1Doc::Serialize).
    The Document will have the following strings:
        File extension:      mf1
        File type ID:        mfc1.Document
        Main frame caption:  mfc1
        Doc type name:       mfc1
        Filter name:         mfc1 Files (*.mf1)
        File new short name: mfc1
        File type long name: mfc1.Document
mfc1View.h, mfc1View.cpp - the view of the document
    These files contain your Cmfc1View class.
    Cmfc1View objects are used to view Cmfc1Doc objects.
res\mfc1Doc.ico
    This is an icon file, which is used as the icon for MDI child windows
    for the Cmfc1Doc class.  This icon is included by the main
    resource file mfc1.rc.
/////////////////////////////////////////////////////////////////////////////

Other Features:

ActiveX Controls
    The application includes support to use ActiveX controls.

Printing and Print Preview support
    The application wizard has generated code to handle the print, print setup, and print preview
    commands by calling member functions in the CView class from the MFC library.
/////////////////////////////////////////////////////////////////////////////

Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named mfc1.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

mfc1.manifest
	Application manifest files are used by Windows XP to describe an applications
	dependency on specific versions of Side-by-Side assemblies. The loader uses this
	information to load the appropriate assembly from the assembly cache or private
	from the application. The Application manifest  maybe included for redistribution
	as an external .manifest file that is installed in the same folder as the application
	executable or it may be included in the executable in the form of a resource.
/////////////////////////////////////////////////////////////////////////////

Other notes:

The application wizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

If your application uses MFC in a shared DLL, and your application is in a
language other than the operating system's current language, you will need
to copy the corresponding localized resources MFC70XXX.DLL from the Microsoft
Visual C++ CD-ROM under the Win\System directory to your computer's system or
system32 directory, and rename it to be MFCLOC.DLL.  ("XXX" stands for the
language abbreviation.  For example, MFC70DEU.DLL contains resources
translated to German.)  If you don't do this, some of the UI elements of
your application will remain in the language of the operating system.

/////////////////////////////////////////////////////////////////////////////
