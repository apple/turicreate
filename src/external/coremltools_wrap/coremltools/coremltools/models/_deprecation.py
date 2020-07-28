import warnings
import functools


def deprecated(obj=None, suffix=""):
    """
    Decorator to mark a function or a class as deprecated
    """

    def decorator_deprecation_warning(obj):
        @functools.wraps(obj)
        def wrapped(*args, **kwargs):
            if isinstance(obj, type):
                msg = (
                    'Class "%s" is deprecated and will be removed in the next release'
                    % obj.__name__
                )
            else:
                msg = (
                    'Function "%s" is deprecated and will be removed in the next release'
                    % obj.__name__
                )
            if suffix:
                msg += "; %s" % suffix
            warnings.warn(msg, category=FutureWarning)
            return obj(*args, **kwargs)

        return wrapped

    if obj is None:
        return decorator_deprecation_warning

    return decorator_deprecation_warning(obj)
