CPack IFW Generator
-------------------

See :module:`CPackIFW` for details on the CPackIFW module.

.. _QtIFW: http://doc.qt.io/qtinstallerframework/index.html


Overview
^^^^^^^^

CPack ``IFW`` generator helps you to create online and offline
binary cross-platform installers with a graphical user interface.

CPack IFW generator prepares project installation and generates configuration
and meta information for QtIFW_ tools.

The QtIFW_ provides a set of tools and utilities to create
installers for the supported desktop Qt platforms: Linux, Microsoft Windows,
and macOS.

You should also install QtIFW_ to use CPack ``IFW`` generator.

Hints
^^^^^

Generally, the CPack ``IFW`` generator automatically finds QtIFW_ tools,
but if you don't use a default path for installation of the QtIFW_ tools,
the path may be specified in either a CMake or an environment variable:

.. variable:: CPACK_IFW_ROOT

 An CMake variable which specifies the location of the QtIFW_ tool suite.

 The variable will be cached in the ``CPackConfig.cmake`` file and used at
 CPack runtime.

.. variable:: QTIFWDIR

 An environment variable which specifies the location of the QtIFW_ tool
 suite.

.. note::
  The specified path should not contain "bin" at the end
  (for example: "D:\\DevTools\\QtIFW2.0.5").

The :variable:`CPACK_IFW_ROOT` variable has a higher priority and overrides
the value of the :variable:`QTIFWDIR` variable.

Internationalization
^^^^^^^^^^^^^^^^^^^^

Some variables and command arguments support internationalization via
CMake script. This is an optional feature.

Installers created by QtIFW_ tools have built-in support for
internationalization and many phrases are localized to many languages,
but this does not apply to the description of the your components and groups
that will be distributed.

Localization of the description of your components and groups is useful for
users of your installers.

A localized variable or argument can contain a single default value, and a
set of pairs the name of the locale and the localized value.

For example:

.. code-block:: cmake

   set(LOCALIZABLE_VARIABLE "Default value"
     en "English value"
     en_US "American value"
     en_GB "Great Britain value"
     )

Variables
^^^^^^^^^

You can use the following variables to change behavior of CPack ``IFW``
generator.

Debug
"""""

.. variable:: CPACK_IFW_VERBOSE

 Set to ``ON`` to enable addition debug output.
 By default is ``OFF``.

Package
"""""""

.. variable:: CPACK_IFW_PACKAGE_TITLE

 Name of the installer as displayed on the title bar.
 By default used :variable:`CPACK_PACKAGE_DESCRIPTION_SUMMARY`.

.. variable:: CPACK_IFW_PACKAGE_PUBLISHER

 Publisher of the software (as shown in the Windows Control Panel).
 By default used :variable:`CPACK_PACKAGE_VENDOR`.

.. variable:: CPACK_IFW_PRODUCT_URL

 URL to a page that contains product information on your web site.

.. variable:: CPACK_IFW_PACKAGE_ICON

 Filename for a custom installer icon. The actual file is '.icns' (macOS),
 '.ico' (Windows). No functionality on Unix.

.. variable:: CPACK_IFW_PACKAGE_WINDOW_ICON

 Filename for a custom window icon in PNG format for the Installer
 application.

.. variable:: CPACK_IFW_PACKAGE_LOGO

 Filename for a logo is used as QWizard::LogoPixmap.

.. variable:: CPACK_IFW_PACKAGE_WATERMARK

 Filename for a watermark is used as QWizard::WatermarkPixmap.

.. variable:: CPACK_IFW_PACKAGE_BANNER

 Filename for a banner is used as QWizard::BannerPixmap.

.. variable:: CPACK_IFW_PACKAGE_BACKGROUND

 Filename for an image used as QWizard::BackgroundPixmap (only used by MacStyle).

.. variable:: CPACK_IFW_PACKAGE_WIZARD_STYLE

 Wizard style to be used ("Modern", "Mac", "Aero" or "Classic").

.. variable:: CPACK_IFW_PACKAGE_WIZARD_DEFAULT_WIDTH

 Default width of the wizard in pixels. Setting a banner image will override this.

.. variable:: CPACK_IFW_PACKAGE_WIZARD_DEFAULT_HEIGHT

 Default height of the wizard in pixels. Setting a watermark image will override this.

.. variable:: CPACK_IFW_PACKAGE_TITLE_COLOR

 Color of the titles and subtitles (takes an HTML color code, such as "#88FF33").

.. variable:: CPACK_IFW_PACKAGE_START_MENU_DIRECTORY

 Name of the default program group for the product in the Windows Start menu.

 By default used :variable:`CPACK_IFW_PACKAGE_NAME`.

.. variable:: CPACK_IFW_TARGET_DIRECTORY

 Default target directory for installation.
 By default used
 "@ApplicationsDir@/:variable:`CPACK_PACKAGE_INSTALL_DIRECTORY`"

 You can use predefined variables.

.. variable:: CPACK_IFW_ADMIN_TARGET_DIRECTORY

 Default target directory for installation with administrator rights.

 You can use predefined variables.

.. variable:: CPACK_IFW_PACKAGE_GROUP

 The group, which will be used to configure the root package

.. variable:: CPACK_IFW_PACKAGE_NAME

 The root package name, which will be used if configuration group is not
 specified

.. variable:: CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_NAME

 Filename of the generated maintenance tool.
 The platform-specific executable file extension is appended.

 By default used QtIFW_ defaults (``maintenancetool``).

.. variable:: CPACK_IFW_PACKAGE_REMOVE_TARGET_DIR

 Set to ``OFF`` if the target directory should not be deleted when uninstalling.

 Is ``ON`` by default

.. variable:: CPACK_IFW_PACKAGE_MAINTENANCE_TOOL_INI_FILE

 Filename for the configuration of the generated maintenance tool.

 By default used QtIFW_ defaults (``maintenancetool.ini``).

.. variable:: CPACK_IFW_PACKAGE_ALLOW_NON_ASCII_CHARACTERS

 Set to ``ON`` if the installation path can contain non-ASCII characters.

 Is ``ON`` for QtIFW_ less 2.0 tools.

.. variable:: CPACK_IFW_PACKAGE_ALLOW_SPACE_IN_PATH

 Set to ``OFF`` if the installation path cannot contain space characters.

 Is ``ON`` for QtIFW_ less 2.0 tools.

.. variable:: CPACK_IFW_PACKAGE_CONTROL_SCRIPT

 Filename for a custom installer control script.

.. variable:: CPACK_IFW_PACKAGE_RESOURCES

 List of additional resources ('.qrc' files) to include in the installer
 binary.

 You can use :command:`cpack_ifw_add_package_resources` command to resolve
 relative paths.

.. variable:: CPACK_IFW_PACKAGE_FILE_EXTENSION

 The target binary extension.

 On Linux, the name of the target binary is automatically extended with
 '.run', if you do not specify the extension.

 On Windows, the target is created as an application with the extension
 '.exe', which is automatically added, if not supplied.

 On Mac, the target is created as an DMG disk image with the extension
 '.dmg', which is automatically added, if not supplied.

.. variable:: CPACK_IFW_REPOSITORIES_ALL

 The list of remote repositories.

 The default value of this variable is computed by CPack and contains
 all repositories added with command :command:`cpack_ifw_add_repository`
 or updated with command :command:`cpack_ifw_update_repository`.

.. variable:: CPACK_IFW_DOWNLOAD_ALL

 If this is ``ON`` all components will be downloaded.
 By default is ``OFF`` or used value
 from ``CPACK_DOWNLOAD_ALL`` if set

Components
""""""""""

.. variable:: CPACK_IFW_RESOLVE_DUPLICATE_NAMES

 Resolve duplicate names when installing components with groups.

.. variable:: CPACK_IFW_PACKAGES_DIRECTORIES

 Additional prepared packages dirs that will be used to resolve
 dependent components.

.. variable:: CPACK_IFW_REPOSITORIES_DIRECTORIES

 Additional prepared repository dirs that will be used to resolve and
 repack dependent components. This feature available only
 since QtIFW_ 3.1.

Tools
"""""

.. variable:: CPACK_IFW_FRAMEWORK_VERSION

 The version of used QtIFW_ tools.

.. variable:: CPACK_IFW_BINARYCREATOR_EXECUTABLE

 The path to "binarycreator" command line client.

 This variable is cached and may be configured if needed.

.. variable:: CPACK_IFW_REPOGEN_EXECUTABLE

 The path to "repogen" command line client.

 This variable is cached and may be configured if needed.

.. variable:: CPACK_IFW_INSTALLERBASE_EXECUTABLE

 The path to "installerbase" installer executable base.

 This variable is cached and may be configured if needed.

.. variable:: CPACK_IFW_DEVTOOL_EXECUTABLE

 The path to "devtool" command line client.

 This variable is cached and may be configured if needed.


Online installer
^^^^^^^^^^^^^^^^

By default CPack IFW generator makes offline installer. This means that all
components will be packaged into a binary file.

To make a component downloaded, you must set the ``DOWNLOADED`` option in
:command:`cpack_add_component`.

Then you would use the command :command:`cpack_configure_downloads`.
If you set ``ALL`` option all components will be downloaded.

You also can use command :command:`cpack_ifw_add_repository` and
variable :variable:`CPACK_IFW_DOWNLOAD_ALL` for more specific configuration.

CPack IFW generator creates "repository" dir in current binary dir. You
would copy content of this dir to specified ``site`` (``url``).

See Also
^^^^^^^^

Qt Installer Framework Manual:

* Index page:
  http://doc.qt.io/qtinstallerframework/index.html

* Component Scripting:
  http://doc.qt.io/qtinstallerframework/scripting.html

* Predefined Variables:
  http://doc.qt.io/qtinstallerframework/scripting.html#predefined-variables

* Promoting Updates:
  http://doc.qt.io/qtinstallerframework/ifw-updates.html

Download Qt Installer Framework for you platform from Qt site:
 http://download.qt.io/official_releases/qt-installer-framework
