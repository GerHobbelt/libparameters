# `libparameters` Design Specification

The library must provide a C++ API which enables programmers to easily create/instantiate *variables* (a.k.a. *parameters*) which will be used to configure/parameterize the application & the algorithms used there-in.[^1]

[^1]: one example of such a *parameterized application* is our `tesseract` fork, where all user-accessible *application parameters* have been migrated from using Google GFLAGS to `libparamaters`-based parameters.



`libparameters`' API & parameter classes (templates) are designed to serve these important goals:

1. "***talk like a native***": *parameter instances* must act as much like their primitive C++ base types (`bool`, `integer`, `double`, `std::string`, ...) as possible: they SHOULD be directly invocable in C++ calculus expressions & statements as if they are these base types. This ensures their usage throughout the application is simple and *unremarkable*. 
2. "***fast like a native***": accessing these parameter instances SHOULD be *fast* for both *read* and *write* actions: the intent is that these parameters are used throughout the application, pervasively and as if their type is a *native primitive* (bool, int, double, string).

   >  --> this requirement precludes using *dictionaries* with (string = text-based) name lookup,
   >  as you often see in other configuration libraries out there!
   
3. "***not just for the basic types***": we SHOULD also support *parameter instances* of more complex type, such as *sets*, e.g. *std::vector* or other set/collection types.

   > For example, a single application/algorithm *parameter* might be a *vector of integers* (`std::vector<int>`).
   
4. "***complete r/w diagnostics for usage analysis***": each parameter's use (read, write, change, reset access) is tracked by the library code so we can always obtain precise reporting about the *actual usage* of *each* of the parameters in our application / algorithm: did we actually *use/need* this parameter during these runs, did our software internally *edit/adjust* any of our parameters during the run? how often did our software fetch (read) our parameters' values for use? etcetera...

## What does our API provide per each parameter instance?

- C++ class template based for easy production of suitable types & instances.
- optional userland callbacks which will be invoked for the various access events: 
  - read value
  - write value
  - modify value
  - reset value to (configurable) default
  - parse value from text string (e.g. when processing human data entry / config file decoding)
  - validate value to be written (so one can provide bespoke range and other sanity checks for a parameter)
- track usage:
  - read count
  - write count
  - modify count (number of times a write actually was a *value change*)
  - reset-to-default count (because $\text{count}_\text{write} - \text{count}_\text{modify} \neq \text{count}_\text{reset}$ as a *modify* action is a *change compared to the previously known value, **not** compared to the default value per sé*!)

Each parameter is registered with a Manager instance; if none is provided, a *Global Manager* singleton is used.

The function of the Manager instance is providing services for *parameter sets*:
- provide a list of registered (i.e. *available*) parameters
- provide aid when adding these parameters to your application's CLI (Command Line Interface), so they are easily configurable by name and configfile or similar text-based means
- produce usage/diagnostics reports: which parameters have actually been *used* and in *what way*? Which of our parameters are most important for our current process?



## See also

- [[Design Notes]]
