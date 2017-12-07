# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

import argparse
import codecs
import copy
import logging
import json
import os

from collections import OrderedDict
from xml.dom.minidom import parse, parseString, Element


class VSFlags:
    """Flags corresponding to cmIDEFlagTable."""
    UserValue = "UserValue"  # (1 << 0)
    UserIgnored = "UserIgnored"  # (1 << 1)
    UserRequired = "UserRequired"  # (1 << 2)
    Continue = "Continue"  #(1 << 3)
    SemicolonAppendable = "SemicolonAppendable"  # (1 << 4)
    UserFollowing = "UserFollowing"  # (1 << 5)
    CaseInsensitive = "CaseInsensitive"  # (1 << 6)
    UserValueIgnored = [UserValue, UserIgnored]
    UserValueRequired = [UserValue, UserRequired]


def vsflags(*args):
    """Combines the flags."""
    values = []

    for arg in args:
        __append_list(values, arg)

    return values


def read_msbuild_xml(path, values={}):
    """Reads the MS Build XML file at the path and returns its contents.

    Keyword arguments:
    values -- The map to append the contents to (default {})
    """

    # Attempt to read the file contents
    try:
        document = parse(path)
    except Exception as e:
        logging.exception('Could not read MS Build XML file at %s', path)
        return values

    # Convert the XML to JSON format
    logging.info('Processing MS Build XML file at %s', path)

    # Get the rule node
    rule = document.getElementsByTagName('Rule')[0]

    rule_name = rule.attributes['Name'].value

    logging.info('Found rules for %s', rule_name)

    # Proprocess Argument values
    __preprocess_arguments(rule)

    # Get all the values
    converted_values = []
    __convert(rule, 'EnumProperty', converted_values, __convert_enum)
    __convert(rule, 'BoolProperty', converted_values, __convert_bool)
    __convert(rule, 'StringListProperty', converted_values,
              __convert_string_list)
    __convert(rule, 'StringProperty', converted_values, __convert_string)
    __convert(rule, 'IntProperty', converted_values, __convert_string)

    values[rule_name] = converted_values

    return values


def read_msbuild_json(path, values=[]):
    """Reads the MS Build JSON file at the path and returns its contents.

    Keyword arguments:
    values -- The list to append the contents to (default [])
    """
    if not os.path.exists(path):
        logging.info('Could not find MS Build JSON file at %s', path)
        return values

    try:
        values.extend(__read_json_file(path))
    except Exception as e:
        logging.exception('Could not read MS Build JSON file at %s', path)
        return values

    logging.info('Processing MS Build JSON file at %s', path)

    return values


def main():
    """Script entrypoint."""
    # Parse the arguments
    parser = argparse.ArgumentParser(
        description='Convert MSBuild XML to JSON format')

    parser.add_argument(
        '-t', '--toolchain', help='The name of the toolchain', required=True)
    parser.add_argument(
        '-o', '--output', help='The output directory', default='')
    parser.add_argument(
        '-r',
        '--overwrite',
        help='Whether previously output should be overwritten',
        dest='overwrite',
        action='store_true')
    parser.set_defaults(overwrite=False)
    parser.add_argument(
        '-d',
        '--debug',
        help="Debug tool output",
        action="store_const",
        dest="loglevel",
        const=logging.DEBUG,
        default=logging.WARNING)
    parser.add_argument(
        '-v',
        '--verbose',
        help="Verbose output",
        action="store_const",
        dest="loglevel",
        const=logging.INFO)
    parser.add_argument('input', help='The input files', nargs='+')

    args = parser.parse_args()

    toolchain = args.toolchain

    logging.basicConfig(level=args.loglevel)
    logging.info('Creating %s toolchain files', toolchain)

    values = {}

    # Iterate through the inputs
    for input in args.input:
        input = __get_path(input)

        read_msbuild_xml(input, values)

    # Determine if the output directory needs to be created
    output_dir = __get_path(args.output)

    if not os.path.exists(output_dir):
        os.mkdir(output_dir)
        logging.info('Created output directory %s', output_dir)

    for key, value in values.items():
        output_path = __output_path(toolchain, key, output_dir)

        if os.path.exists(output_path) and not args.overwrite:
            logging.info('Comparing previous output to current')

            __merge_json_values(value, read_msbuild_json(output_path))
        else:
            logging.info('Original output will be overwritten')

        logging.info('Writing MS Build JSON file at %s', output_path)

        __write_json_file(output_path, value)


###########################################################################################
# private joining functions
def __merge_json_values(current, previous):
    """Merges the values between the current and previous run of the script."""
    for value in current:
        name = value['name']

        # Find the previous value
        previous_value = __find_and_remove_value(previous, value)

        if previous_value is not None:
            flags = value['flags']
            previous_flags = previous_value['flags']

            if flags != previous_flags:
                logging.warning(
                    'Flags for %s are different. Using previous value.', name)

                value['flags'] = previous_flags
        else:
            logging.warning('Value %s is a new value', name)

    for value in previous:
        name = value['name']
        logging.warning(
            'Value %s not present in current run. Appending value.', name)

        current.append(value)


def __find_and_remove_value(list, compare):
    """Finds the value in the list that corresponds with the value of compare."""
    # next throws if there are no matches
    try:
        found = next(value for value in list
                     if value['name'] == compare['name'] and value['switch'] ==
                     compare['switch'])
    except:
        return None

    list.remove(found)

    return found


###########################################################################################
# private xml functions
def __convert(root, tag, values, func):
    """Converts the tag type found in the root and converts them using the func
    and appends them to the values.
    """
    elements = root.getElementsByTagName(tag)

    for element in elements:
        converted = func(element)

        # Append to the list
        __append_list(values, converted)


def __convert_enum(node):
    """Converts an EnumProperty node to JSON format."""
    name = __get_attribute(node, 'Name')
    logging.debug('Found EnumProperty named %s', name)

    converted_values = []

    for value in node.getElementsByTagName('EnumValue'):
        converted = __convert_node(value)

        converted['value'] = converted['name']
        converted['name'] = name

        # Modify flags when there is an argument child
        __with_argument(value, converted)

        converted_values.append(converted)

    return converted_values


def __convert_bool(node):
    """Converts an BoolProperty node to JSON format."""
    converted = __convert_node(node, default_value='true')

    # Check for a switch for reversing the value
    reverse_switch = __get_attribute(node, 'ReverseSwitch')

    if reverse_switch:
        converted_reverse = copy.deepcopy(converted)

        converted_reverse['switch'] = reverse_switch
        converted_reverse['value'] = 'false'

        return [converted_reverse, converted]

    # Modify flags when there is an argument child
    __with_argument(node, converted)

    return __check_for_flag(converted)


def __convert_string_list(node):
    """Converts a StringListProperty node to JSON format."""
    converted = __convert_node(node)

    # Determine flags for the string list
    flags = vsflags(VSFlags.UserValue)

    # Check for a separator to determine if it is semicolon appendable
    # If not present assume the value should be ;
    separator = __get_attribute(node, 'Separator', default_value=';')

    if separator == ';':
        flags = vsflags(flags, VSFlags.SemicolonAppendable)

    converted['flags'] = flags

    return __check_for_flag(converted)


def __convert_string(node):
    """Converts a StringProperty node to JSON format."""
    converted = __convert_node(node, default_flags=vsflags(VSFlags.UserValue))

    return __check_for_flag(converted)


def __convert_node(node, default_value='', default_flags=vsflags()):
    """Converts a XML node to a JSON equivalent."""
    name = __get_attribute(node, 'Name')
    logging.debug('Found %s named %s', node.tagName, name)

    converted = {}
    converted['name'] = name
    converted['switch'] = __get_attribute(node, 'Switch')
    converted['comment'] = __get_attribute(node, 'DisplayName')
    converted['value'] = default_value

    # Check for the Flags attribute in case it was created during preprocessing
    flags = __get_attribute(node, 'Flags')

    if flags:
        flags = flags.split(',')
    else:
        flags = default_flags

    converted['flags'] = flags

    return converted


def __check_for_flag(value):
    """Checks whether the value has a switch value.

    If not then returns None as it should not be added.
    """
    if value['switch']:
        return value
    else:
        logging.warning('Skipping %s which has no command line switch',
                        value['name'])
        return None


def __with_argument(node, value):
    """Modifies the flags in value if the node contains an Argument."""
    arguments = node.getElementsByTagName('Argument')

    if arguments:
        logging.debug('Found argument within %s', value['name'])
        value['flags'] = vsflags(VSFlags.UserValueIgnored, VSFlags.Continue)


def __preprocess_arguments(root):
    """Preprocesses occurrances of Argument within the root.

    Argument XML values reference other values within the document by name. The
    referenced value does not contain a switch. This function will add the
    switch associated with the argument.
    """
    # Set the flags to require a value
    flags = ','.join(vsflags(VSFlags.UserValueRequired))

    # Search through the arguments
    arguments = root.getElementsByTagName('Argument')

    for argument in arguments:
        reference = __get_attribute(argument, 'Property')
        found = None

        # Look for the argument within the root's children
        for child in root.childNodes:
            # Ignore Text nodes
            if isinstance(child, Element):
                name = __get_attribute(child, 'Name')

                if name == reference:
                    found = child
                    break

        if found is not None:
            logging.info('Found property named %s', reference)
            # Get the associated switch
            switch = __get_attribute(argument.parentNode, 'Switch')

            # See if there is already a switch associated with the element.
            if __get_attribute(found, 'Switch'):
                logging.debug('Copying node %s', reference)
                clone = found.cloneNode(True)
                root.insertBefore(clone, found)
                found = clone

            found.setAttribute('Switch', switch)
            found.setAttribute('Flags', flags)
        else:
            logging.warning('Could not find property named %s', reference)


def __get_attribute(node, name, default_value=''):
    """Retrieves the attribute of the given name from the node.

    If not present then the default_value is used.
    """
    if node.hasAttribute(name):
        return node.attributes[name].value.strip()
    else:
        return default_value


###########################################################################################
# private path functions
def __get_path(path):
    """Gets the path to the file."""
    if not os.path.isabs(path):
        path = os.path.join(os.getcwd(), path)

    return os.path.normpath(path)


def __output_path(toolchain, rule, output_dir):
    """Gets the output path for a file given the toolchain, rule and output_dir"""
    filename = '%s_%s.json' % (toolchain, rule)
    return os.path.join(output_dir, filename)


###########################################################################################
# private JSON file functions
def __read_json_file(path):
    """Reads a JSON file at the path."""
    with open(path, 'r') as f:
        return json.load(f)


def __write_json_file(path, values):
    """Writes a JSON file at the path with the values provided."""
    # Sort the keys to ensure ordering
    sort_order = ['name', 'switch', 'comment', 'value', 'flags']
    sorted_values = [
        OrderedDict(
            sorted(
                value.items(), key=lambda value: sort_order.index(value[0])))
        for value in values
    ]

    with open(path, 'w') as f:
        json.dump(sorted_values, f, indent=2, separators=(',', ': '))


###########################################################################################
# private list helpers
def __append_list(append_to, value):
    """Appends the value to the list."""
    if value is not None:
        if isinstance(value, list):
            append_to.extend(value)
        else:
            append_to.append(value)

###########################################################################################
# main entry point
if __name__ == "__main__":
    main()
