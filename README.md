# libparameters

Use C++ runtime configurable typed parameters as if they were native types, while you can track read/write access/usage of each in your code sections for improved diagnostics of your application's behaviour. Used in enhanced tesseract, f.e.

## Features:

- coding: set/get (assignment, test, ...) is very simple and looks exactly like when you'ld be using the underlying native types, thanks to C++ operator overloading.
- strongly typed: all major native types are supported: integer, floating point, boolean, string.
- tracking/statistics/diagnostics: tracks r/w access, i.e. you can simply observe when and how a parameter instance is used in your (complex) application.
- init/config: parameter instances can be set up easily from command line, config file or any other source.
- init/config: parameter instances are config parser agnostic.
- tracking/statistics/diagnostics: easy additional access methods are available when you don't the tracking statistics to be impacted by these config setup/verify/store setup code sections. 
- scoped: parameter instances can be per-instance, i.e. be part of a application object instance.
- scoped: alternatively, parameter instances can be declared as globals, i.e. one setting for all parts in your application.
- bounds/relations/advanced customization: application-specific filter, validation and parsing callbacks can be specified per parameter instance for enhanced, custom behaviour.


