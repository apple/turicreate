class ProgressTablePrinter(object):
    def __init__(self, column_names, column_display_names):
        """
        column_names : list(str)
             Keyword args passed to update(..)

        column_display_names : list(str)
             Names with are displayed in the header of the table

        The ordering of column_names and column_display_names must match.
        """
        assert len(column_names) == len(column_display_names)
        num_columns = len(column_names)

        self.column_names = column_names
        self.column_width = max(map(lambda x: len(x), column_display_names)) + 2
        self.hr = "+" + "+".join(["-" * self.column_width] * num_columns) + "+"

        # Print progress table header
        print(self.hr)
        print(
            ("| {:<{width}}" * num_columns + "|").format(
                *column_display_names, width=self.column_width - 1
            )
        )
        print(self.hr)

    def print_row(self, **kwargs):
        """
        keys of kwargs must be the names passed to __init__(...) as `column_names`
        """
        meta_string = "|"
        for key in self.column_names:
            float_specifier = ""
            if isinstance(kwargs[key], float):
                float_specifier = ".3f"
            meta_string += " {%s:<{width}%s}|" % (key, float_specifier)
        kwargs["width"] = self.column_width - 1

        print(meta_string.format(**kwargs))
        print(self.hr)
