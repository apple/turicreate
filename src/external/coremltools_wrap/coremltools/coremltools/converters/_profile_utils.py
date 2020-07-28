from __future__ import print_function as _
import os
import time

_FUNCTION_PROFILE_REGISTRY = {}  # str -> list (function name to time stack)
_ENABLE_PROFILING = os.environ.get("ENABLE_PROFILING", False)


def _profile(_f=None):
    def func_wrapper(func):
        f_name = func.__module__ + "." + func.__name__
        if f_name in _FUNCTION_PROFILE_REGISTRY:
            raise ValueError(
                "Function {} is already registered for profiling.".format(f_name)
            )

        _FUNCTION_PROFILE_REGISTRY[f_name] = []
        return func

    if _f is None:
        return func_wrapper
    return func_wrapper(_f)


_INITIAL_CALL = True


def _pr_color(skk, color="94m", end="\n"):
    print("\033[{} {}\033[00m".format(color, skk), end=end)


def _profiler(frame, event, arg, indent=[0]):
    if frame.f_globals.get("__name__", None) is None:
        return

    package_name = __name__.split(".")[0]

    function_name = frame.f_globals["__name__"] + "." + frame.f_code.co_name

    profile_function = (
        package_name in str(frame) and function_name in _FUNCTION_PROFILE_REGISTRY
    )

    if event == "call" and profile_function:
        global _INITIAL_CALL
        if _INITIAL_CALL:
            _INITIAL_CALL = False
            print("\n" * 2)

        indent[0] += 3
        _pr_color(
            "{} call {} {}".format(
                "=" * indent[0] + ">",
                function_name.split(".")[-1],
                " (" + ".".join(function_name.split(".")[2:-1]) + ")",
            )
        )
        start_time = time.clock()
        _FUNCTION_PROFILE_REGISTRY[function_name].append(start_time)

    elif event == "return" and profile_function:
        duration = time.clock() - _FUNCTION_PROFILE_REGISTRY[function_name][-1]
        duration = round(duration)
        _pr_color(
            "{} exit {} {} ".format(
                "<" + "=" * indent[0],
                function_name.split(".")[-1],
                " (" + ".".join(function_name.split(".")[2:-1]) + ")",
            ),
            end="",
        )
        _pr_color(": Time spent {} seconds ".format(duration,), color="91m")
        indent[0] -= 3
        _FUNCTION_PROFILE_REGISTRY[function_name].pop()

    return _profiler
