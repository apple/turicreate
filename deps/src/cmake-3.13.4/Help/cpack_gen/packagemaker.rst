CPack PackageMaker Generator
----------------------------

PackageMaker CPack generator (macOS).

Variables specific to CPack PackageMaker generator
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The following variable is specific to installers built on Mac
macOS using PackageMaker:

.. variable:: CPACK_OSX_PACKAGE_VERSION

 The version of macOS that the resulting PackageMaker archive should be
 compatible with. Different versions of macOS support different
 features. For example, CPack can only build component-based installers for
 macOS 10.4 or newer, and can only build installers that download
 component son-the-fly for macOS 10.5 or newer. If left blank, this value
 will be set to the minimum version of macOS that supports the requested
 features. Set this variable to some value (e.g., 10.4) only if you want to
 guarantee that your installer will work on that version of macOS, and
 don't mind missing extra features available in the installer shipping with
 later versions of macOS.
