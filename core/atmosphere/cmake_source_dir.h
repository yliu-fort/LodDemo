//COPYONLY
//    Copy the file without replacing any variable references or other content. This option may not be used with NEWLINE_STYLE.

//ESCAPE_QUOTES
//    Escape any substituted quotes with backslashes (C-style).
//    set(FOO_STRING "\"foo\"")

//@ONLY
//    Restrict variable replacement to references of the form . This is useful for configuring scripts that use ${VAR} syntax.
    
//NEWLINE_STYLE <style>
//    Specify the newline style for the output file. Specify UNIX or LF for \n newlines, or specify DOS, WIN32, or CRLF for \r\n newlines. This option may not be used with COPYONLY.
    

//Example

//Consider a source tree containing a foo.h.in file:
/* #undef FOO_ENABLE */
/* #undef FOO_STRING */
//An adjacent CMakeLists.txt may use configure_file to configure the header:
//option(FOO_ENABLE "Enable Foo" ON)
//if(FOO_ENABLE)
//  set(FOO_STRING "foo")
//endif()
//configure_file(foo.h.in foo.h @ONLY)
//This creates a foo.h in the build directory corresponding to this source directory. If the FOO_ENABLE option is on, the configured file will contain:
//#define FOO_ENABLE
//#define FOO_STRING "foo"
//Otherwise it will contain:
///* #undef FOO_ENABLE */
///* #undef FOO_STRING */
//One may then use the include_directories() command to specify the output directory as an include directory:
//include_directories(${CMAKE_CURRENT_BINARY_DIR})
//so that sources may include the header as #include <foo.h>.

#define SRC_PATH "C:/Users/LIFTER-XX/Documents/GitHub/LodDemo/core/atmosphere/"

#define INFILE(...) (SRC_PATH#__VA_ARGS__) // will deprecated

#define FP(ARG) (std::string(SRC_PATH).append(ARG).c_str()) // need string support
