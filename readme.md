
## This is created by me, in reaction to: https://developercommunity.visualstudio.com/t/atl-csimplearray-and-csimplemap/1246581

## How to use

Code is short and simple.
```cpp
#define USING_SIMPLE_ATL_ARR
```
Above, defined or not makes code use `ATL::CSimpleArray<T>` or `ATL::CAtlArray<T>`.

Please run in both modes to see the issue described bellow.

### The problem is apparent in RELEASE builds 

`ATL::CSimpleArray<T>` and `ATL::CAtlArray<T>` are behaving wildly differently due to fundamentally different error handling (in RELEASE builds).

Ditto, in RELEASE builds simple atl array uses [SEH](https://docs.microsoft.com/en-us/windows/win32/debug/about-structured-exception-handling) and "normal" array does not. That is regardless of  `_ATL_NO_EXCEPTIONS` being defined or not.

## Why do we care?

In RELEASE builds and using `ATL::CSimpleArray<T>` and using bad index for the `[]` operator application will simply exit if programmer is unaware of SEH. As many are.

`ATL::CSimpleMap<K,V>` has the same problem.

Is this by design? If it is, I could not find it documented.

## Remedy

For starters one should always have any kind of her few WIN32 main's simply checking for SEH being thrown. Like we do in here, as an example:

```cpp
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
    // in there use normal try/throw/catch
    app_run_();
  } // in here catch only SE aka Structured Exceptions
  __except (EXCEPTION_EXECUTE_HANDLER) {
    PRINT("We are here because something has raised an SE exception.");
  }

  return EXIT_SUCCESS;
}
```
One should also create a [minidump](https://docs.microsoft.com/en-us/windows/win32/debug/minidump-files) from inside that SEH `__except` handler block above.

### ATL code change

Ultimately to make simple array and simple map in ATL, not raise Structured Exceptions, and behave like their standard twins, seems like quite a trivial code change. There might be other ATL code with the same or similar issue. I do not know.

NOTE: if `_ATL_NO_EXCEPTIONS` is used there is no CPP unwinding. In that case and in RELEASE builds, SEH will/should be always used.

In DEBUG builds, wrong/bad code always asserts, using standard [UCRT](https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/assert-asserte-assert-expr-macros?view=msvc-170) `_ASSERTE` message box. Called from [`ATLASSERT`](https://docs.microsoft.com/en-us/cpp/atl/reference/debugging-and-error-reporting-macros?view=msvc-170)

---

* (c) 2019-2022 by dbj@dbj.org
* https://dbj.org/license_dbj
