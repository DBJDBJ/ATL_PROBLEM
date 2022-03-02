/*
* (c) 2019-2022 by dbj@dbj.org
* https://dbj.org/license_dbj
*/
#include <assert.h>
#include <strsafe.h>
#include <exception>

#define WIN32_LEAN_AND_MEAN
#define STRICT 1

// #define USING_SIMPLE_ATL_ARR

#ifdef USING_SIMPLE_ATL_ARR
#include <atlsimpcoll.h>
#else
#include <comdef.h> // HRESULT to message
#include <atlcoll.h>
#endif

#pragma region cruft

#define APP_BUFFER_LEN 1024

#ifdef USING_SIMPLE_ATL_ARR
#define APP_NAME_ "DBJ_ATL_TEST using ATL::CSimpleArray "
#else
#define APP_NAME_ "DBJ_ATL_TEST using ATL::CAtlArray "
#endif

#ifdef _DEBUG
#define APP_NAME APP_NAME_ "DEBUG build"
#else
#define APP_NAME APP_NAME_ "RELEASE build"
#endif

/*-----------------------------------------------------------------
 * must use the result immediately
 */
__inline LPSTR last_error_(void) {
  LPSTR lpMsgBuf = 0;
  DWORD dw = GetLastError();

  FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPSTR)&lpMsgBuf, 0, NULL);

  static CHAR lpDisplayBuf[APP_BUFFER_LEN] = {0};

  (void)StringCchPrintfA(lpDisplayBuf, APP_BUFFER_LEN, "Last error (%d): %s",
                         dw, lpMsgBuf);

  LocalFree(lpMsgBuf);
  return lpDisplayBuf;
}
// print with last error message added
__inline void PRINT(const char* format_, ...) {
  char buffy[APP_BUFFER_LEN] = {0};  // magical number; not exemplary code
  va_list args = {0};
  va_start(args, format_);
  int nBuf = _vsnprintf_s(buffy, APP_BUFFER_LEN, APP_BUFFER_LEN, format_, args);
  if (nBuf < 1) {
    OutputDebugStringA(__FILE__ " : buffer overflow\n");
    (void)MessageBoxA(NULL, (__FILE__ " : buffer overflow\n"), "Error", MB_OK);
    va_end(args);
    return;
  }
  va_end(args);

  // add the last error full message
  HRESULT res_ =
      StringCchPrintfA(buffy, APP_BUFFER_LEN, "%s\n\n%s", buffy, last_error_());

  if (res_ != S_OK) {
    OutputDebugStringA(__FILE__ " : buffer overflow\n");
    (void)MessageBoxA(NULL, (__FILE__ " : buffer overflow\n"), "Error", MB_OK);
    return;
  }
  OutputDebugStringA(buffy);
  (void)MessageBoxA(NULL, (LPCSTR)buffy, APP_NAME, MB_OK);
}
#pragma endregion  // cruft

/*-----------------------------------------------------------------
 */
static void app_run_ (void)
{
  try {
#ifdef USING_SIMPLE_ATL_ARR
    ATL::CSimpleArray<int> int_arr;
#else
    ATL::CAtlArray<int> int_arr;
#endif

    int_arr.Add(100);  // 0
    int_arr.Add(200);  // 1
    int_arr.Add(300);  // 2

    PRINT("\n 3-rd element: %d\n", int_arr[2]);  // OK

    /*
    The issue is in RELEASE builds CSimpleArray and CAtlArray behave fundamentaly differently
    due to the error handling difference.

    The difference is in ATL::CSimpleArray operator []

    T& operator[] (_In_ int nIndex)
        {
                ATLASSERT(nIndex >= 0 && nIndex < m_nSize);
                if(nIndex < 0 || nIndex >= m_nSize)
                {
                        _AtlRaiseException((DWORD)EXCEPTION_ARRAY_BOUNDS_EXCEEDED);
// core of the difference is standard not simple atl array
// uses here: AtlThrow(E_INVALIDARG);
// instead of _AtlRaiseException()
// thus it throws CAtlException, 
// insted of raising Structured Exception
// thus behaving fundamentaly differently from a simple array
                }
                return m_aT[nIndex];
        }
    */
    PRINT("\n 4-th element: ", int_arr[3]);

  } catch (std::exception& x) {
    PRINT("\nException: ", x.what(), "\n");
  }
#ifndef USING_SIMPLE_ATL_ARR
  // using  ATL::CSimpleArray this exception type is not even defined
  catch (CAtlException& atlx_) {
    // in RELEASE builds and using ATL::CAtlArray
    // we land here; as expected
    _com_error err((HRESULT)atlx_);
    LPCTSTR err_msg_ = err.ErrorMessage(); // WCHAR * actually
    PRINT("\nCAtlException with message from HRESULT:%S\n", err_msg_ );
  }
#endif
  catch (...) {
    PRINT("\nUnknown Exception\n");
  }
}

/*
error C2713: Only one form of exception handling permitted per function
error C2712: Cannot use __try in functions that require object unwinding
*/
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow) {
  UNREFERENCED_PARAMETER(hInstance);
  UNREFERENCED_PARAMETER(hPrevInstance);
  UNREFERENCED_PARAMETER(lpCmdLine);
  UNREFERENCED_PARAMETER(nCmdShow);

  __try {
    app_run_();
  } // eof __try
  __except (EXCEPTION_EXECUTE_HANDLER) {
    PRINT("We are here because ATL::CSimpleArray operator[] has raised SE exception.");
  }

  return EXIT_SUCCESS;
}
